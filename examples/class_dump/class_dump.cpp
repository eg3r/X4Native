// ---------------------------------------------------------------------------
// x4native_class_dump — dumps the full X4 entity class registry and MD event
// type IDs to CSV files.
//
// PURPOSE
//   X4's entity system assigns a numeric ID to every class name (e.g. "ship"=115,
//   "sector"=86). Those IDs are internal C++ constants not exposed in any static
//   reference file. GetComponentClassMatrix() is the only public API that returns
//   both the string name AND the numeric ID for every registered class.
//
//   The MD event system uses polymorphic C++ event objects, each with a vtable
//   whose slot [1] returns a compile-time integer type ID (e.g. KilledEvent=233).
//   These IDs are discoverable at runtime via RTTI scanning of the .rdata section.
//
//   Run this extension once after installing, then copy the CSV files out.
//   The extension can be disabled immediately afterward.
//
// OUTPUT  (written to the extension folder on on_game_loaded)
//   class_ids.csv         — unique (id, name) pairs, sorted by id.
//   class_matrix.csv      — full inheritance hierarchy.
//   event_type_ids.csv    — MD event type IDs from RTTI scan (id, name).
//
// WHY NOT x4n::log?
//   x4n::log prepends a timestamp to every line, which would require stripping
//   before the CSV can be used. std::ofstream gives clean, undecorated output.
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cstring>
#include <windows.h>

static int g_sub_loaded = 0;

// ---------------------------------------------------------------------------
// MD event type scanner — three data sources merged into event_type_ids.csv:
//
// 1. PropertyChange Type Table (.rdata) — short_name → type_id (556 entries)
// 2. RTTI scan (.rdata COL walk) — C++ class name + vtable RVA → type_id
// 3. Property Descriptor Table (.rdata) — XSD event names for reference
//
// The generator script (generate_event_type_ids.ps1) cross-references the CSV
// with common.xsd to produce x4_md_events.h with typed subscription functions.
// ---------------------------------------------------------------------------
#include <unordered_map>

/// PropertyChange Type Table entry layout (16 bytes, .rdata)
struct PropChangeTypeEntry {
    const char* name;       // short lowercase name ("killed", "attacked", etc.)
    uint32_t    type_id;
    uint32_t    _pad;
};

/// Find the PropertyChange Type Table in .rdata by scanning for its signature:
/// sequential entries where entry[i].type_id == i, with valid string ptrs.
/// Returns (table_ptr, count) or (nullptr, 0) if not found.
static std::pair<const PropChangeTypeEntry*, uint32_t> find_propchange_type_table(
    uintptr_t rdata_start, uintptr_t rdata_end, uintptr_t exe_base)
{
    // The table has ~556 entries starting at type_id 0..555, each 16 bytes.
    // Entry[0] may be a sentinel (null name or type_id 0 with no string).
    // We scan for a long run of sequential type_ids. Allow entries with null/empty
    // names (sentinels) but require enough valid string pointers to confirm.

    for (uintptr_t addr = rdata_start; addr + 16 * 100 <= rdata_end; addr += 8) {
        auto* candidate = reinterpret_cast<const PropChangeTypeEntry*>(addr);

        // Quick check: first entry should have type_id 0
        if (candidate[0].type_id != 0) continue;

        // Count sequential type_ids and valid string entries
        uint32_t sequential = 0;
        uint32_t valid_strings = 0;
        for (uint32_t i = 0; i < 600 && addr + (i + 1) * 16 <= rdata_end; i++) {
            auto& e = candidate[i];
            if (e.type_id != i) break;
            sequential++;

            // Check if name is a valid string pointer
            auto sptr = reinterpret_cast<uintptr_t>(e.name);
            if (sptr >= exe_base && sptr < exe_base + 0x20000000 && e.name[0] >= 'a' && e.name[0] <= 'z') {
                valid_strings++;
            }
            // Allow null/empty name entries (sentinels, padding)
        }

        // Need 500+ sequential IDs with at least 400 valid strings.
        // Additionally validate against known event names: RTTI "KilledEvent" → short "killed"
        // should appear at the matching type_id index. This distinguishes the event type table
        // from other sequential property tables in .rdata.
        if (sequential >= 500 && valid_strings >= 400) {
            // Spot-check: entry[29] should be "attacked" (AttackedEvent=29),
            // entry[55] should be "buildfinished" (BuildFinishedEvent=55)
            bool valid_event_table = false;
            if (sequential > 55) {
                auto& e29 = candidate[29];
                auto& e55 = candidate[55];
                if (e29.name && std::strcmp(e29.name, "attacked") == 0 &&
                    e55.name && std::strcmp(e55.name, "buildfinished") == 0) {
                    valid_event_table = true;
                }
            }
            if (valid_event_table) {
                return {candidate, sequential};
            }
        }
    }
    return {nullptr, 0};
}

