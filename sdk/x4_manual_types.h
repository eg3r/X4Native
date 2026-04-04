// ==========================================================================
// x4_manual_types.h - Hand-Authored Types (RE / Community)
// ==========================================================================
// Types not present in the game's FFI data surface. These come from reverse
// engineering, community contributions, or other sources.
//
// This file is NEVER overwritten by the generation pipeline.
//
// Guidelines:
//   - Prefix names with X4 to avoid collision with game-native types
//   - Note the game version each type was verified against
//   - Can depend on generated types (include x4_game_types.h first)
//
// Sections (match domain headers in x4n_*.h):
//   FRAMEWORK TYPES      — X4NativeFrameUpdate
//   ENTITY SYSTEM         — class IDs, component registry RVA, component offsets
//   SEED / HASH           — LCG constants, session seed RVA
//   WALKABLE INTERIORS    — X4RoomType enum, room property offsets
//   CONSTRUCTION PLANS    — MacroData offsets, ConnectionEntry layout, plan entry struct
//   VISIBILITY            — Object-class and Space-class visibility offsets
// ==========================================================================
#pragma once

#include "x4_game_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ======== FRAMEWORK TYPES ================================================
// X4NativeFrameUpdate — on_native_frame_update payload

// ---- Frame timing global RVAs ----
// Read by the native frame tick hook (core.cpp) to populate X4NativeFrameUpdate.
// These are accumulated timing counters inside X4_FrameTick's critical section.
// WARNING: data addresses change between builds. Re-verify on game updates.
// Verified: v9.00 build 603098 (X4_FrameTick @ 0x140F55670 decompilation)
#define X4_RVA_FRAME_GAME_TIME     0x06AF6368  /* double — game_time (scaled by speed multiplier) */
#define X4_RVA_FRAME_RAW_TIME      0x06AF6370  /* double — always-incrementing wall time */
#define X4_RVA_FRAME_REAL_TIME     0x06AF6378  /* double — unpaused real time */
#define X4_RVA_FRAME_SPEED_MULT    0x06AF6390  /* double — game speed multiplier (1.0/2.0/5.0/10.0) */

// ---- Engine context struct offsets (first param to X4_FrameTick) ----
// The engine context is an opaque struct passed to X4_FrameTick by the message pump.
// WARNING: struct offsets change between builds. Re-verify on game updates.
// Verified: v9.00 build 603098 (X4_FrameTick decompilation)
#define X4_ENGINECTX_OFFSET_FRAME_COUNTER  568  /* int32  — frame counter since last FPS sample */
#define X4_ENGINECTX_OFFSET_FPS_TIMER      576  /* double — timestamp of last FPS calculation */
#define X4_ENGINECTX_OFFSET_FPS            584  /* float  — current FPS (frames / elapsed) */

// ---- Native frame update event payload (on_native_frame_update) ----
// Passed as the void* data arg to event callbacks.
// Populated by core.cpp from the globals and engine context above.
// Verified: v9.00 build 603098 (see docs/rev/GAME_LOOP.md)
typedef struct X4NativeFrameUpdate {
    double  delta;          // Frame delta in seconds (capped at 1.0)
    double  game_time;      // Accumulated game time (with speed multiplier)
    double  real_time;      // Accumulated real time (paused = no accumulation)
    float   fps;            // Current FPS (from engine context + X4_ENGINECTX_OFFSET_FPS)
    float   speed_multiplier; // Game speed (1x, 2x, 5x, 10x)
    bool    game_paused;    // True if game is paused
    bool    is_suspended;   // True if window minimized / lost focus
    int     frame_counter;  // Frame counter since last FPS sample
} X4NativeFrameUpdate;

// ======== VISIBILITY EVENT PAYLOAD ========================================
// Fired as "on_radar_changed" event data when an entity enters or leaves
// gravidar range. Installed by the framework (core.cpp) via MinHook detour
// on RadarVisibilityChanged_BuildEvent.

