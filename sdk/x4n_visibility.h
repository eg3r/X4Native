// ---------------------------------------------------------------------------
// x4n_visibility.h — Visibility System Reads
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::visibility::get_radar_visible()      — read +0x400 byte
//   x4n::visibility::get_forced_radar_visible() — read +0x401 byte
//   x4n::visibility::set_radar_visible()      — write +0x400 byte directly
//   x4n::visibility::is_map_visible()         — IsObjectKnown AND (radar OR forced)
//   x4n::visibility::get_known_to_all()       — read known_to_all flag
//   x4n::visibility::get_known_factions_count() — read faction count
//   x4n::visibility::get_space_known_to_all() — Space-class known_to_all
//   x4n::visibility::get_space_known_factions_count() — Space-class faction count
//   x4n::visibility::radar_event_entity()     — read entity ID from RadarVisibilityChangedEvent
//   x4n::visibility::radar_event_visible()    — read visible bool from RadarVisibilityChangedEvent
//
// Event subscription (via standard event system, NOT this header):
//   x4n::on("on_radar_changed", [](const X4RadarChangedEvent* e) { ... });
//   x4n::off(id);
//
// Functions mix C FFI exports and raw memory reads.
// Raw offsets are defined in x4_manual_types.h and updated per game build.
// All functions require on_game_loaded to have fired.
// See docs/rev/VISIBILITY.md for full system documentation.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_entity.h"

namespace x4n { namespace visibility {

/// Read the radar_visible byte (+0x400) directly from an Object-class entity.
/// This byte is SET by the game engine's radar scan when an entity enters range,
/// and by MD `set_object_radar_visible`. It is NOT cleared when the entity leaves
/// radar range -- the byte persists once set. Map visibility is controlled by the
/// C++ holomap renderer (live gravidar checks), not by this byte alone.
/// Returns false if the entity pointer can't be resolved.
/// NOTE: Only valid for Object-class entities (stations, ships, satellites -- type 71).
///       Space-class entities (clusters, sectors, zones) have no radar byte.
/// @stability raw memory offset — X4Native handles version updates (0x400). Re-verify on game updates.
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
/// @stability raw memory offset — X4Native handles version updates (0x401). Re-verify on game updates.
/// @verified v9.00 build 600626
inline bool get_forced_radar_visible(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return false;
    return *reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_OBJECT_OFFSET_FORCED_RADAR_VISIBLE) != 0;
}

/// Check if an Object-class entity passes the Lua sidebar visibility filter.
/// Checks BOTH isknown AND (isradarvisible OR forceradarvisible).
/// This matches menu_map.lua:7471 filter logic for the sidebar object list.
/// NOTE: This is a SECONDARY filter. The PRIMARY map visibility gate is the
/// C++ holomap renderer which uses live gravidar proximity checks. An entity
/// can pass this check but still not appear on the map if the holomap renderer
/// has excluded it (e.g., entity left radar range but +1024 byte persisted).
/// Uses C FFI IsObjectKnown for the known check, direct memory for radar.
/// Returns false if game API unavailable or entity not found.
/// @stability C FFI + raw memory offsets — X4Native handles version updates.
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
/// @stability raw memory offset — X4Native handles version updates (858). Re-verify on game updates.
/// @verified v9.00 build 600626
inline bool get_known_to_all(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return false;
    return *reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_OBJECT_OFFSET_KNOWN_TO_ALL) != 0;
}

/// Read the known_factions_count from an Object-class entity (+888).
/// Returns the number of factions that know about this entity.
/// @stability raw memory offset — X4Native handles version updates (888). Re-verify on game updates.
/// @verified v9.00 build 600626
inline size_t get_known_factions_count(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return 0;
    return *reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_OBJECT_OFFSET_KNOWN_FACTIONS_COUNT);
}

/// Read the known_to_all flag from a Space-class entity (+818).
/// Space-class: clusters (type 15), sectors (type 86), zones (type 107).
/// @stability raw memory offset — X4Native handles version updates (818). Re-verify on game updates.
/// @verified v9.00 build 600626
inline bool get_space_known_to_all(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return false;
    return *reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_SPACE_OFFSET_KNOWN_TO_ALL) != 0;
}

/// Read the known_factions_count from a Space-class entity (+848).
/// @stability raw memory offset — X4Native handles version updates (848). Re-verify on game updates.
/// @verified v9.00 build 600626
inline size_t get_space_known_factions_count(uint64_t id) {
    void* comp = entity::find_component(id);
    if (!comp) return 0;
    return *reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_SPACE_OFFSET_KNOWN_FACTIONS_COUNT);
}

// ---------------------------------------------------------------------------
// Radar visibility write — set the +0x400 byte directly on a component.
// Use for testing or when the engine's property system is not appropriate.
// Does NOT dispatch RadarVisibilityChangedEvent (use SetObjectRadarVisible MD action for that).
// @verified v9.00 build 600626
inline void set_radar_visible(uint64_t id, bool visible) {
    void* comp = entity::find_component(id);
    if (!comp) return;
    *reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(comp) + X4_OBJECT_OFFSET_RADAR_VISIBLE) = visible ? 1 : 0;
}

// ---------------------------------------------------------------------------
// RadarVisibilityChangedEvent subscription.
//
// Subscribe to radar visibility transitions (entity entering/leaving range).
// Fires from the engine's property change system during sector updates.
// Does NOT fire for SetObjectForcedRadarVisible changes (different path).
//
// Subscribe via the standard event system (NOT this header):
//   int id = x4n::on("on_radar_changed", [](const X4RadarChangedEvent* e) {
//       x4n::log::info("radar: %llu -> %d", e->entity_id, (int)e->visible);
//   });
//   x4n::off(id);
//
// @verified v9.00 build 600626
// ---------------------------------------------------------------------------

/// Extract the entity ComponentID from a RadarVisibilityChangedEvent object.
inline uint64_t radar_event_entity(void* event) {
    return *reinterpret_cast<uint64_t*>(
        reinterpret_cast<uintptr_t>(event) + X4_RADAR_EVENT_OFFSET_ENTITY_ID);
}

/// Extract the new visibility state from a RadarVisibilityChangedEvent object.
/// true = entity entered radar range, false = entity left radar range.
inline bool radar_event_visible(void* event) {
    return *reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(event) + X4_RADAR_EVENT_OFFSET_VISIBLE) != 0;
}

}} // namespace x4n::visibility
