// ---------------------------------------------------------------------------
// x4n_game_utils.h — Game Utility Functions
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Higher-level utilities built on the SDK primitives and RE'd constants.
// All functions require on_game_loaded to have fired.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

namespace x4n {

/// Radian-to-degree conversion factor.
/// GetObjectPositionInSector returns radians, SetObjectSectorPos expects degrees.
constexpr float RAD_TO_DEG = 180.0f / 3.14159265f;

/// Resolve a UniverseID to its Object* via the component registry.
/// Returns nullptr if the ID is invalid or the registry isn't initialized.
/// Caches the registry pointer on first call. Only call after on_game_loaded.
inline void* find_component(uint64_t id) {
    auto* g = game();
    if (!g || !g->ComponentRegistry_Find) return nullptr;
    static void* s_registry = nullptr;
    if (!s_registry) {
        s_registry = *reinterpret_cast<void**>(exe_base() + X4_RVA_COMPONENT_REGISTRY);
    }
    if (!s_registry) return nullptr;
    return g->ComponentRegistry_Find(s_registry, id, 4);
}

/// Advance a seed using the game's LCG formula (same as MD autoadvanceseed).
/// Formula: next = ROR64(seed * multiplier + addend, 30)
inline uint64_t advance_seed(uint64_t seed) {
    uint64_t lcg = seed * X4_SEED_LCG_MULTIPLIER + X4_SEED_LCG_ADDEND;
    return (lcg >> X4_SEED_LCG_ROTATE) | (lcg << (64 - X4_SEED_LCG_ROTATE));
}

/// Convert an X4RoomType enum value to its lowercase string name.
/// Returns nullptr for out-of-range or sentinel values.
inline const char* roomtype_name(X4RoomType type) {
    #define X4N_RT(e, s) case e: return s;
    switch (type) {
        X4N_RT(X4_ROOMTYPE_BAR,               "bar")
        X4N_RT(X4_ROOMTYPE_CASINO,            "casino")
        X4N_RT(X4_ROOMTYPE_CORRIDOR,          "corridor")
        X4N_RT(X4_ROOMTYPE_CREWQUARTERS,      "crewquarters")
        X4N_RT(X4_ROOMTYPE_EMBASSY,           "embassy")
        X4N_RT(X4_ROOMTYPE_FACTIONREP,        "factionrep")
        X4N_RT(X4_ROOMTYPE_GENERATORROOM,     "generatorroom")
        X4N_RT(X4_ROOMTYPE_INFRASTRUCTURE,    "infrastructure")
        X4N_RT(X4_ROOMTYPE_INTELLIGENCEOFFICE,"intelligenceoffice")
        X4N_RT(X4_ROOMTYPE_LIVINGROOM,        "livingroom")
        X4N_RT(X4_ROOMTYPE_MANAGER,           "manager")
        X4N_RT(X4_ROOMTYPE_OFFICE,            "office")
        X4N_RT(X4_ROOMTYPE_PLAYEROFFICE,      "playeroffice")
        X4N_RT(X4_ROOMTYPE_PRISON,            "prison")
        X4N_RT(X4_ROOMTYPE_SECURITY,          "security")
        X4N_RT(X4_ROOMTYPE_SERVERROOM,        "serverroom")
        X4N_RT(X4_ROOMTYPE_SERVICEROOM,       "serviceroom")
        X4N_RT(X4_ROOMTYPE_SHIPTRADERCORNER,  "shiptradercorner")
        X4N_RT(X4_ROOMTYPE_TRADERCORNER,      "tradercorner")
        X4N_RT(X4_ROOMTYPE_TRAFFICCONTROL,    "trafficcontrol")
        X4N_RT(X4_ROOMTYPE_WARROOM,           "warroom")
        default: return nullptr;
    }
    #undef X4N_RT
}

} // namespace x4n

