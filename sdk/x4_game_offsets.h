// ==========================================================================
// x4_game_offsets.h — Runtime-Resolved Game Offsets
// ==========================================================================
// Defines X4GameOffsets: a struct of all version-dependent values that the
// X4Native core populates at startup from its compiled #define constants.
//
// Extensions read from this struct via x4n::offsets()
// instead of using compile-time #define constants. This means extensions
// compile once and work with any X4Native core version — only the core
// DLL needs rebuilding per game update.
//
// The struct has three sections:
//   1. Pre-resolved pointers  — exe_base + RVA, ready to dereference
//   2. Vtable slot indices    — pre-divided by 8, ready for vtable[slot]
//   3. Struct field offsets   — raw byte offsets into game objects
//
// NOT included (never change between builds):
//   - Algorithm constants (X4_SEED_LCG_*, X4_ORDER_PARAM_TYPE_*)
//   - Enum values (X4RoomType)
// ==========================================================================
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct X4GameOffsets {
    // === Pre-resolved pointers (exe_base + RVA, computed once at startup) ===
    // Extensions dereference these directly — no arithmetic needed.
    double*   frame_game_time;          // X4_RVA_FRAME_GAME_TIME
    double*   frame_raw_time;           // X4_RVA_FRAME_RAW_TIME
    double*   frame_real_time;          // X4_RVA_FRAME_REAL_TIME
    double*   frame_speed_mult;         // X4_RVA_FRAME_SPEED_MULT
    void*     component_registry;       // X4_RVA_COMPONENT_REGISTRY (ptr to registry ptr)
    void*     faction_registry;         // X4_RVA_FACTION_REGISTRY (ptr to registry; tree at +16, root at +24)
    void*     game_universe;            // X4_RVA_GAME_UNIVERSE (ptr to universe ptr)
    uint64_t* session_seed;             // X4_RVA_SESSION_SEED
    void*     construction_plan_db;     // X4_RVA_CONSTRUCTION_PLAN_DB (ptr to plan DB ptr)
    void*     macro_registry;           // X4_RVA_MACRO_REGISTRY (ptr to macro registry ptr)
    void*     radar_event_vtable;       // X4_RADAR_EVENT_VTABLE_RVA

    // === Vtable slot indices (byte_offset / 8, ready for vtable[slot]) ===
    uint32_t  vtable_get_class_type;    // X4_VTABLE_GET_CLASS_TYPE / 8
    uint32_t  vtable_get_class_id;      // X4_VTABLE_GET_CLASS_ID / 8
    uint32_t  vtable_is_class_id;       // X4_VTABLE_IS_CLASS_ID / 8
    uint32_t  vtable_is_derived_class;  // X4_VTABLE_IS_DERIVED_CLASS / 8
    uint32_t  vtable_get_id_code;       // X4_VTABLE_GET_ID_CODE / 8
    uint32_t  vtable_set_world_xform;   // X4_VTABLE_SET_WORLD_XFORM / 8
    uint32_t  vtable_set_position;      // X4_VTABLE_SET_POSITION / 8
    uint32_t  vtable_destroy;           // X4_VTABLE_DESTROY / 8
    uint32_t  vtable_get_faction_id;    // X4_VTABLE_GET_FACTION_ID / 8

    // === Engine context offsets ===
    uint32_t  enginectx_frame_counter;  // X4_ENGINECTX_OFFSET_FRAME_COUNTER
    uint32_t  enginectx_fps_timer;      // X4_ENGINECTX_OFFSET_FPS_TIMER
    uint32_t  enginectx_fps;            // X4_ENGINECTX_OFFSET_FPS

    // === Component struct offsets ===
    uint32_t  component_id;             // X4_COMPONENT_OFFSET_ID
    uint32_t  component_definition;     // X4_COMPONENT_OFFSET_DEFINITION
    uint32_t  component_parent;         // X4_COMPONENT_OFFSET_PARENT
    uint32_t  component_children;       // X4_COMPONENT_OFFSET_CHILDREN
    uint32_t  component_exists;         // X4_COMPONENT_OFFSET_EXISTS
    uint32_t  component_combined_seed;  // X4_COMPONENT_OFFSET_COMBINED_SEED

    // === Container offsets ===
    uint32_t  container_spawntime;      // X4_CONTAINER_OFFSET_SPAWNTIME

    // === Space offsets ===
    uint32_t  space_has_sunlight;       // X4_SPACE_OFFSET_HAS_SUNLIGHT
    uint32_t  space_sunlight;           // X4_SPACE_OFFSET_SUNLIGHT

    // === Game universe ===
    uint32_t  game_universe_galaxy_offset; // X4_GAME_UNIVERSE_GALAXY_OFFSET

    // === Object-class visibility offsets ===
    uint32_t  object_owner_faction_ptr;    // X4_OBJECT_OFFSET_OWNER_FACTION_PTR
    uint32_t  object_known_read;           // X4_OBJECT_OFFSET_KNOWN_READ
    uint32_t  object_known_to_all;         // X4_OBJECT_OFFSET_KNOWN_TO_ALL
    uint32_t  object_known_factions_arr;   // X4_OBJECT_OFFSET_KNOWN_FACTIONS_ARR
    uint32_t  object_known_factions_cap;   // X4_OBJECT_OFFSET_KNOWN_FACTIONS_CAP
    uint32_t  object_known_factions_count; // X4_OBJECT_OFFSET_KNOWN_FACTIONS_COUNT
    uint32_t  object_liveview_local;       // X4_OBJECT_OFFSET_LIVEVIEW_LOCAL
    uint32_t  object_liveview_monitor;     // X4_OBJECT_OFFSET_LIVEVIEW_MONITOR
    uint32_t  object_masstraffic_queue;    // X4_OBJECT_OFFSET_MASSTRAFFIC_QUEUE
    uint32_t  object_radar_visible;        // X4_OBJECT_OFFSET_RADAR_VISIBLE
    uint32_t  object_forced_radar_visible; // X4_OBJECT_OFFSET_FORCED_RADAR_VISIBLE

    // === Space-class visibility offsets ===
    uint32_t  space_owner_faction_ptr;     // X4_SPACE_OFFSET_OWNER_FACTION_PTR
    uint32_t  space_known_read;            // X4_SPACE_OFFSET_KNOWN_READ
    uint32_t  space_known_to_all;          // X4_SPACE_OFFSET_KNOWN_TO_ALL
    uint32_t  space_known_factions_arr;    // X4_SPACE_OFFSET_KNOWN_FACTIONS_ARR
    uint32_t  space_known_factions_cap;    // X4_SPACE_OFFSET_KNOWN_FACTIONS_CAP
    uint32_t  space_known_factions_count;  // X4_SPACE_OFFSET_KNOWN_FACTIONS_COUNT

    // === Sector resource area offsets ===
    uint32_t  sector_resarea_vec_begin;    // X4_SECTOR_RESAREA_VEC_BEGIN
    uint32_t  sector_resarea_vec_end;      // X4_SECTOR_RESAREA_VEC_END

    // === MacroData / ConnectionEntry offsets ===
    uint32_t  macrodata_connections_begin; // X4_MACRODATA_OFFSET_CONNECTIONS_BEGIN
    uint32_t  macrodata_connections_end;   // X4_MACRODATA_OFFSET_CONNECTIONS_END
    uint32_t  connection_entry_size;       // X4_CONNECTION_ENTRY_SIZE
    uint32_t  connection_offset_hash;      // X4_CONNECTION_OFFSET_HASH
    uint32_t  connection_offset_name;      // X4_CONNECTION_OFFSET_NAME
    uint32_t  macrodefaults_room_conn_begin; // X4_MACRODEFAULTS_OFFSET_ROOM_CONNECTIONS_BEGIN
    uint32_t  macrodefaults_room_conn_end;   // X4_MACRODEFAULTS_OFFSET_ROOM_CONNECTIONS_END

    // === Radar event layout offsets ===
    uint32_t  radar_event_entity_id;       // X4_RADAR_EVENT_OFFSET_ENTITY_ID
    uint32_t  radar_event_visible;         // X4_RADAR_EVENT_OFFSET_VISIBLE

    // === Room offsets ===
    uint32_t  room_roomtype;               // X4_ROOM_OFFSET_ROOMTYPE
    uint32_t  room_name;                   // X4_ROOM_OFFSET_NAME
    uint32_t  room_private;                // X4_ROOM_OFFSET_PRIVATE
    uint32_t  room_persistent;             // X4_ROOM_OFFSET_PERSISTENT
} X4GameOffsets;

#ifdef __cplusplus
} // extern "C"
#endif