typedef struct X4RadarChangedEvent {
    uint64_t entity_id;     // ComponentID of the affected entity
    uint8_t  visible;       // 1 = entered radar range, 0 = left radar range
} X4RadarChangedEvent;

// ======== ENTITY SYSTEM ==================================================
// Engine class IDs, component registry global, component data offsets.
// Consumed by x4n_entity.h (x4n::entity::find_component).

// ---- X4Component: Base type for all game entities ----
// Sectors, clusters, stations, ships, NPCs, zones all share this base layout.
// Subclasses extend with additional fields past the base region.
// IMPORTANT: Fields from class_id (+0x68) onward are only valid for Object-hierarchy
// entities (stations, ships, satellites, gates, etc.). Player and NPC entities do NOT
// have these fields at these offsets. Use vtable GetClassID (slot 566) to identify type.
// This is a READ-ONLY VIEW over game-engine-owned memory. Components are
// allocated by the game's ComponentFactory — never construct X4Component yourself.
// Obtain pointers via x4n::entity::find_component(id).
// Layout confirmed by decompiling 15+ functions (see docs/rev/COMPONENT_SYSTEM.md §2).
// Verified: v9.00 build 603098 (universal prefix), build 602526 (Object-specific fields)
// Embedded definition interface at X4Component+0x30.
// Has its own vtable. this ptr = &component->definition.
// vtable[3] = GetName(), vtable[4] = GetMacroName() — both return MSVC std::string*.
typedef struct X4DefinitionInterface {
    void** vtable;
#ifdef __cplusplus
    /// Call vtable[3] GetName() — returns raw std::string* (caller handles SSO).
    void* GetName()      { return reinterpret_cast<void*(*)(X4DefinitionInterface*)>(vtable[3])(this); }
    /// Call vtable[4] GetMacroName() — returns raw std::string* (caller handles SSO).
    void* GetMacroName() { return reinterpret_cast<void*(*)(X4DefinitionInterface*)>(vtable[4])(this); }
#endif
} X4DefinitionInterface;

typedef struct X4Component {
    void*     vtable;            // +0x00: main vtable (~800+ slots, see X4_VTABLE_* constants)
    uint64_t  id;                // +0x08: UniverseID (also raw generation seed — dual purpose)
    uint8_t   _pad_10[0x20];    // +0x10..+0x2F: internal engine bookkeeping (unresolved)
    X4DefinitionInterface definition; // +0x30: embedded sub-object (8 bytes — just the vtable ptr)
    void*     ctrl_vtable;       // +0x38: shared_ptr control block vtable
    int32_t   ref_count;         // +0x40: atomic reference count
    int32_t   weak_count;        // +0x44: atomic weak ref / lifecycle state (1->2->3)
    uint8_t   _pad_48[0x20];    // +0x48..+0x67: unresolved
    int32_t   class_id;          // +0x68: DEPRECATED — NOT the runtime class ID. Use vtable GetClassID (slot 566) instead.
    uint8_t   _pad_6C[0x04];    // +0x6C..+0x6F: padding
    void*     parent;            // +0x70: parent X4Component* (null for galaxy root)
    uint8_t   _pad_78[0x30];    // +0x78..+0xA7: unresolved (48 bytes)
    void*     children;          // +0xA8: child container ptr (group-indexed partition array, 32-byte buckets)
    uint8_t   _pad_B0[0x21];    // +0xB0..+0xD0: unresolved
    uint8_t   exists;            // +0xD1: existence flag (0=destroyed, nonzero=alive)
} X4Component;

#ifdef __cplusplus
static_assert(offsetof(X4Component, id)         == 0x08, "X4Component::id offset mismatch");
static_assert(offsetof(X4Component, definition) == 0x30, "X4Component::definition offset mismatch");
static_assert(offsetof(X4Component, ref_count)  == 0x40, "X4Component::ref_count offset mismatch");
static_assert(offsetof(X4Component, class_id)   == 0x68, "X4Component::class_id offset mismatch");
static_assert(offsetof(X4Component, parent)     == 0x70, "X4Component::parent offset mismatch");
static_assert(offsetof(X4Component, children)   == 0xA8, "X4Component::children offset mismatch");
static_assert(offsetof(X4Component, exists)     == 0xD1, "X4Component::exists offset mismatch");
#endif

