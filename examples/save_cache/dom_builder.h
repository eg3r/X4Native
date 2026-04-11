// ---------------------------------------------------------------------------
// dom_builder.h -- Construct libxml2 DOM tree from X4BN binary cache
// ---------------------------------------------------------------------------
// Reads an .x4bin file and constructs a valid xmlDoc with xmlNode/xmlAttr
// trees using standard malloc. The resulting tree is indistinguishable from
// one produced by libxml2's XML parser.
//
// All nodes are allocated via malloc() so that the game's xmlFreeDoc()
// (which calls xmlFree -> free) can clean them up normally.
// ---------------------------------------------------------------------------
#pragma once
#include <cstdint>

struct BuildResult {
    void*    doc;           // xmlDocPtr, or nullptr on failure
    uint32_t node_count;    // elements constructed
    uint32_t attr_count;    // attributes constructed
    double   elapsed_ms;    // construction time
};

// Build a complete xmlDoc DOM tree from an .x4bin binary cache file.
// Returns doc=nullptr on any error (corrupt file, hash mismatch, alloc failure).
// The caller does NOT own the returned doc -- the game's xmlFreeDoc will free it.
BuildResult build_dom_from_binary(const char* x4bin_path);

// Validate that an .x4bin file's source hash matches the given XML buffer hash.
// Used to check cache freshness when we have the decompressed XML in memory.
bool validate_cache_hash(const char* x4bin_path, uint64_t xml_buffer_hash);
