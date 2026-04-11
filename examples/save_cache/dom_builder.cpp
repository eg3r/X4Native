// ---------------------------------------------------------------------------
// dom_builder.cpp -- Construct libxml2 DOM tree from X4BN binary cache
// ---------------------------------------------------------------------------
#include "dom_builder.h"
#include "binary_format.h"
#include "struct_verify.h"
#include <x4n_log.h>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <fstream>
#include <vector>

// ---------------------------------------------------------------------------
// libxml2-compatible node allocation
// ---------------------------------------------------------------------------
// All allocations use malloc/strdup so the game's xmlFreeDoc (-> free) works.

static void* alloc_zeroed(size_t size) {
    void* p = std::malloc(size);
    if (p) std::memset(p, 0, size);
    return p;
}

static char* dup_string(const char* s) {
    return s ? _strdup(s) : nullptr;
}

// Helpers to write fields at known offsets
template<typename T>
static T read_at(void* base, size_t offset) {
    T val{};
    std::memcpy(&val, static_cast<char*>(base) + offset, sizeof(T));
    return val;
}

template<typename T>
static void write_at(void* base, size_t offset, T val) {
    std::memcpy(static_cast<char*>(base) + offset, &val, sizeof(T));
}

// ---------------------------------------------------------------------------
// Core DOM construction
// ---------------------------------------------------------------------------

