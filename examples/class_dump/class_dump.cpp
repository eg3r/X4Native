// ---------------------------------------------------------------------------
// x4native_class_dump — dumps the full X4 entity class registry to CSV files.
//
// PURPOSE
//   X4's entity system assigns a numeric ID to every class name (e.g. "ship"=115,
//   "sector"=86). Those IDs are internal C++ constants not exposed in any static
//   reference file. GetComponentClassMatrix() is the only public API that returns
//   both the string name AND the numeric ID for every registered class.
//
//   Run this extension once after installing, then copy the CSV files out.
//   The extension can be disabled immediately afterward.
//
// OUTPUT  (written to the extension folder on on_game_loaded)
//   class_ids.csv    — unique (id, name) pairs, sorted by id.  Use this to
//                      fill in X4Native/docs/rev/SUBSYSTEMS.md §13.2.
//   class_matrix.csv — full (class1name, class1id, class2name, class2id,
//                      is_subclass) matrix for the complete hierarchy.
//
// WHY NOT x4n::log?
//   x4n::log prepends a timestamp to every line, which would require stripping
//   before the CSV can be used. std::ofstream gives clean, undecorated output.
// ---------------------------------------------------------------------------
#include <x4native.h>
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <vector>

static int g_sub_loaded = 0;

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
    g_sub_loaded = x4n::on("on_game_loaded", on_game_loaded);
    x4n::log::info("class_dump: ready — will write CSVs on game load");
}

X4N_SHUTDOWN {
    x4n::off(g_sub_loaded);
}