// Component registry — opaque, accessed only via ComponentRegistry_Find.
typedef struct X4ComponentRegistry_ X4ComponentRegistry;

// ---- Engine class IDs (runtime numeric IDs) ----
// Auto-generated from class_ids.csv by generate_class_ids.ps1.
// Full table in x4_game_class_ids.inc. Use x4n::GameClass enum (e.g. x4n::GameClass::Station).
// WARNING: class IDs shift when new classes are added between game builds.
//
// Key hierarchy (build 603098):
//   component(21) > entity(43) > object(72)  — stations, ships, satellites, gates
//   component(21) > entity(43) > positional(76) > player(75)   — player character
//   component(21) > entity(43) > positional(76) > nonplayer(115) > npc(71) — NPCs
//   object(72) > container(110) > controllable(111) — ordered entities
//   space(117) > cluster(15), sector(87), zone(108) — spatial hierarchy
#include "x4_game_class_ids.inc"

// ---- Global data RVA: Component registry ----
// Add to imagebase to get absolute address. Dereference to get the actual value.
// WARNING: data address changes between builds. Re-verify on game updates.
// Verified: v9.00 build 603098 (confirmed via ComponentRegistry_Find callers)
#define X4_RVA_COMPONENT_REGISTRY       0x06C866C0  /* void** — g_ComponentRegistry */

// ---- Component base struct offsets ----
// IMPORTANT: Only offsets +0x00 through +0x44 are in the universal Component prefix.
// Offsets +0x68 and beyond are Object-hierarchy specific (stations, ships, satellites,
// gates, etc. — anything that IS-A "object"). They are NOT valid for Player, NPC,
// Positional, or other non-Object entities. Use vtable GetClassID (slot 566) to check.
// Layout confirmed by decompiling 15+ functions (see docs/rev/COMPONENT_SYSTEM.md §4).
// NOTE: struct offsets — update when game build changes.
// Verified: v9.00 build 603098 (universal prefix), build 602526 (Object-specific)
//
// --- Universal Component prefix (valid for ALL component types) ---
#define X4_COMPONENT_OFFSET_RAW_SEED       0x08   /* uint64 — raw generation seed (same field as ID) */
#define X4_COMPONENT_OFFSET_ID             0x08   /* uint64 — UniverseID (same field as RAW_SEED) */
#define X4_COMPONENT_OFFSET_DEFINITION     0x30   /* void*  — embedded DefinitionInterface vtable ptr */
                                                   /*          vtable[3] = GetName() -> std::string*   */
                                                   /*          vtable[4] = GetMacroName() -> std::string* */
#define X4_COMPONENT_OFFSET_CTRL_VTABLE    0x38   /* void*  — shared_ptr control block vtable */
#define X4_COMPONENT_OFFSET_REF_COUNT      0x40   /* int32  — atomic reference count */
#define X4_COMPONENT_OFFSET_WEAK_COUNT     0x44   /* int32  — atomic weak ref / lifecycle state (1->2->3) */
//
// --- Object-hierarchy only (NOT valid for Player, NPC, Positional-only entities) ---
#define X4_COMPONENT_OFFSET_CLASS_ID       0x68   /* int32  — DEPRECATED: NOT the runtime class ID.           */
                                                   /*          GetClassID is a vtable function (slot 566)        */
                                                   /*          returning a hardcoded constant per class.         */
                                                   /*          This field's actual purpose is unconfirmed.       */
#define X4_COMPONENT_OFFSET_PARENT         0x70   /* void*  — parent X4Component* (null for galaxy root) */
                                                   /*          parent->+0x08 = parent UniverseID           */
#define X4_COMPONENT_OFFSET_CHILDREN       0xA8   /* void*  — child container ptr (group-indexed array, see below) */
#define X4_COMPONENT_OFFSET_EXISTS         0xD1   /* uint8  — existence flag (0=destroyed, nonzero=alive) */
                                                   /*          confirmed by GetSectors_Lua: cmp [rax+0D1h],0 */
