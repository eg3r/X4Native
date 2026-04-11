// ---------------------------------------------------------------------------
// struct_verify.cpp -- Runtime verification of libxml2 struct layout
// ---------------------------------------------------------------------------
#include "struct_verify.h"
#include <x4n_log.h>
#include <cstring>

// Helper: read a pointer-sized value at byte offset from base
template<typename T>
static T read_at(void* base, size_t offset) {
    T val{};
    std::memcpy(&val, static_cast<char*>(base) + offset, sizeof(T));
    return val;
}

// Safe read: returns false if access causes SEH exception
static bool safe_read_int(void* base, size_t offset, int& out) {
    __try {
        out = read_at<int>(base, offset);
        return true;
    } __except(1) { return false; }
}

static bool safe_read_ptr(void* base, size_t offset, void*& out) {
    __try {
        out = read_at<void*>(base, offset);
        return true;
    } __except(1) { return false; }
}

static const char* safe_read_str(void* ptr) {
    __try {
        if (ptr && ((const char*)ptr)[0] >= 0x20 && ((const char*)ptr)[0] < 0x7F)
            return (const char*)ptr;
    } __except(1) {}
    return nullptr;
}

bool verify_libxml2_layout(void* obj) {
    if (!obj) {
        x4n::log::error("struct_verify: returned object is NULL");
        return false;
    }

    // The returned object is NOT a raw xmlDocPtr -- it's a game wrapper.
    // Probe the object to understand its layout and find the embedded xmlDoc.

    x4n::log::info("struct_verify: probing object at %p", obj);

    // --- Hex dump first 256 bytes ---
    x4n::log::info("struct_verify: hex dump (first 256 bytes):");
    for (size_t row = 0; row < 256; row += 32) {
        char hex[200];
        int pos = 0;
        pos += snprintf(hex + pos, sizeof(hex) - pos, "+%03X:", (unsigned)row);
        for (size_t col = 0; col < 32; col += 8) {
            uint64_t val = 0;
            __try { val = read_at<uint64_t>(obj, row + col); } __except(1) { val = 0xDEAD; }
            pos += snprintf(hex + pos, sizeof(hex) - pos, " %016llX", (unsigned long long)val);
        }
        x4n::log::info("struct_verify: %s", hex);
    }

    // --- Scan for XML_DOCUMENT_NODE (9) ---
    x4n::log::info("struct_verify: scanning for XML_DOCUMENT_NODE (9) in first 512 bytes...");
    for (size_t off = 0; off < 512; off += 4) {
        int val = 0;
        if (!safe_read_int(obj, off, val)) break;
        if (val == XML_DOCUMENT_NODE) {
            x4n::log::info("struct_verify:   FOUND type=9 at offset +0x%03X", (unsigned)off);
        }
    }

    // --- Scan for pointer-sized values that point back to obj (self-pointer) ---
    x4n::log::info("struct_verify: scanning for self-pointer in first 512 bytes...");
    for (size_t off = 0; off < 512; off += 8) {
        void* val = nullptr;
        if (!safe_read_ptr(obj, off, val)) break;
        if (val == obj) {
            x4n::log::info("struct_verify:   FOUND self-pointer at offset +0x%03X", (unsigned)off);
        }
    }

    // --- Scan for pointers whose target contains "savegame" string ---
    x4n::log::info("struct_verify: scanning for 'savegame' name pointer...");
    for (size_t off = 0; off < 512; off += 8) {
        void* val = nullptr;
        if (!safe_read_ptr(obj, off, val)) break;
        if (!val) continue;
        const char* s = safe_read_str(val);
        if (s && std::strcmp(s, "savegame") == 0) {
            x4n::log::info("struct_verify:   FOUND 'savegame' string ptr at offset +0x%03X", (unsigned)off);
        }
    }

    // --- Scan for child pointers: look for pointers to objects containing type=1 (ELEMENT_NODE) ---
    x4n::log::info("struct_verify: scanning for child element pointers...");
    for (size_t off = 0; off < 512; off += 8) {
        void* val = nullptr;
        if (!safe_read_ptr(obj, off, val)) break;
        if (!val || val == obj) continue;
        // Check if this pointer leads to an object with type=1 at standard xmlNode offset
        int child_type = 0;
        if (safe_read_int(val, 0x08, child_type) && child_type == XML_ELEMENT_NODE) {
            // Check if it has a name pointer
            void* name_ptr = nullptr;
            if (safe_read_ptr(val, 0x10, name_ptr) && name_ptr) {
                const char* name = safe_read_str(name_ptr);
                if (name) {
                    x4n::log::info("struct_verify:   FOUND child element at +0x%03X -> type=1, name=\"%s\"",
                                   (unsigned)off, name);
                }
            }
        }
    }

    // --- Also check if the object itself IS an xmlDoc at a non-zero base offset ---
    // Try common wrapper patterns: vtable at +0, xmlDoc at +8 or +16
    x4n::log::info("struct_verify: trying xmlDoc at wrapper offsets...");
    for (size_t base_off = 0; base_off <= 64; base_off += 8) {
        char* inner = (char*)obj + base_off;
        int type_val = 0;
        void* self_val = nullptr;
        void* children_val = nullptr;
        if (!safe_read_int(inner, 0x08, type_val)) continue;
        if (type_val != XML_DOCUMENT_NODE) continue;
        safe_read_ptr(inner, 0x40, self_val);
        safe_read_ptr(inner, 0x18, children_val);
        x4n::log::info("struct_verify:   xmlDoc candidate at wrapper+0x%02X: "
                       "type=%d, doc=%p (self=%p), children=%p",
                       (unsigned)base_off, type_val, self_val, inner, children_val);
    }

    x4n::log::info("struct_verify: probe complete -- review log to determine layout");
    return false;  // Always return false during probing phase
}
