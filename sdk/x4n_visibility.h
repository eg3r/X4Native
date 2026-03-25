// ---------------------------------------------------------------------------
// x4n_visibility.h — Visibility System Reads
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::visibility::get_radar_visible()
//   x4n::visibility::get_forced_radar_visible()
//   x4n::visibility::is_map_visible()
//   x4n::visibility::get_known_to_all()
//   x4n::visibility::get_known_factions_count()
//   x4n::visibility::get_space_known_to_all()
//   x4n::visibility::get_space_known_factions_count()
//
// NOTE: Functions mix C FFI (stable) and raw memory reads (fragile).
// See @stability annotations on each function.
// All functions require on_game_loaded to have fired.
// See docs/rev/VISIBILITY.md for full system documentation.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_entity.h"

namespace x4n { namespace visibility {

/// Read the radar_visible byte (+0x400) directly from an Object-class entity.
/// This is the transient flag set/cleared by the game engine's radar scan.
/// Returns false if the entity pointer can't be resolved.
/// NOTE: Only valid for Object-class entities (stations, ships, satellites -- type 71).
///       Space-class entities (clusters, sectors, zones) have no radar byte.
/// @stability FRAGILE -- raw memory offset (0x400). Re-verify on game updates.
/// @verified v9.00 build 600626
inline bool get_radar_visible(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return false;
    return *reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_OBJECT_OFFSET_RADAR_VISIBLE) != 0;
}

/// Read the forced_radar_visible byte (+0x401) directly from an Object-class entity.
/// This is the persistent override set by SetObjectForcedRadarVisible (satellites, nav beacons).
/// Returns false if the entity pointer can't be resolved.
/// @stability FRAGILE -- raw memory offset (0x401). Re-verify on game updates.
/// @verified v9.00 build 600626
inline bool get_forced_radar_visible(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return false;
    return *reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_OBJECT_OFFSET_FORCED_RADAR_VISIBLE) != 0;
}

/// Check if an Object-class entity is currently visible on the map.
/// Map visibility requires BOTH isknown AND (isradarvisible OR forceradarvisible).
/// This matches menu_map.lua:7471 filter logic.
/// Uses C FFI IsObjectKnown for the known check, direct memory for radar.
/// Returns false if game API unavailable or entity not found.
/// @stability MIXED -- C FFI (stable) + raw memory offsets (fragile).
/// @verified v9.00 build 600626
inline bool is_map_visible(uint64_t id) {
    auto* g = game();
    if (!g || !g->IsObjectKnown) return false;
    if (!g->IsObjectKnown(id)) return false;
    // Radar check: either engine-set or forced
    void* comp = entity::find_component(id);
    if (!comp) return false;
    auto addr = reinterpret_cast<uintptr_t>(comp);
    uint8_t radar  = *reinterpret_cast<uint8_t*>(addr + X4_OBJECT_OFFSET_RADAR_VISIBLE);
    uint8_t forced = *reinterpret_cast<uint8_t*>(addr + X4_OBJECT_OFFSET_FORCED_RADAR_VISIBLE);
    return (radar != 0) || (forced != 0);
}

/// Read the known_to_all flag from an Object-class entity (+858).
/// When true, the entity is known to ALL factions unconditionally.
/// @stability FRAGILE -- raw memory offset (858). Re-verify on game updates.
/// @verified v9.00 build 600626
inline bool get_known_to_all(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return false;
    return *reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_OBJECT_OFFSET_KNOWN_TO_ALL) != 0;
}

/// Read the known_factions_count from an Object-class entity (+888).
/// Returns the number of factions that know about this entity.
/// @stability FRAGILE -- raw memory offset (888). Re-verify on game updates.
/// @verified v9.00 build 600626
inline size_t get_known_factions_count(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return 0;
    return *reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_OBJECT_OFFSET_KNOWN_FACTIONS_COUNT);
}

/// Read the known_to_all flag from a Space-class entity (+818).
/// Space-class: clusters (type 15), sectors (type 86), zones (type 107).
/// @stability FRAGILE -- raw memory offset (818). Re-verify on game updates.
/// @verified v9.00 build 600626
inline bool get_space_known_to_all(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return false;
    return *reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_SPACE_OFFSET_KNOWN_TO_ALL) != 0;
}

/// Read the known_factions_count from a Space-class entity (+848).
/// @stability FRAGILE -- raw memory offset (848). Re-verify on game updates.
/// @verified v9.00 build 600626
inline size_t get_space_known_factions_count(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return 0;
    return *reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_SPACE_OFFSET_KNOWN_FACTIONS_COUNT);
}

}} // namespace x4n::visibility