#define X4_COMPONENT_OFFSET_COMBINED_SEED  0x3C0  /* int64  — raw_seed + session_seed (= MD $Station.seed) */

// ---- Container class offsets (stations, ships — NOT space suits) ----
// Container is the base class for entities that hold other entities.
// Stations and ships inherit these offsets from Container.
// WARNING: SpaceSuit stores spawntime at a different offset (0xC88).
// Verified: v9.00 build 603098 (Container_GetSpawnTime @ 0x140B19D30,
//   Container load/save @ 0x1403B1FB0/0x1403B4210, save property 0x44C)
#define X4_CONTAINER_OFFSET_SPAWNTIME      0x6E0  /* double — game time when created/connected to universe */
                                                   /*          sentinel -1.0 = not set.                      */
                                                   /*          MD: $container.spawntime (class index 0x44C)   */
                                                   /*          MD: $container.age = gametime - spawntime      */

// ---- Component main vtable slot offsets (byte offsets into vtable) ----
// The main vtable at +0x00 has ~800+ slots. Key slots:
// WARNING: vtable shifted +8 (one slot inserted) in build 603098 vs 602526.
// All offsets from GetClassID onward increased by 8.
// Verified: v9.00 build 603098 (GetComponentClass decompilation: vtable+4528 for GetClassID,
//   AddCluster: vtable+4536 for IsClassID, SetObjectRadarVisible: vtable+4544 for IsDerived)
#define X4_VTABLE_GET_CLASS_TYPE      136   /* slot 17:  GetClassType() -> uint */
#define X4_VTABLE_GET_CLASS_ID       4528   /* slot 566: GetClassID() -> uint (120=sentinel) */
#define X4_VTABLE_IS_CLASS_ID        4536   /* slot 567: IsClassID(classid) -> bool */
#define X4_VTABLE_IS_DERIVED_CLASS   4544   /* slot 568: IsOrDerivedFromClassID(classid) -> bool */
#define X4_VTABLE_GET_ID_CODE        4760   /* slot 595: GetIDCode() -> std::string* */
#define X4_VTABLE_SET_WORLD_XFORM    5144   /* slot 643: SetWorldTransform(...) */
#define X4_VTABLE_SET_POSITION       5184   /* slot 648: SetPosition(transform*) */
#define X4_VTABLE_DESTROY            5408   /* slot 676: Destroy(reason, flags) */
#define X4_VTABLE_GET_FACTION_ID     5608   /* slot 701: GetFactionID() -> int */

// ---- Child container internal layout (at COMPONENT_OFFSET_CHILDREN) ----
// The child container is a GROUP-INDEXED PARTITION ARRAY (NOT a hash map).
// component+0xA8 is a POINTER to a container object:
//   container+0x08: bucket_array_begin  (pointer to 32-byte bucket array)
//   container+0x10: bucket_array_end
//   container+0x20: total_child_count   (DWORD)
// Each bucket (32 bytes): {child_ptr_begin, child_ptr_end, capacity_end, count}.
// Buckets are indexed by GROUP INDEX (not class ID). Multiple class IDs share
// the same group (e.g., clusters and sectors both use group 2).
// Callers apply vtable-based IsClassID post-filters for exact class matching.
// Use ChildComponent_Enumerate (0x1402F9B80) to iterate; do not walk manually.
// Key internal addresses (for reference, not for direct use):
//   ChildComponent_Enumerate:       0x1402F9B80 (61 callers)
//   ChildComponent_Iterator_Init:   0x1402FF740
//   ChildComponent_Iterator_Next:   0x1402F9AA0
//   ChildComponent_GetBucketCount:  0x1402E5120

// ---- Component ID decomposition (ComponentRegistry_Find @ 0x1400CE890) ----
// UniverseID layout: bits 0-24 = slot index (1-based), bits 25-40 = generation counter.
// Registry has up to 32 pages, ~1M entries per page, 3 entries packed per 32-byte block.
// Third param to ComponentRegistry_Find is class mask (4 = general component lookup).

