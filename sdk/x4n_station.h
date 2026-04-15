// ---------------------------------------------------------------------------
// x4n_station.h — Station typed wrapper
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::station::Station — lightweight handle for station/defensible entities
//
// Usage:
//   x4n::station::Station station(station_id);
//   for (auto& mod : station.modules()) {
//       if (mod.is_production() && mod.ware_id() == "energycells") {
//           mod.set_paused(true);
//       }
//   }
//   // Or all-in-one:
//   uint32_t affected = station.set_ware_paused("energycells", true);
//
// Thin handle: UniverseID + cached X4Component*. No heap allocation
// except the module vector from enumeration.
// All functions require on_game_loaded to have fired.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_entity.h"
#include "x4n_module.h"
#include <cstdint>
#include <string>
#include <vector>

namespace x4n { namespace station {

/// Lightweight typed handle for a station/defensible entity.
/// Constructor validates the entity is actually a Station class.
class Station {
    UniverseID id_;
    X4Component* comp_;
public:
    explicit Station(UniverseID id)
        : id_(id), comp_(entity::find_component(id)) {
        if (comp_ && !entity::is_a(comp_, GameClass::Station))
            comp_ = nullptr;
    }

    bool valid() const { return comp_ != nullptr; }
    UniverseID id() const { return id_; }

    const char* name() const {
        auto* g = game();
        return (valid() && g) ? g->GetComponentName(id_) : "";
    }
    const char* macro() const {
        return valid() ? entity::get_component_macro(comp_) : nullptr;
    }

    /// Enumerate all modules on this station.
    std::vector<module::Module> modules(bool include_construction = false,
                                        bool include_wrecks = false) const {
        std::vector<module::Module> result;
        auto* g = game();
        if (!valid() || !g) return result;

        uint32_t count = g->GetNumStationModules(id_, include_construction, include_wrecks);
        if (count == 0) return result;

        auto buf = std::vector<UniverseID>(count);
        uint32_t actual = g->GetStationModules(buf.data(), count, id_,
                                                include_construction, include_wrecks);
        result.reserve(actual);
        for (uint32_t i = 0; i < actual; ++i) {
            result.emplace_back(buf[i]);
        }
        return result;
    }

    /// Check if this station produces a given ware (fast, station-level check).
    /// Uses ignorestate=true so it returns > 0 even when modules are paused.
    bool produces(const char* ware_id) const {
        auto* g = game();
        if (!valid() || !g) return false;
        return g->GetContainerWareProduction(id_, ware_id, true) > 0.0;
    }

    /// Pause or resume all production/processing modules for a specific ware.
    /// Returns the number of modules affected (0 if ware not found).
    uint32_t set_ware_paused(const std::string& ware_id, bool paused) {
        if (!produces(ware_id.c_str())) return 0;
        uint32_t affected = 0;
        for (auto& mod : modules()) {
            if (!mod.is_operational()) continue;
            if (!mod.is_production() && !mod.is_processing()) continue;
            if (mod.ware_id() != ware_id) continue;
            mod.set_paused(paused);
            ++affected;
        }
        return affected;
    }
};

}} // namespace x4n::station
