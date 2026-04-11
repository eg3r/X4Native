// ---------------------------------------------------------------------------
// binary_format.h -- X4BN binary save cache format
// ---------------------------------------------------------------------------
// On-disk format for pre-parsed XML DOM data. The DLL reads this to construct
// libxml2 xmlNode trees without parsing XML text.
//
// See docs/rev/BINARY_SAVE_CACHE_PLAN.md for the full specification.
// ---------------------------------------------------------------------------
#pragma once
#include <cstdint>

static constexpr uint32_t X4BN_MAGIC   = 0x4E423458; // "X4BN" little-endian
static constexpr uint32_t X4BN_VERSION = 1;

#pragma pack(push, 1)

struct X4BinHeader {
    uint32_t magic;               // X4BN_MAGIC
    uint32_t format_version;      // X4BN_VERSION
    uint64_t source_hash;         // xxhash64 of the .xml.gz file
    uint32_t node_count;          // number of element nodes
    uint32_t attr_count;          // number of attribute records
    uint32_t string_table_offset; // byte offset from file start
    uint32_t string_table_size;   // bytes (packed null-terminated strings)
    uint32_t node_table_offset;   // byte offset to NodeRecord array
    uint32_t attr_table_offset;   // byte offset to AttrRecord array
    uint8_t  reserved[16];
};

static_assert(sizeof(X4BinHeader) == 56);

// One per XML element (<component>, <connection>, <listener>, etc.)
struct NodeRecord {
    uint32_t name_offset;         // into string table
    uint32_t first_child_idx;     // node index, 0xFFFFFFFF = none
    uint32_t next_sibling_idx;    // node index, 0xFFFFFFFF = none
    uint32_t first_attr_idx;      // attr index, 0xFFFFFFFF = none
    uint16_t child_count;
    uint16_t flags;               // reserved
};

static_assert(sizeof(NodeRecord) == 20);

// One per XML attribute (class="ship_s", id="[0x16a855]", etc.)
struct AttrRecord {
    uint32_t name_offset;         // into string table
    uint32_t value_offset;        // into string table
    uint32_t next_attr_idx;       // attr index, 0xFFFFFFFF = last
};

static_assert(sizeof(AttrRecord) == 12);

static constexpr uint32_t IDX_NONE = 0xFFFFFFFF;

#pragma pack(pop)