// ---- Galaxy global ----
// WARNING: data address changes between builds. Re-verify on game updates.
// Verified: v9.00 build 603098 (AddCluster at 0x14013DAD0)
#define X4_RVA_GAME_UNIVERSE            0x03CAEE68  /* void** — g_GameUniverse */
#define X4_GAME_UNIVERSE_GALAXY_OFFSET  552         /* *(g_GameUniverse + 552) = galaxy component ptr */

// ======== SEED / HASH CONSTANTS ==========================================
// LCG formula and session seed global.
// Consumed by x4n_math.h (x4n::math::advance_seed).

// ---- Seed system constants (see docs/rev/WALKABLE_INTERIORS.md §16) ----
// LCG formula: next = ROR64(seed * multiplier + addend, 30)
// These are algorithm constants embedded in code, not data references.
// Likely stable across builds (PRNG design, not tunable), but verify on major engine changes.
// Found inside MD_EvalSeed_AutoAdvance (0x140C10590 in build 602526).
// Verified: v9.00 build 603098 (algorithm constants, stable across builds)
#define X4_SEED_LCG_MULTIPLIER  0x5851F42D4C957F2DULL
#define X4_SEED_LCG_ADDEND     0x14057B7EF767814FULL
#define X4_SEED_LCG_ROTATE     30

// ---- Global data RVA: Session seed ----
// WARNING: data address changes between builds. Re-verify on game updates.
// Verified: v9.00 build 603098
#define X4_RVA_SESSION_SEED             0x03CAEDD0  /* uint64* — g_SessionSeed */

// ======== WALKABLE INTERIORS =============================================
// Room type enum and room property offsets.
// Consumed by x4n_rooms.h (x4n::rooms::roomtype_name).

// ---- RoomType enum (see docs/rev/WALKABLE_INTERIORS.md §17) ----
// Enum init at 0x1407521A0, data table at 0x1424794A0, 22 entries.
// Verified: v9.00 build 602526
typedef enum X4RoomType {
    X4_ROOMTYPE_BAR               = 0,
    X4_ROOMTYPE_CASINO            = 1,
    X4_ROOMTYPE_CORRIDOR          = 2,
    X4_ROOMTYPE_CREWQUARTERS      = 3,
    X4_ROOMTYPE_EMBASSY           = 4,
    X4_ROOMTYPE_FACTIONREP        = 5,
    X4_ROOMTYPE_GENERATORROOM     = 6,
    X4_ROOMTYPE_INFRASTRUCTURE    = 7,
    X4_ROOMTYPE_INTELLIGENCEOFFICE = 8,
    X4_ROOMTYPE_LIVINGROOM        = 9,
    X4_ROOMTYPE_MANAGER           = 10,
    X4_ROOMTYPE_OFFICE            = 11,
    X4_ROOMTYPE_PLAYEROFFICE      = 12,
    X4_ROOMTYPE_PRISON            = 13,
    X4_ROOMTYPE_SECURITY          = 14,
    X4_ROOMTYPE_SERVERROOM        = 15,
    X4_ROOMTYPE_SERVICEROOM       = 16,
    X4_ROOMTYPE_SHIPTRADERCORNER  = 17,
    X4_ROOMTYPE_TRADERCORNER      = 18,
    X4_ROOMTYPE_TRAFFICCONTROL    = 19,
    X4_ROOMTYPE_WARROOM           = 20,
    X4_ROOMTYPE_NONE              = 21,
} X4RoomType;

