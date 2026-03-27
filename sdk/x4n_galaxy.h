// ---------------------------------------------------------------------------
// x4n_galaxy.h — Galaxy Sector/Cluster Cache
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::galaxy::rebuild_cache()          — scan galaxy, build macro<->ID maps
//   x4n::galaxy::find_sector_by_macro()   — O(1) macro name -> UniverseID
//   x4n::galaxy::find_cluster_by_macro()  — O(1) macro name -> UniverseID
//   x4n::galaxy::for_each_sector()        — iterate all sectors with callback
//   x4n::galaxy::sector_count()           — number of cached sectors
//
// Cache is built by enumerating faction-owned sectors via FFI, then reading
// each component's macro name via x4n::entity::get_component_macro().
// Call rebuild_cache() after on_game_loaded.
//
// All functions require on_game_loaded to have fired.
// @stability LOW risk — uses only FFI exports + entity::get_component_macro (MODERATE).
// @verified v9.00 build 602526
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_entity.h"
#include <unordered_map>
#include <string>
#include <cstdint>

namespace x4n { namespace galaxy {

// ---------------------------------------------------------------------------
// Internal cache — use find_*_by_macro() to query, not direct access.
// One instance per extension DLL (inline variable, C++17).
// ---------------------------------------------------------------------------
namespace cache {
    inline std::unordered_map<std::string, uint64_t> sectors;
    inline std::unordered_map<std::string, uint64_t> clusters;
}

// ---------------------------------------------------------------------------
// Cache management
// ---------------------------------------------------------------------------

/// Rebuild the sector/cluster cache by scanning the galaxy.
/// Safe to call multiple times. Typically called once from on_game_loaded.
/// Enumerates all sectors via GetSectorsByOwner per faction, plus "ownerless".
/// ~152 sectors in a full galaxy, <1ms.
inline void rebuild_cache() {
    cache::sectors.clear();
    cache::clusters.clear();

    auto* g = game();
    if (!g) return;

    auto cache_sector = [&](uint64_t sid) {
        const char* macro = entity::get_component_macro(sid);
        if (!macro || !macro[0]) return;
        cache::sectors.emplace(macro, sid);

        UniverseID cluster = g->GetContextByClass(
            static_cast<UniverseID>(sid), "cluster", false);
        if (cluster != 0) {
            const char* cmacro = entity::get_component_macro(
                static_cast<uint64_t>(cluster));
            if (cmacro && cmacro[0])
                cache::clusters.emplace(cmacro, static_cast<uint64_t>(cluster));
        }
    };

    // X4 has ~30 factions max, ~152 sectors in full galaxy.
    constexpr uint32_t MAX_FACTIONS = 128;
    constexpr uint32_t MAX_SECTORS  = 256;
    const char* factions[MAX_FACTIONS] = {};
    UniverseID  sector_buf[MAX_SECTORS] = {};

    uint32_t nf = g->GetAllFactions(factions, MAX_FACTIONS, true);
    for (uint32_t fi = 0; fi < nf; ++fi) {
        uint32_t ns = g->GetSectorsByOwner(sector_buf, MAX_SECTORS, factions[fi]);
        for (uint32_t si = 0; si < ns; ++si)
            cache_sector(static_cast<uint64_t>(sector_buf[si]));
    }

    // Unowned sectors.
    uint32_t ns = g->GetSectorsByOwner(sector_buf, MAX_SECTORS, "ownerless");
    for (uint32_t si = 0; si < ns; ++si)
        cache_sector(static_cast<uint64_t>(sector_buf[si]));
}

// ---------------------------------------------------------------------------
// Lookups
// ---------------------------------------------------------------------------

/// Find a sector's local UniverseID by its macro name. Returns 0 if not found.
inline uint64_t find_sector_by_macro(const char* macro) {
    if (!macro || !macro[0]) return 0;
    auto it = cache::sectors.find(macro);
    return (it != cache::sectors.end()) ? it->second : 0;
}

/// Find a cluster's local UniverseID by its macro name. Returns 0 if not found.
inline uint64_t find_cluster_by_macro(const char* macro) {
    if (!macro || !macro[0]) return 0;
    auto it = cache::clusters.find(macro);
    return (it != cache::clusters.end()) ? it->second : 0;
}

inline size_t sector_count()  { return cache::sectors.size(); }
inline size_t cluster_count() { return cache::clusters.size(); }

// ---------------------------------------------------------------------------
// Iteration
// ---------------------------------------------------------------------------

/// Iterate all cached sectors. Callback: void(uint64_t id, const char* macro).
template<typename Fn>
inline void for_each_sector(Fn&& fn) {
    for (const auto& [macro, id] : cache::sectors)
        fn(id, macro.c_str());
}

/// Iterate all cached clusters. Callback: void(uint64_t id, const char* macro).
template<typename Fn>
inline void for_each_cluster(Fn&& fn) {
    for (const auto& [macro, id] : cache::clusters)
        fn(id, macro.c_str());
}

}} // namespace x4n::galaxy
