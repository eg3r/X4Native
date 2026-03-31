// ---------------------------------------------------------------------------
// x4n_entity.h — Entity Resolution & Component Properties
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::entity::find_component()      — resolve UniverseID to Object*
//   x4n::entity::get_class()        — runtime class ID via vtable (all types)
//   x4n::entity::is_derived_from()     — IS-A class check via vtable (all types)
//   x4n::entity::get_component_macro() — read macro name from any component
//   x4n::entity::get_component_id()    — read UniverseID from component pointer
//   x4n::entity::get_spawntime()       — Container-class spawntime
//
// Class IDs are auto-generated per game build via the pipeline (x4_game_class_ids.inc).
// Use x4n::GameClass enum with get_class() / is_derived_from().
//
// All functions require on_game_loaded to have fired.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include <cstdint>

namespace x4n { namespace entity {

/// Resolve a UniverseID to its X4Component* via the component registry.
/// Returns nullptr if the ID is invalid or the registry isn't initialized.
/// Caches the registry pointer on first call. Only call after on_game_loaded.
/// @stability MODERATE — depends on X4_RVA_COMPONENT_REGISTRY + ComponentRegistry_Find.
/// @verified v9.00 build 602526
inline X4Component* find_component(uint64_t id) {
    auto* g = game();
    if (!g || !g->ComponentRegistry_Find) return nullptr;
    static X4ComponentRegistry* s_reg = nullptr;
    if (!s_reg)
        s_reg = *reinterpret_cast<X4ComponentRegistry**>(exe_base() + X4_RVA_COMPONENT_REGISTRY);
    if (!s_reg) return nullptr;
    return static_cast<X4Component*>(
        g->ComponentRegistry_Find(s_reg, id, 4));
}

/// Get the runtime class ID of a component via its vtable (GetGameClass, slot 566).
/// Works for ALL component types (player, NPC, object, space, etc.).
/// Returns static_cast<GameClass>(GAMECLASS_SENTINEL) if the component is null or invalid.
/// @note Do NOT use comp->class_id (+0x68) -- that field is NOT the class ID.
///       GetGameClass is a virtual function returning a hardcoded constant per class.
/// @stability STABLE -- vtable-based, no raw offsets.
/// @verified v9.00 build 603098 (GetComponentClass decompilation)
inline GameClass get_class(const X4Component* comp) {
    if (!comp || !comp->vtable) return static_cast<GameClass>(GAMECLASS_SENTINEL);
    using Fn = uint32_t(__fastcall*)(const void*);
    auto fn = reinterpret_cast<Fn*>(comp->vtable)[X4_VTABLE_GET_CLASS_ID / 8];
    return fn ? static_cast<GameClass>(fn(comp)) : static_cast<GameClass>(GAMECLASS_SENTINEL);
}

/// Convenience overload: get class ID by UniverseID.
inline GameClass get_class(uint64_t id) {
    return get_class(find_component(id));
}

/// Check if a component IS-A given class via vtable (IsOrDerivedFromGameClass, slot 568).
/// Works for ALL component types. Use GameClass enum values.
/// @stability STABLE -- vtable-based, no raw offsets.
/// @verified v9.00 build 603098 (SetObjectRadarVisible, AddCluster decompilation)
inline bool is_derived_from(const X4Component* comp, GameClass cls) {
    if (!comp || !comp->vtable) return false;
    using Fn = bool(__fastcall*)(const void*, uint32_t);
    auto fn = reinterpret_cast<Fn*>(comp->vtable)[X4_VTABLE_IS_DERIVED_CLASS / 8];
    return fn ? fn(comp, static_cast<uint32_t>(cls)) : false;
}

/// Convenience overload: IS-A check by UniverseID.
inline bool is_derived_from(uint64_t id, GameClass cls) {
    return is_derived_from(find_component(id), cls);
}

/// Get the class name string for an entity (e.g. "station", "sector", "player").
/// Wraps the game's GetComponentClass export. Returns "" if unavailable.
inline const char* get_class_name(uint64_t id) {
    auto* g = game();
    if (!g || !g->GetComponentClass) return "";
    return g->GetComponentClass(id);
}

/// Read a component's UniverseID from its object pointer.
/// @stability MODERATE — depends on X4_COMPONENT_OFFSET_ID (+0x08).
/// @verified v9.00 build 602526 (GetClusters_Lua, GetSectors_Lua)
inline uint64_t get_component_id(const X4Component* component) {
    if (!component) return 0;
    return component->id;
}

/// Read the macro name string from any component (sector, cluster, station, ship).
/// Uses the embedded interface at component+0x30, vtable slot 4 (GetMacroName).
/// Returns nullptr if the component is invalid or has no macro name.
/// The returned pointer is owned by the component's internal std::string — valid
/// as long as the component exists. Do NOT store across frames.
/// @note Assumes MSVC x64 std::string layout (SSO threshold = 16 bytes).
/// @stability MODERATE — depends on X4_COMPONENT_OFFSET_DEFINITION (+0x30) + vtable[4].
/// @verified v9.00 build 602526 (GetComponentData "macro" handler at 0x1402461CC)
#ifdef _MSC_VER
inline const char* get_component_macro(X4Component* component) {
    if (!component || !component->definition.vtable) return nullptr;
    auto* str = reinterpret_cast<uint64_t*>(component->definition.GetMacroName());
    if (!str) return nullptr;
    // MSVC x64 std::string SSO: data inline at str[0..1] if capacity (str[3]) < 16.
    return (str[3] < 16)
        ? reinterpret_cast<const char*>(str)
        : reinterpret_cast<const char*>(str[0]);
}
#endif

/// Convenience overload: read macro name by UniverseID.
#ifdef _MSC_VER
inline const char* get_component_macro(uint64_t id) {
    return get_component_macro(find_component(id));
}
#endif

/// Read the spawntime from a Container-class entity (station, ship).
/// Returns the game time (seconds since game start) when the object was
/// created or connected to the universe.
/// Returns -1.0 if the component is null or spawntime is unset.
/// Only valid for Container-derived entities (stations, ships).
/// @note SpaceSuit stores this at a different offset (0xC88) — do not use for suits.
/// @stability LOW — raw struct offset, verify on game updates.
/// @verified v9.00 build 900 (Container_GetSpawnTime @ 0x140B19D30)
inline double get_spawntime(uint64_t id) {
    auto* comp = find_component(id);
    if (!comp) return -1.0;
    return *reinterpret_cast<double*>(
        reinterpret_cast<uintptr_t>(comp) + X4_CONTAINER_OFFSET_SPAWNTIME);
}

/// Read the spawntime directly from a component pointer.
/// @see get_spawntime(uint64_t) for details.
inline double get_spawntime(const X4Component* comp) {
    if (!comp) return -1.0;
    return *reinterpret_cast<const double*>(
        reinterpret_cast<uintptr_t>(comp) + X4_CONTAINER_OFFSET_SPAWNTIME);
}

}} // namespace x4n::entity