// ---- Room property offsets within Room entity (class 82) ----
// NOTE: These are raw struct offsets — update when game build changes. Any field
// added/removed/reordered in the Room class will shift these. Re-verify on
// every game update. Currently only used for documentation; the hook approach
// reads these via the game's own code paths, not direct memory access.
// Verified: v9.00 build 602526 (see docs/rev/WALKABLE_INTERIORS.md §22)
#define X4_ROOM_OFFSET_ROOMTYPE    0x2C0  /* int32  — X4RoomType enum */
#define X4_ROOM_OFFSET_UNK_3A0     0x3A0  /* uint8  — purpose unknown, set by CreateDynamicInterior */
#define X4_ROOM_OFFSET_NAME        0x3A8  /* string — room name (std::string) */
#define X4_ROOM_OFFSET_PRIVATE     0x408  /* uint8  — private flag */
#define X4_ROOM_OFFSET_PERSISTENT  0x409  /* uint8  — persistent flag */

// ======== CONSTRUCTION PLANS =============================================
// MacroData offsets, ConnectionEntry layout, plan entry struct, plan/macro registry RVAs.
// Consumed by x4n_plans.h (x4n::plans::resolve_macro, resolve_connection, plan_set_entries).

// ---- Global data RVAs: Plan and macro registries ----
// WARNING: data addresses change between builds. Re-verify on game updates.
// Verified: v9.00 build 603098 (ImportMapConstructionPlan, AddCluster callers)
#define X4_RVA_CONSTRUCTION_PLAN_DB     0x06C86950  /* void** — g_ConstructionPlanRegistry (RB-tree at +16) */
#define X4_RVA_MACRO_REGISTRY           0x06C867B8  /* void*  — g_MacroRegistry (BST at +64) */

// ---- MacroData field offsets ----
// Returned by MacroRegistry_Lookup. Connection array is sorted by FNV-1a hash.
// WARNING: struct offsets — fragile across builds. Re-verify on game updates.
// Verified: v9.00 build 602526
#define X4_MACRODATA_OFFSET_CONNECTIONS_BEGIN  0x170  /* void* — start of ConnectionEntry array */
#define X4_MACRODATA_OFFSET_CONNECTIONS_END    0x178  /* void* — end of ConnectionEntry array */

// ---- ConnectionEntry layout ----
// Each entry is 352 bytes (0x160 stride). Sorted by FNV-1a hash at +8.
// Name string at +16 is std::string (MSVC SSO: inline if len<16, heap ptr if >=16).
// Confirmed by GetNumPlannedStationModules (0x14019ce00) which reads ConnectionEntry+16
// to populate UIConstructionPlanEntry.connectionid.
// Verified: v9.00 build 602526
#define X4_CONNECTION_ENTRY_SIZE    0x160  /* 352 bytes */
#define X4_CONNECTION_OFFSET_HASH   0x08   /* uint32 — FNV-1a hash of lowercased name */
#define X4_CONNECTION_OFFSET_NAME   0x10   /* std::string — connection name (e.g. "connection_room01") */

// ---- Dynamic Interior door selection ----
// Controllable::CreateDynamicInterior (0x140415F40) selects a door connection from
// the corridor macro's "room" class MacroDefaults. The connection pointer array is at
// MacroDefaults offset +1112 (begin) / +1120 (end).
// Door selection algorithm when door param is NULL:
//   if (seed != 0): index = seeded_random(&seed, count)   -- deterministic LCG
//   if (seed == 0): index = tls_random(count)              -- unpredictable
// seeded_random (0x1414839F0): next = ROR64(seed*0x5851F42D4C957F2D+0x14057B7EF767814F, 30)
// This is identical to x4n::advance_seed(). Standard rooms (npc_instantiation) always
// use seed = station.seed + roomtype_index, so door selection is deterministic.
// After door selection, a SECOND seeded_random call selects which station window to
// attach the corridor to.
// Rooms created with seed=0 (playeroffice, embassy) use TLS random -- non-reproducible.
//
// The doors= output of MD get_room_definition returns the same ordered connection list.
// To replay door selection: advance_seed(station_seed + roomtype_index) % doors.count.
#define X4_MACRODEFAULTS_OFFSET_ROOM_CONNECTIONS_BEGIN  0x458  /* void** — ConnectionEntry* array begin */
#define X4_MACRODEFAULTS_OFFSET_ROOM_CONNECTIONS_END    0x460  /* void** — ConnectionEntry* array end */