/// Extract clean class name from RTTI decorated name.
/// ".?AVKilledEvent@U@@" → "KilledEvent"
static std::string extract_event_class_name(const char* decorated) {
    const char* start = decorated;
    if (start[0] == '.' && start[1] == '?' && start[2] == 'A' && start[3] == 'V')
        start += 4;
    std::string name(start);
    auto at_pos = name.find('@');
    if (at_pos != std::string::npos)
        name = name.substr(0, at_pos);
    return name;
}

static void dump_event_types(const std::string& base_path) {
    auto exe_base = x4n::detail::g_api->exe_base;
    if (!exe_base) {
        x4n::log::warn("class_dump: exe_base not available, skipping event scan");
        return;
    }

    // Parse PE headers to find .rdata section bounds
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(exe_base);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(exe_base + dos->e_lfanew);
    auto sections = IMAGE_FIRST_SECTION(nt);

    uintptr_t rdata_start = 0, rdata_end = 0;
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (std::strncmp(reinterpret_cast<char*>(sections[i].Name), ".rdata", 6) == 0) {
            rdata_start = exe_base + sections[i].VirtualAddress;
            rdata_end = rdata_start + sections[i].Misc.VirtualSize;
            break;
        }
    }
    if (!rdata_start) {
        x4n::log::warn("class_dump: .rdata section not found, skipping event scan");
        return;
    }

    // ---- Source 1: PropertyChange Type Table ----
    // Maps short_name → type_id (authoritative, 556 entries)
    std::unordered_map<uint32_t, std::string> short_names; // type_id → short_name
    auto [table, table_count] = find_propchange_type_table(rdata_start, rdata_end, exe_base);
    if (table && table_count > 0) {
        uint32_t named = 0;
        for (uint32_t i = 0; i < table_count; i++) {
            if (table[i].name && table[i].name[0]) {
                short_names[table[i].type_id] = table[i].name;
                named++;
            }
        }
        x4n::log::info("class_dump: PropertyChange type table — %u entries, %u with names",
                       table_count, named);
    } else {
        x4n::log::warn("class_dump: PropertyChange type table not found in .rdata");
    }

    // ---- Source 2: RTTI scan ----
    // Maps type_id → (C++ class name, vtable RVA)
    struct RttiEntry { std::string name; uintptr_t vtable_rva; };
    std::unordered_map<uint32_t, RttiEntry> rtti_entries; // type_id → {name, vtable_rva}
    uint32_t cols_checked = 0;

    for (uintptr_t addr = rdata_start; addr + 24 <= rdata_end; addr += 4) {
        auto* col = reinterpret_cast<uint32_t*>(addr);
        if (col[0] != 1) continue;
        if (col[5] != static_cast<uint32_t>(addr - exe_base)) continue;
        cols_checked++;

        auto td_addr = exe_base + static_cast<uint32_t>(col[3]);
        auto* td_name = reinterpret_cast<const char*>(td_addr + 16);
        if (!std::strstr(td_name, "Event@U@@")) continue;

        for (uintptr_t vt_scan = rdata_start; vt_scan + 8 <= rdata_end; vt_scan += 8) {
            if (*reinterpret_cast<uintptr_t*>(vt_scan) == addr) {
                auto vtable_va = vt_scan + 8;
                auto* vtable = reinterpret_cast<void**>(vtable_va);
                auto* fn = reinterpret_cast<uint8_t*>(vtable[1]);
                if (fn[0] == 0xB8 && fn[5] == 0xC3) {
                    uint32_t type_id = *reinterpret_cast<uint32_t*>(fn + 1);
                    rtti_entries[type_id] = {
                        extract_event_class_name(td_name),
                        vtable_va - exe_base
                    };
                }
                break;
            }
        }
    }

    x4n::log::info("class_dump: RTTI scan — %u COLs checked, %u event classes found",
                   cols_checked, static_cast<uint32_t>(rtti_entries.size()));

    // ---- Source 3: XSD event name descriptor scan ----
    // Scan .rdata for "event_*" string pointers paired with descriptor IDs.
    // Entries are 16 bytes: {const char* name, uint32_t descriptor_id, uint32_t pad}.
    // These are dumped to the CSV for reference but NOT used for type_id mapping
    // (descriptor_ids are a separate MD scripting namespace; the generator script
    // matches XSD names to short_names algorithmically via common.xsd).

    std::unordered_map<uint32_t, std::string> descid_to_xsd;

    for (uintptr_t addr = rdata_start; addr + 16 <= rdata_end; addr += 8) {
        auto str_ptr = *reinterpret_cast<uintptr_t*>(addr);
        auto desc_id = *reinterpret_cast<uint32_t*>(addr + 8);

        if (str_ptr < rdata_start || str_ptr >= rdata_end) continue;
        auto* str = reinterpret_cast<const char*>(str_ptr);
        if (std::strncmp(str, "event_", 6) != 0) continue;
        if (desc_id < 1000 || desc_id > 2500) continue;

        size_t len = 0;
        bool valid = true;
        for (; len < 80 && str[len]; len++) {
            if (str[len] < 0x20 || str[len] > 0x7E) { valid = false; break; }
        }
        if (!valid || len < 8 || len > 70) continue;
        descid_to_xsd[desc_id] = std::string(str, len);
    }

    x4n::log::info("class_dump: found %u unique event_* descriptor entries",
                   static_cast<uint32_t>(descid_to_xsd.size()));

    // ---- Merge and write CSV ----
    std::set<uint32_t> all_ids;
    for (auto& [id, _] : short_names) all_ids.insert(id);
    for (auto& [id, _] : rtti_entries) all_ids.insert(id);

    {
        std::ofstream f(base_path + "event_type_ids.csv");
        if (!f) {
            x4n::log::error("class_dump: could not open event_type_ids.csv for writing");
            return;
        }
        f << "id,rtti_name,short_name,vtable_rva\n";
        for (uint32_t id : all_ids) {
            auto rtti_it = rtti_entries.find(id);
            auto short_it = short_names.find(id);
            f << id << ","
              << (rtti_it != rtti_entries.end() ? rtti_it->second.name : "") << ","
              << (short_it != short_names.end() ? short_it->second : "") << ",";
            if (rtti_it != rtti_entries.end())
                f << "0x" << std::hex << rtti_it->second.vtable_rva << std::dec;
            f << "\n";
        }
    }

    x4n::log::info("class_dump: merged %u event types → event_type_ids.csv",
                   static_cast<uint32_t>(all_ids.size()));
}

