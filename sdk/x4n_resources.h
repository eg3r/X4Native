// ---------------------------------------------------------------------------
// x4n_resources.h — Sector Resource Access
// ---------------------------------------------------------------------------
// Part of the X4Native SDK.
//
// Provides:
//   x4n::resources::get_sector_resources()  — per-sector resource data (unfiltered)
//
// Unlike GetDiscoveredSectorResources (FFI), this reads ResourceArea objects
// directly from the sector's child vector, bypassing the player discovery
// filter. Returns ALL resources in the sector regardless of player knowledge.
//
// Also reads per-RA properties from X4RegionYieldDef (max yield, rating,
// gather speed, respawn delay, etc.) which the FFI does NOT expose.
//
// Each SectorResource carries const pointers to the underlying
// X4ResourceArea objects for per-area inspection (valid current frame only).
//
// All functions require on_game_loaded to have fired. UI thread only.
//
// @stability LOW — raw struct offsets, verify on game updates.
// @verified v9.00 build 604402
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_entity.h"
#include <cstdint>
#include <string>
#include <vector>

namespace x4n { namespace resources {

// ---------------------------------------------------------------------------
// Public types
// ---------------------------------------------------------------------------

/// Per-ware accumulated resource data for a sector.
/// Aggregated from all ResourceArea children sharing the same ware type.
struct SectorResource {
    std::string ware;       ///< Ware ID (e.g. "ore", "hydrogen", "silicon").
    int64_t     yield;      ///< Total current yield across all resource areas.
    int64_t     max_yield;  ///< Total max yield across all resource areas.

    /// Individual resource areas for this ware. Read-only, valid for current frame.
    /// Access per-area details via ra->definition (rating, gather speed, respawn, etc.).
    std::vector<const X4ResourceArea*> areas;
};

// ---------------------------------------------------------------------------
// Implementation detail — SEH requires POD types (no std::string destructors).
// ---------------------------------------------------------------------------
namespace detail {

struct RawAccum {
    const char* ware;       // game-owned pointer (WareClass name)
    int64_t     yield;
    int64_t     max_yield;
};

struct RawAreaRef {
    uint32_t                accum_idx;
    const X4ResourceArea*   ra;
};

/// SEH-guarded collection. Returns count of ware entries in accum_buf.
inline uint32_t collect_raw(
    UniverseID sector_id,
    RawAccum* accum_buf, uint32_t max_wares,
    RawAreaRef* area_buf, uint32_t max_areas, uint32_t* area_count)
{
    *area_count = 0;
    if (!accum_buf || max_wares == 0) return 0;

    __try {
        auto* sector = entity::find_component(sector_id);
        if (!sector) return 0;
        if (!entity::is_a(sector, x4n::GameClass::Sector)) return 0;

        auto base = reinterpret_cast<uintptr_t>(sector);
        auto** begin = *reinterpret_cast<X4EntityBase***>(base + detail::offsets()->sector_resarea_vec_begin);
        auto** end   = *reinterpret_cast<X4EntityBase***>(base + detail::offsets()->sector_resarea_vec_end);
        if (!begin || begin >= end) return 0;

        uint32_t wcount = 0;
        uint32_t acount = 0;
        for (auto** it = begin; it < end; ++it) {
            auto* ent = *it;
            if (!ent || !ent->id) continue;

            auto* ra  = static_cast<X4ResourceArea*>(ent);
            auto* def = ra->definition;
            if (!def || !def->ware_class) continue;

            const char* name = def->ware_class->name();
            if (!name || !name[0]) continue;

            int64_t cur = ra->current_yield;
            int64_t max = def->max_yield;
            if (max <= 0 && cur <= 0) continue;

            // Find or create ware entry (pointer equality — same WareClass)
            uint32_t widx = wcount;
            for (uint32_t i = 0; i < wcount; ++i) {
                if (accum_buf[i].ware == name) { widx = i; break; }
            }
            if (widx == wcount) {
                if (wcount >= max_wares) continue;
                accum_buf[wcount] = { name, 0, 0 };
                ++wcount;
            }

            accum_buf[widx].yield     += cur;
            accum_buf[widx].max_yield += max;

            if (acount < max_areas) {
                area_buf[acount++] = { widx, ra };
            }
        }
        *area_count = acount;
        return wcount;
    }
    __except (1) {
        return 0;
    }
}

} // namespace detail

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// Get ALL resource data for a sector, bypassing the player discovery filter.
///
/// Returns a vector of per-ware resource data accumulated across all
/// ResourceArea children. Each entry has total yield/max and a vector of
/// const pointers to the underlying X4ResourceArea objects for per-area
/// inspection (definition, rating, gather speed, respawn, etc.).
///
/// @param sector_id   Sector UniverseID
/// @return            Vector of SectorResource entries. Owned strings.
///                    Area pointers valid for current frame only.
///
/// @note UI thread only.
inline std::vector<SectorResource> get_sector_resources(UniverseID sector_id)
{
    constexpr uint32_t MAX_WARES = 16;
    constexpr uint32_t MAX_AREAS = 256;

    detail::RawAccum accum[MAX_WARES];
    detail::RawAreaRef areas[MAX_AREAS];
    uint32_t area_count = 0;
    uint32_t n = detail::collect_raw(sector_id, accum, MAX_WARES,
                                     areas, MAX_AREAS, &area_count);

    std::vector<SectorResource> results;
    results.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        results.push_back({
            std::string(accum[i].ware),
            accum[i].yield,
            accum[i].max_yield,
            {},
        });
    }

    for (uint32_t i = 0; i < area_count; ++i) {
        auto& ref = areas[i];
        if (ref.accum_idx < n) {
            results[ref.accum_idx].areas.push_back(ref.ra);
        }
    }

    return results;
}

}} // namespace x4n::resources