// ---- Construction plan entry (528 bytes) ----
// Internal plan entry used by the station construction system.
// Allocate via GameAlloc, init via PlanEntry_Construct (0x140D10960).
// Transform layout: position (__m128) + 3x3 rotation matrix (3x __m128, row-major).
//   +48: [pos_x, pos_y, pos_z, 0.0]       -- position relative to station origin
//   +64: [r0_x,  r0_y,  r0_z,  0.0]       -- rotation matrix row 0
//   +80: [r1_x,  r1_y,  r1_z,  0.0]       -- rotation matrix row 1
//   +96: [r2_x,  r2_y,  r2_z,  0.0]       -- rotation matrix row 2
// Identity rotation = {1,0,0,0}, {0,1,0,0}, {0,0,1,0}.
// All-zeros rotation is INVALID — always set at least identity.
//
// Euler angle convention (UIConstructionPlanEntry.offset <-> rotation matrix):
//   Extract:  yaw   = atan2(-r2[0], r2[2]) * (-180/pi)
//             pitch = asin(clamp(r2[1])) * (180/pi)
//             roll  = atan2(-r0[1], r1[1]) * (180/pi)
//   Inject:   y = -yaw*(pi/180), p = pitch*(pi/180), r = roll*(pi/180)
//             row0 = { cy*cr+sy*sp*sr, -cy*sr+sy*sp*cr, sy*cp, 0 }
//             row1 = { cp*sr,           cp*cr,          -sp,   0 }
//             row2 = { -sy*cr+cy*sp*sr, sy*sr+cy*sp*cr, cy*cp, 0 }
//
// Predecessor chain: predecessor ptr links to another X4PlanEntry in the same
// plan. During spawn, Station_InitFromPlan (0x140488120) uses entry->id stored
// at module_entity+848 to find the predecessor module via
// Station_FindModuleByPlanEntryID (0x140489B20), then Entity_EstablishConnection
// (0x140399580) links the connection points bidirectionally.
// Entry IDs only need to be unique within the plan (auto-assigned from atomic
// counter at 0x14388ACE0 if id==0 on construct).
//
// See docs/rev/CONSTRUCTION_PLANS.md for full documentation.
// NOTE: struct layout — update when game build changes.
// Verified: v9.00 build 603098 (PlanEntry_Construct @ 0x140D10960 decompiled,
//   528-byte allocation confirmed at call sites, all field offsets matched)
typedef struct alignas(16) X4PlanEntry {
    int64_t   id;                   // +0:   unique ID (auto-assigned from atomic counter if 0)
    void*     macro_ptr;            // +8:   MacroData* (from MacroRegistry_Lookup)
    void*     connection_ptr;       // +16:  ConnectionEntry* on THIS module (nullptr = auto/root)
    void*     predecessor;          // +24:  X4PlanEntry* predecessor (nullptr = root module)
    void*     pred_connection_ptr;  // +32:  ConnectionEntry* on PREDECESSOR module (nullptr = auto)
    uint8_t   pad_40[8];           // +40:  padding (observed zero)
    float     pos_x;               // +48:  position X relative to station origin
    float     pos_y;               // +52:  position Y
    float     pos_z;               // +56:  position Z
    float     pos_w;               // +60:  padding (typically 0.0)
    float     rot_row0[4];         // +64:  rotation matrix row 0 [r0x, r0y, r0z, 0]
    float     rot_row1[4];         // +80:  rotation matrix row 1 [r1x, r1y, r1z, 0]
    float     rot_row2[4];         // +96:  rotation matrix row 2 [r2x, r2y, r2z, 0]
    uint8_t   loadout[408];        // +112: equipment loadout (init by sub_1400EF140)
    uint8_t   is_fixed;            // +520: fixed/immovable flag
    uint8_t   is_modified;         // +521: modified flag
    uint8_t   is_bookmark;         // +522: bookmark flag
    uint8_t   pad_end[5];          // +523: padding to 528 bytes (16-byte aligned)
} X4PlanEntry;
#define X4_PLAN_ENTRY_SIZE  sizeof(X4PlanEntry)  /* 528 */

