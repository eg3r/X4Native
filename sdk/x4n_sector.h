// ---------------------------------------------------------------------------
// x4n_sector.h — Sector typed wrapper with resource & environment access
// ---------------------------------------------------------------------------
// Part of the X4Native SDK.
//
// Provides:
//   x4n::sector::Sector — lightweight handle for sector entities
//
// Usage:
//   x4n::sector::Sector sec(sector_id);
//   if (sec.valid()) {
//       float sun = sec.sunlight();       // solar intensity (from parent cluster)
//       auto  res = sec.resources();       // all resource areas with max/current yield
//   }
//
// Thin handle: UniverseID + cached X4Component*. No heap allocation.
// All functions require on_game_loaded to have fired.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_entity.h"
#include "x4n_resources.h"
#include <cstdint>

namespace x4n { namespace sector {

/// Lightweight typed handle for a sector entity.
/// Constructor validates the entity is a Sector.
class Sector {
    UniverseID   id_;
    X4Component* comp_;

public:
    explicit Sector(UniverseID id)
        : id_(id), comp_(entity::find_component(id)) {
        if (comp_ && !entity::is_a(comp_, GameClass::Sector))
            comp_ = nullptr;
    }

    bool valid() const { return comp_ != nullptr; }
    UniverseID id() const { return id_; }
    X4Component* component() const { return comp_; }

    /// Sunlight intensity from the parent cluster (Space+0x368).
    /// Walks parent chain to find a Space entity with the sunlight flag.
    /// Returns -1.0 if not available.
    /// @verified v9.00 build 604402 (IDA: sub_1407B51D0 sunlight getter)
    float sunlight() const {
        if (!valid()) return -1.0f;
        auto* cur = reinterpret_cast<const X4Component*>(comp_->parent);
        while (cur) {
            auto p = reinterpret_cast<uintptr_t>(cur);
            if (*reinterpret_cast<const uint8_t*>(p + detail::offsets()->space_has_sunlight))
                return static_cast<float>(*reinterpret_cast<const double*>(p + detail::offsets()->space_sunlight));
            cur = reinterpret_cast<const X4Component*>(cur->parent);
        }
        return -1.0f;
    }

    /// All resource data for this sector (unfiltered, with per-area pointers).
    /// Area pointers valid for current frame only.
    std::vector<resources::SectorResource> resources() const {
        if (!comp_) return {};
        return resources::get_sector_resources(id_);
    }
};

}} // namespace x4n::sector
