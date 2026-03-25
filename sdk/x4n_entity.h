// ---------------------------------------------------------------------------
// x4n_entity.h — Entity Resolution
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::entity::find_component()  — resolve UniverseID to Object*
//
// All functions require on_game_loaded to have fired.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

namespace x4n { namespace entity {

/// Resolve a UniverseID to its Object* via the component registry.
/// Returns nullptr if the ID is invalid or the registry isn't initialized.
/// Caches the registry pointer on first call. Only call after on_game_loaded.
/// @stability MODERATE — depends on X4_RVA_COMPONENT_REGISTRY + ComponentRegistry_Find.
/// @verified v9.00 build 600626
inline void* find_component(uint64_t id) {
    auto* g = game();
    if (!g || !g->ComponentRegistry_Find) return nullptr;
    static void* s_comp_reg = nullptr;
    if (!s_comp_reg)
        s_comp_reg = *reinterpret_cast<void**>(exe_base() + X4_RVA_COMPONENT_REGISTRY);
    if (!s_comp_reg) return nullptr;
    return g->ComponentRegistry_Find(s_comp_reg, id, 4);
}

}} // namespace x4n::entity