// ======== VISIBILITY =====================================================
// Object-class and Space-class visibility offsets.
// Consumed by x4n_visibility.h (x4n::visibility::get_radar_visible, etc.).

// ---- Object-Class Visibility Offsets (type 71: stations, ships, satellites) ----
// Full layout documented in docs/rev/VISIBILITY.md Section 9.
// Verified: v9.00 build 602526
#define X4_OBJECT_OFFSET_OWNER_FACTION_PTR      840    /* void* — owner faction context pointer */
#define X4_OBJECT_OFFSET_KNOWN_READ             857    /* uint8 — encyclopedia "read" flag */
#define X4_OBJECT_OFFSET_KNOWN_TO_ALL           858    /* uint8 — global known flag (rarely set) */
#define X4_OBJECT_OFFSET_KNOWN_FACTIONS_ARR     864    /* 16 bytes — SSO faction pointer array (inline if cap<=2, heap ptr if >2) */
#define X4_OBJECT_OFFSET_KNOWN_FACTIONS_CAP     880    /* size_t — array capacity (2 = inline SSO) */
#define X4_OBJECT_OFFSET_KNOWN_FACTIONS_COUNT   888    /* size_t — number of factions in known-to list */
#define X4_OBJECT_OFFSET_LIVEVIEW_LOCAL         0x3C8  /* uint8 — local gravidar visibility (set when entity scanned in player's zone) */
#define X4_OBJECT_OFFSET_LIVEVIEW_MONITOR       0x3C9  /* uint8 — remote monitor visibility (set when entity visible via remote observation) */
#define X4_OBJECT_OFFSET_MASSTRAFFIC_QUEUE      0x3E0  /* ptr   — mass traffic queue object (null if not in mass traffic) */
#define X4_OBJECT_OFFSET_RADAR_VISIBLE          0x400  /* uint8 — radar visibility, set by engine property system + MD action */
#define X4_OBJECT_OFFSET_FORCED_RADAR_VISIBLE   0x401  /* uint8 — forced radar visibility (satellites, nav beacons) */

// ---- RadarVisibilityChangedEvent Layout ----
// Dispatched by the engine when radar visibility changes on an entity.
// Three dispatchers: SetForcedRadarVisible_Internal, SetObjectRadarVisible_Action,
// and the engine property change handler (case 378 in sector update pipeline).
// Verified: v9.00 build 603098 (confirmed via SetObjectRadarVisible_Action LEA)
#define X4_RADAR_EVENT_VTABLE_RVA           0x02B4A408  /* const U::RadarVisibilityChangedEvent::`vftable' */
#define X4_RADAR_EVENT_OFFSET_ENTITY_ID     24          /* uint64 — ComponentID of affected entity */
#define X4_RADAR_EVENT_OFFSET_VISIBLE       32          /* uint8  — new visibility state (0=left range, 1=entered range) */

// ---- Space-Class Visibility Offsets (type 15/86/107: clusters, sectors, zones) ----
// Different offsets from Object-class. No radar bytes (Space entities use known-to only).
// Full layout documented in docs/rev/VISIBILITY.md Section 9.
// Verified: v9.00 build 602526
#define X4_SPACE_OFFSET_OWNER_FACTION_PTR       800    /* void* — owner faction context pointer */
#define X4_SPACE_OFFSET_KNOWN_READ              817    /* uint8 — encyclopedia "read" flag */
#define X4_SPACE_OFFSET_KNOWN_TO_ALL            818    /* uint8 — global known flag */
#define X4_SPACE_OFFSET_KNOWN_FACTIONS_ARR      824    /* 16 bytes — SSO faction pointer array */
#define X4_SPACE_OFFSET_KNOWN_FACTIONS_CAP      840    /* size_t — array capacity */
#define X4_SPACE_OFFSET_KNOWN_FACTIONS_COUNT    848    /* size_t — number of factions in known-to list */

#ifdef __cplusplus
}
#endif
