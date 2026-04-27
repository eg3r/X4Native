// ---------------------------------------------------------------------------
// X4Native Extension SDK — Modern C++ API
// ---------------------------------------------------------------------------
//
// Convenience header that pulls in the entire SDK. Use this if you don't
// care about compile-time granularity:
//
//   #include <x4native.h>    // everything
//
// For selective includes, pick only what you need. Every sub-header is
// self-contained (includes x4n_core.h automatically):
//
//   #include <x4n_core.h>           // x4n::game(), exe_base(), X4N_EXTENSION macro
//   #include <x4n_events.h>         // x4n::on/off/raise/bridge_lua_event
//   #include <x4n_log.h>            // x4n::log::info/warn/error/debug
//   #include <x4n_stash.h>          // x4n::stash::set/get (survives /reloadui)
//   #include <x4n_hooks.h>          // x4n::hook::before/after/remove
//   #include <x4n_settings.h>       // x4n::settings::get/set, on_setting_changed
//
// Game domain helpers:
//
//   #include <x4n_entity.h>         // x4n::entity::find_component
//   #include <x4n_math.h>           // x4n::math::RAD_TO_DEG, advance_seed, fnv1a_lower
//   #include <x4n_rooms.h>          // x4n::rooms::roomtype_name
//   #include <x4n_memory.h>         // x4n::memory::game_alloc, game_alloc_array (SMem pool)
//   #include <x4n_plans.h>          // x4n::plans::resolve_macro, plan_registry, plan_set_entries, ...
//   #include <x4n_visibility.h>     // x4n::visibility::get_radar_visible, is_map_visible, ...
//   #include <x4n_galaxy.h>         // x4n::galaxy::find_sector_by_macro, rebuild_cache, ...
//   #include <x4n_faction.h>        // x4n::faction::find_class_by_id, get_relation, ...
//   #include <x4n_sector.h>         // x4n::sector::Sector (sunlight, resources)
//   #include <x4n_resources.h>      // x4n::resources::get_sector_resources (unfiltered)
//   #include <x4n_module.h>         // x4n::module::Module (production/processing module wrapper)
//   #include <x4n_station.h>        // x4n::station::Station (station wrapper, uses x4n_module.h)
//   #include <x4n_ship.h>           // x4n::ship::Ship (typed handle + order creation)
//   #include <x4n_container.h>      // x4n::container::Container (station/ship base, price factor)
//   #include <x4n_buildprocessor.h> // x4n::buildprocessor::Buildprocessor (construction progress)
//
// Minimal extension (events only):
//
//   #include <x4n_core.h>
//   #include <x4n_events.h>
//
//   X4N_EXTENSION {
//       x4n::on("on_game_loaded", [] { /* ... */ });
//   }
//
// Full extension (all features):
//
//   #include <x4native.h>
//
//   X4N_EXTENSION {
//       x4n::log::info("Hello from my extension!");
//       x4n::on("on_game_loaded", [] {
//           auto* g = x4n::game();
//           x4n::log::info("Player ID: %llu", g->GetPlayerID());
//       });
//   }
//
// See docs/SDK_CONTRACT.md for the full API reference.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_events.h"
#include "x4n_log.h"
#include "x4n_stash.h"
#include "x4n_hooks.h"
#include "x4n_settings.h"

// Game domain helpers
#include "x4n_entity.h"
#include "x4n_faction.h"
#include "x4n_math.h"
#include "x4n_memory.h"
#include "x4n_rooms.h"
#include "x4n_plans.h"
#include "x4n_visibility.h"
#include "x4n_galaxy.h"
#include "x4n_sector.h"
#include "x4n_resources.h"
#include "x4n_module.h"
#include "x4n_station.h"
#include "x4n_ship.h"
#include "x4n_container.h"
#include "x4n_buildprocessor.h"