static void on_game_loaded() {
    auto* g = x4n::game();
    if (!g) {
        x4n::log::warn("class_dump: game function table not available");
        return;
    }

    // GetComponentClassMatrix does not support the null-pointer count-query pattern —
    // it returns 0 when result is null. Allocate a generous buffer upfront.
    // ~100 classes × ~100 classes = 10000 max pairs; actual count is far less
    // (only true IS-A relationships are stored), so 10000 is safe.
    const uint32_t MAX_PAIRS = 10000;
    std::vector<ComponentClassPair> pairs(MAX_PAIRS);
    uint32_t filled = g->GetComponentClassMatrix(pairs.data(), MAX_PAIRS);
    if (filled == 0) {
        x4n::log::warn("class_dump: GetComponentClassMatrix returned 0 pairs — "
                       "is the game world fully loaded?");
        return;
    }
    pairs.resize(filled);

    // Collect unique (id, name) pairs from both matrix columns, sorted by id.
    std::set<std::pair<uint32_t, std::string>> ids;
    for (const auto& p : pairs) {
        if (p.class1name && p.class1name[0]) ids.insert({p.class1id, p.class1name});
        if (p.class2name && p.class2name[0]) ids.insert({p.class2id, p.class2name});
    }

    const std::string base = std::string(x4n::path()) + "/";

    // --- class_ids.csv ---
    // One row per registered class: id and name, sorted ascending by id.
    // This is the primary output for filling in SUBSYSTEMS.md §13.2.
    {
        std::ofstream f(base + "class_ids.csv");
        if (!f) {
            x4n::log::error("class_dump: could not open %sclass_ids.csv for writing", base.c_str());
            return;
        }
        f << "id,name\n";
        for (const auto& [id, name] : ids)
            f << id << "," << name << "\n";
    }

    // --- class_matrix.csv ---
    // Full relationship table. is_subclass=1 means class1 IS-A class2.
    // Useful for understanding the inheritance hierarchy (which classes
    // derive from which parents).
    {
        std::ofstream f(base + "class_matrix.csv");
        if (!f) {
            x4n::log::error("class_dump: could not open %sclass_matrix.csv for writing", base.c_str());
            return;
        }
        f << "class1name,class1id,class2name,class2id,is_subclass\n";
        for (const auto& p : pairs) {
            f << (p.class1name ? p.class1name : "") << ","
              << p.class1id                         << ","
              << (p.class2name ? p.class2name : "") << ","
              << p.class2id                         << ","
              << (p.isclass ? "1" : "0")            << "\n";
        }
    }

    x4n::log::info("class_dump: wrote %u unique classes (%u pairs) → %s",
                   static_cast<uint32_t>(ids.size()), filled, base.c_str());
    x4n::log::info("class_dump: files: class_ids.csv, class_matrix.csv");
}

X4N_EXTENSION {
    // Event scanning reads static .rdata/.text — run immediately, no game state needed.
    const std::string base = std::string(x4n::path()) + "/";
    dump_event_types(base);

    // Class dump needs GetComponentClassMatrix FFI — requires game loaded.
    g_sub_loaded = x4n::on("on_game_loaded", on_game_loaded);
    x4n::log::info("class_dump: event_type_ids.csv written; class CSVs on game load");
}

X4N_SHUTDOWN {
    x4n::off(g_sub_loaded);
}
