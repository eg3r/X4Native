// ---------------------------------------------------------------------------
// x4n_entity.h — Entity Resolution & Component Properties
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::entity::find_component()      — resolve UniverseID to X4Component*
//   x4n::entity::resolve_entity()      — SEH-guarded raw pointer validation
//   x4n::entity::get_component_macro() — macro name (heavy components only)
//   x4n::entity::get_spawntime()       — Container-class spawntime
//
// For game_class() and is_a(), use the free functions:
//   x4n::entity::game_class(comp)               — returns GameClass enum
//   x4n::entity::is_a(comp, GameClass::Station)  — IS-A check via vtable
// For sector-specific properties (sunlight, resources), use x4n::sector::Sector.
//
// All functions require on_game_loaded to have fired.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include <cstdint>

namespace x4n { namespace entity {

/// Runtime class ID via vtable GetClassID slot.
/// Safe on all GameClass-registered types (ships, stations, sectors, etc.).
/// NOT safe on lightweight types like ResourceArea — crashes (no GameClass entry).
inline GameClass game_class(const X4Component* comp) {
    if (!comp || !comp->vtable) return static_cast<GameClass>(GAMECLASS_SENTINEL);
    using Fn = uint32_t(__fastcall*)(const void*);
    auto fn = reinterpret_cast<Fn*>(comp->vtable)[detail::offsets()->vtable_get_class_id];
    return static_cast<GameClass>(fn ? fn(comp) : GAMECLASS_SENTINEL);
}

/// IS-A check via vtable IsOrDerivedFromClassID slot.
inline bool is_a(const X4Component* comp, GameClass cls) {
    if (!comp || !comp->vtable) return false;
    using Fn = bool(__fastcall*)(const void*, uint32_t);
    auto fn = reinterpret_cast<Fn*>(comp->vtable)[detail::offsets()->vtable_is_derived_class];
    return fn ? fn(comp, static_cast<uint32_t>(cls)) : false;
}

/// Resolve a UniverseID to its X4Component* via the component registry.
/// Returns nullptr if the ID is invalid or the registry isn't initialized.
/// Reads the registry pointer from the global each call (the game may
/// reconstruct the registry on save/load, so caching would go stale).
/// Only call after on_game_loaded.
/// @stability MODERATE — depends on X4_RVA_COMPONENT_REGISTRY + ComponentRegistry_Find.
/// @verified v9.00 build 602526
inline X4Component* find_component(uint64_t id) {
    auto* g = game();
    if (!g || !g->ComponentRegistry_Find) return nullptr;
    auto* reg = *reinterpret_cast<X4ComponentRegistry**>(detail::offsets()->component_registry);
    if (!reg) return nullptr;
    return static_cast<X4Component*>(
        g->ComponentRegistry_Find(reg, id, 4));
}

/// Get the class name string for an entity (e.g. "station", "sector", "player").
/// Wraps the game's GetComponentClass export. Returns "" if unavailable.
inline const char* get_class_name(uint64_t id) {
    auto* g = game();
    if (!g || !g->GetComponentClass) return "";
    return g->GetComponentClass(id);
}

/// Resolve a raw uint64_t (from MD event fields) to a typed X4EntityBase*.
/// MD event params often contain raw component pointers stored as uint64_t.
/// SEH-guarded: validates the pointer is readable by probing ->id.
/// Returns nullptr if the value is null or not a valid entity pointer.
inline X4EntityBase* resolve_entity(uint64_t raw_ptr) {
    if (raw_ptr == 0) return nullptr;
    __try {
        auto* ent = reinterpret_cast<X4EntityBase*>(raw_ptr);
        (void)ent->id;  // probe — triggers SEH if invalid
        return ent;
    } __except(1) {
        return nullptr;
    }
}

/// Read the macro name string from a heavy component (sector, cluster, station, ship).
/// Uses the embedded interface at component+0x30, vtable slot 4 (GetMacroName).
/// Returns nullptr if the component is invalid or has no macro name.
/// The returned pointer is owned by the component's internal std::string — valid
/// as long as the component exists. Do NOT store across frames.
/// @note Only valid for heavy components (X4Component). NOT for X4ResourceArea.
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
        reinterpret_cast<uintptr_t>(comp) + detail::offsets()->container_spawntime);
}

/// Read the spawntime directly from a component pointer.
/// @see get_spawntime(uint64_t) for details.
inline double get_spawntime(const X4Component* comp) {
    if (!comp) return -1.0;
    return *reinterpret_cast<const double*>(
        reinterpret_cast<uintptr_t>(comp) + detail::offsets()->container_spawntime);
}

}} // namespace x4n::entity
