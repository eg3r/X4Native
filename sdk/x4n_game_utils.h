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
    static void* s_comp_reg = nullptr;
    if (!s_comp_reg)
        s_comp_reg = *reinterpret_cast<void**>(exe_base() + X4_RVA_COMPONENT_REGISTRY);
    if (!s_comp_reg) return nullptr;
    return g->ComponentRegistry_Find(s_comp_reg, id, 4);
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

/// Compute FNV-1a hash of a lowercased string (engine convention).
/// Used by MacroRegistry, ConstructionPlanDB, and connection lookups.
inline uint64_t fnv1a_lower(const char* str) {
    uint64_t hash = 2166136261ULL;
    for (const char* p = str; *p; p++) {
        char c = (*p >= 'A' && *p <= 'Z') ? (*p + 32) : *p;
        hash = static_cast<int8_t>(c) ^ (16777619ULL * hash);
    }
    return hash;
}

/// Resolve a connection name string to its ConnectionEntry pointer within a macro.
/// macro_ptr must come from resolve_macro(). Returns nullptr if not found.
/// Uses FNV-1a hash + binary search on the macro's sorted connection array.
/// See docs/rev/CONSTRUCTION_PLANS.md for ConnectionEntry layout.
inline void* resolve_connection(void* macro_ptr, const char* connection_name) {
    if (!macro_ptr || !connection_name || !connection_name[0]) return nullptr;

    uint64_t hash = fnv1a_lower(connection_name);
    if (hash == 2166136261ULL) return nullptr;  // empty string hash = seed → invalid

    auto addr = reinterpret_cast<uintptr_t>(macro_ptr);
    auto begin = *reinterpret_cast<uintptr_t*>(addr + X4_MACRODATA_OFFSET_CONNECTIONS_BEGIN);
    auto end   = *reinterpret_cast<uintptr_t*>(addr + X4_MACRODATA_OFFSET_CONNECTIONS_END);
    if (!begin || begin >= end) return nullptr;

    // Binary search: entries sorted by hash at entry+8, stride 352 bytes
    // Note: engine stores hash as uint64 in the comparison despite FNV-1a producing 32-bit;
    // the upper 32 bits are the XOR overflow from 64-bit arithmetic.
    size_t count = (end - begin) / X4_CONNECTION_ENTRY_SIZE;
    size_t lo = 0, hi = count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        auto entry_addr = begin + mid * X4_CONNECTION_ENTRY_SIZE;
        auto entry_hash = *reinterpret_cast<uint64_t*>(entry_addr + X4_CONNECTION_OFFSET_HASH);
        if (entry_hash < hash)
            lo = mid + 1;
        else if (entry_hash > hash)
            hi = mid;
        else
            return reinterpret_cast<void*>(entry_addr);
    }
    return nullptr;
}

/// Resolve a macro name string to its internal MacroData pointer.
/// Returns nullptr if macro not found. silent=true suppresses game error logs.
/// Lowercases the name before lookup (engine convention).
/// Caches the registry pointer on first call. Only call after on_game_loaded.
inline void* resolve_macro(const char* macro_name, bool silent = true) {
    auto* g = game();
    if (!g || !g->MacroRegistry_Lookup) return nullptr;
    static uintptr_t s_macro_reg = 0;
    if (!s_macro_reg)
        s_macro_reg = *reinterpret_cast<uintptr_t*>(exe_base() + X4_RVA_MACRO_REGISTRY);
    if (!s_macro_reg) return nullptr;

    // Lowercase (engine uses lowercased FNV-1a keys)
    char lower[256];
    size_t len = 0;
    for (const char* p = macro_name; *p && len < sizeof(lower) - 1; p++, len++)
        lower[len] = (*p >= 'A' && *p <= 'Z') ? (*p + 32) : *p;
    lower[len] = 0;

    struct { const char* data; size_t length; } sv{ lower, len };
    return g->MacroRegistry_Lookup(reinterpret_cast<void*>(s_macro_reg), &sv, silent ? 1 : 0);
}

/// Get the construction plan registry pointer.
/// Caches on first call. Only call after on_game_loaded.
inline void* plan_registry() {
    static void* s_plan_reg = nullptr;
    if (!s_plan_reg)
        s_plan_reg = *reinterpret_cast<void**>(exe_base() + X4_RVA_CONSTRUCTION_PLAN_DB);
    return s_plan_reg;
}

/// Allocate a single game object via SMem pool. Returns typed pointer.
/// Use for objects that the game will later free (plan entries, events, etc.).
/// See docs/rev/MEMORY.md.
template<typename T>
T* game_alloc() {
    auto* g = game();
    if (!g || !g->GameAlloc) return nullptr;
    return static_cast<T*>(g->GameAlloc(sizeof(T), 0, 0, 0, 16));
}

/// Allocate a typed array via SMem pool. Returns pointer to first element.
template<typename T>
T* game_alloc_array(size_t count) {
    auto* g = game();
    if (!g || !g->GameAlloc) return nullptr;
    return static_cast<T*>(g->GameAlloc(count * sizeof(T), 0, 0, 0, 16));
}

/// Set the entry vector on an EditableConstructionPlan.
/// Copies entry pointers into a game-allocated array and wires up the
/// plan's internal vector (plan+184/192/200). See docs/rev/CONSTRUCTION_PLANS.md.
inline bool plan_set_entries(void* plan, X4PlanEntry** entries, size_t count) {
    if (!plan || !count) return false;

    auto* arr = game_alloc_array<X4PlanEntry*>(count);
    if (!arr) return false;

    for (size_t i = 0; i < count; i++)
        arr[i] = entries[i];

    auto addr = reinterpret_cast<uintptr_t>(plan);
    *reinterpret_cast<X4PlanEntry***>(addr + 184) = arr;
    *reinterpret_cast<X4PlanEntry***>(addr + 192) = arr + count;
    *reinterpret_cast<X4PlanEntry***>(addr + 200) = arr + count;
    return true;
}

} // namespace x4n