BuildResult build_dom_from_binary(const char* x4bin_path) {
    using namespace xml_offsets;
    using Clock = std::chrono::high_resolution_clock;
    auto t0 = Clock::now();

    BuildResult result{};

    // --- Read the entire file ---
    std::ifstream file(x4bin_path, std::ios::binary | std::ios::ate);
    if (!file) {
        x4n::log::error("dom_builder: cannot open %s", x4bin_path);
        return result;
    }
    size_t file_size = static_cast<size_t>(file.tellg());
    file.seekg(0);
    std::vector<char> data(file_size);
    file.read(data.data(), file_size);
    file.close();

    if (file_size < sizeof(X4BinHeader)) {
        x4n::log::error("dom_builder: file too small");
        return result;
    }

    // --- Parse header ---
    auto* hdr = reinterpret_cast<const X4BinHeader*>(data.data());
    if (hdr->magic != X4BN_MAGIC) {
        x4n::log::error("dom_builder: bad magic 0x%08X", hdr->magic);
        return result;
    }
    if (hdr->format_version != X4BN_VERSION) {
        x4n::log::error("dom_builder: unsupported version %u", hdr->format_version);
        return result;
    }

    const char* str_base = data.data() + hdr->string_table_offset;
    auto* nodes_tbl = reinterpret_cast<const NodeRecord*>(data.data() + hdr->node_table_offset);
    auto* attrs_tbl = reinterpret_cast<const AttrRecord*>(data.data() + hdr->attr_table_offset);

    uint32_t node_count = hdr->node_count;
    uint32_t attr_count = hdr->attr_count;

    x4n::log::info("dom_builder: %u nodes, %u attrs, %u bytes strings",
                   node_count, attr_count, hdr->string_table_size);

    // --- Allocate all xmlNodes ---
    std::vector<void*> nodes(node_count);
    for (uint32_t i = 0; i < node_count; i++) {
        nodes[i] = alloc_zeroed(SIZEOF_NODE);
        if (!nodes[i]) {
            x4n::log::error("dom_builder: malloc failed at node %u", i);
            // TODO: cleanup already allocated nodes
            return result;
        }
        write_at(nodes[i], NODE_TYPE, static_cast<int>(XML_ELEMENT_NODE));
    }

    // --- Allocate all xmlAttrs + their text value nodes ---
    std::vector<void*> attrs(attr_count);
    std::vector<void*> texts(attr_count);
    for (uint32_t i = 0; i < attr_count; i++) {
        attrs[i] = alloc_zeroed(SIZEOF_ATTR);
        texts[i] = alloc_zeroed(SIZEOF_NODE);
        if (!attrs[i] || !texts[i]) {
            x4n::log::error("dom_builder: malloc failed at attr %u", i);
            return result;
        }
        write_at(attrs[i], ATTR_TYPE, static_cast<int>(XML_ATTRIBUTE_NODE));
        write_at(texts[i], NODE_TYPE, static_cast<int>(XML_TEXT_NODE));
    }

    // --- Create xmlDoc ---
    void* doc = alloc_zeroed(SIZEOF_DOC);
    if (!doc) {
        x4n::log::error("dom_builder: malloc failed for xmlDoc");
        return result;
    }
    write_at(doc, DOC_TYPE, static_cast<int>(XML_DOCUMENT_NODE));
    write_at(doc, DOC_DOC, doc); // self-pointer

    // doc->version = "1.0", doc->encoding = "UTF-8"
    // Offsets: version at 0x68, encoding at 0x70, standalone at 0x4C
    write_at<char*>(doc, 0x68, dup_string("1.0"));
    write_at<char*>(doc, 0x70, dup_string("UTF-8"));
    write_at<int>(doc, 0x4C, -1); // standalone = -1

    // --- Wire node tree ---
    for (uint32_t i = 0; i < node_count; i++) {
        auto& rec = nodes_tbl[i];

        write_at<char*>(nodes[i], NODE_NAME, dup_string(str_base + rec.name_offset));
        write_at(nodes[i], NODE_DOC, doc);

        if (rec.first_child_idx != IDX_NONE && rec.first_child_idx < node_count)
            write_at(nodes[i], NODE_CHILDREN, nodes[rec.first_child_idx]);

        if (rec.next_sibling_idx != IDX_NONE && rec.next_sibling_idx < node_count)
            write_at(nodes[i], NODE_NEXT, nodes[rec.next_sibling_idx]);

        if (rec.first_attr_idx != IDX_NONE && rec.first_attr_idx < attr_count)
            write_at(nodes[i], NODE_PROPERTIES, attrs[rec.first_attr_idx]);
    }

    // --- Wire parent/prev/last pointers (need child -> parent linkage) ---
    for (uint32_t i = 0; i < node_count; i++) {
        void* child = read_at<void*>(nodes[i], NODE_CHILDREN);
        void* prev = nullptr;
        while (child) {
            write_at(child, NODE_PARENT, nodes[i]);
            write_at(child, NODE_PREV, prev);
            write_at(nodes[i], NODE_LAST, child); // last = most recent
            prev = child;
            child = read_at<void*>(child, NODE_NEXT);
        }
    }

    // --- Wire attributes ---
    for (uint32_t i = 0; i < attr_count; i++) {
        auto& rec = attrs_tbl[i];

        write_at<char*>(attrs[i], ATTR_NAME, dup_string(str_base + rec.name_offset));
        write_at(attrs[i], ATTR_DOC, doc);

        // Text node carries the attribute value
        write_at<char*>(texts[i], NODE_CONTENT, dup_string(str_base + rec.value_offset));
        write_at(texts[i], NODE_DOC, doc);
        write_at(texts[i], NODE_PARENT, attrs[i]); // parent is the attr
        write_at(attrs[i], ATTR_CHILDREN, texts[i]);
        // ATTR_LAST is at same offset as NODE_LAST (0x20)
        write_at<void*>(attrs[i], 0x20, texts[i]);

        if (rec.next_attr_idx != IDX_NONE && rec.next_attr_idx < attr_count)
            write_at(attrs[i], ATTR_NEXT, attrs[rec.next_attr_idx]);
    }

    // --- Wire attr parent/prev pointers ---
    for (uint32_t i = 0; i < node_count; i++) {
        void* attr = read_at<void*>(nodes[i], NODE_PROPERTIES);
        void* prev = nullptr;
        while (attr) {
            write_at(attr, ATTR_PARENT, nodes[i]);
            // xmlAttr->prev is at same offset as xmlNode->prev (0x38)
            write_at<void*>(attr, 0x38, prev);
            prev = attr;
            attr = read_at<void*>(attr, ATTR_NEXT);
        }
    }

    // --- Set document root ---
    if (node_count > 0) {
        write_at(doc, DOC_CHILDREN, nodes[0]);
        write_at<void*>(doc, 0x20, nodes[0]); // doc->last
        write_at(nodes[0], NODE_PARENT, doc);
    }

    auto t1 = Clock::now();
    result.doc = doc;
    result.node_count = node_count;
    result.attr_count = attr_count;
    result.elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    x4n::log::info("dom_builder: constructed DOM in %.1f ms (%u nodes, %u attrs)",
                   result.elapsed_ms, node_count, attr_count);

    return result;
}

// ---------------------------------------------------------------------------
// Cache validation
// ---------------------------------------------------------------------------

bool validate_cache_hash(const char* x4bin_path, uint64_t xml_buffer_hash) {
    std::ifstream file(x4bin_path, std::ios::binary);
    if (!file) return false;

    X4BinHeader hdr{};
    file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!file || hdr.magic != X4BN_MAGIC) return false;

    return hdr.source_hash == xml_buffer_hash;
}

// read_at is defined at top of this file as a static template
