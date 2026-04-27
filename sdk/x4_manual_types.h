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
// FIND: Decompile X4_FrameTick (internal_functions.json). In else-branch (a2==0), find
//   EnterCriticalSection block. GAME_TIME=first xmmword (+=delta*speed), RAW_TIME=high qword,
//   REAL_TIME=second xmmword (!IsGamePaused_0 guard), SPEED_MULT=the qword multiplier.
// Verified: build 606138
#define X4_RVA_FRAME_GAME_TIME     0x06AF02A8  /* double — game_time (scaled by speed multiplier) */
#define X4_RVA_FRAME_RAW_TIME      0x06AF02B0  /* double — always-incrementing wall time */
#define X4_RVA_FRAME_REAL_TIME     0x06AF02B8  /* double — unpaused real time */
#define X4_RVA_FRAME_SPEED_MULT    0x06AF02D0  /* double — game speed multiplier (1.0/2.0/5.0/10.0) */

// FIND: Same X4_FrameTick decompile — a1+568 frame_counter, a1+576 fps_timer, a1+584 fps.
// Verified: build 606138 (unchanged from 603098)
#define X4_ENGINECTX_OFFSET_FRAME_COUNTER  568  /* int32  — frame counter since last FPS sample */
#define X4_ENGINECTX_OFFSET_FPS_TIMER      576  /* double — timestamp of last FPS calculation */
#define X4_ENGINECTX_OFFSET_FPS            584  /* float  — current FPS (frames / elapsed) */

// ---- Native frame update event payload (on_native_frame_update) ----
// Passed as the void* data arg to event callbacks.
// Populated by core.cpp from the globals and engine context above.
// Verified: build 606138 (see docs/rev/GAME_LOOP.md)
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
// have these fields at these offsets. Use vtable GetClassID (slot 567) to identify type.
// This is a READ-ONLY VIEW over game-engine-owned memory. Components are
// allocated by the game's ComponentFactory — never construct X4Component yourself.
// Obtain pointers via x4n::entity::find_component(id).
// Layout confirmed by decompiling 15+ functions (see docs/rev/COMPONENT_SYSTEM.md §2).
// Verified: build 606138 (universal prefix + Object-specific fields)
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

// --- X4EntityBase ---
// Minimal base shared by ALL game entities (ships, stations, sectors,
// resource areas, etc.). Verified in-game across Sector, Ship, ResourceArea.
// Heavier types (X4Component, X4ResourceArea) inherit from this.
//
// @verified v9.00 build 606138 (in-game probe: Sector/Ship/ResourceArea)
struct X4EntityBase {
    void*      vtable;          // +0x00: main vtable (~800+ slots, see X4_VTABLE_* constants)
    UniverseID id;              // +0x08: UniverseID (also raw generation seed — dual purpose)
    void*      _unk_10;         // +0x10: null for Sector/ResourceArea, heap ptr for Ship. Purpose unknown.
    void*      _type_meta;      // +0x18: per-type constant (.text ptr). Same for all instances of same class. Factory/dispatch related.
    void*      _unk_20;         // +0x20: instance-specific ptr (null for ResourceArea). Purpose unknown.
    void*      _unk_28;         // +0x28: instance-specific ptr (null for ResourceArea). Purpose unknown.
};
static_assert(offsetof(X4EntityBase, vtable) == 0x00, "X4EntityBase vtable offset");
static_assert(offsetof(X4EntityBase, id)     == 0x08, "X4EntityBase id offset");

// --- X4Component ---
// Full ("heavy") component layout verified on ships, stations, sectors.
// Fields at +0x30 onward are only valid for these heavy entity types.
// Lighter entities (ResourceArea, etc.) share the X4EntityBase prefix
// but diverge after +0x10 — do NOT cast them to X4Component for field access.
struct X4Component : X4EntityBase {
    X4DefinitionInterface definition; // +0x30: embedded sub-object (8 bytes — just the vtable ptr)
    void*     ctrl_vtable;       // +0x38: shared_ptr control block vtable
    int32_t   ref_count;         // +0x40: atomic reference count
    int32_t   weak_count;        // +0x44: atomic weak ref / lifecycle state (1->2->3)
    uint8_t   _pad_48[0x20];    // +0x48..+0x67: unresolved
    int32_t   class_id;          // +0x68: DEPRECATED — NOT the runtime class ID. Use game_class() instead.
    uint8_t   _pad_6C[0x04];    // +0x6C..+0x6F: padding
    void*     parent;            // +0x70: parent X4Component* (null for galaxy root)
    uint8_t   _pad_78[0x30];    // +0x78..+0xA7: unresolved (48 bytes)
    void*     children;          // +0xA8: child container ptr (group-indexed partition array, 32-byte buckets)
    uint8_t   _pad_B0[0x21];    // +0xB0..+0xD0: unresolved
    uint8_t   exists;            // +0xD1: existence flag (0=destroyed, nonzero=alive)

    // For game_class() and is_a(), use x4n::entity::game_class(comp) and
    // x4n::entity::is_a(comp, cls) from x4n_entity.h. These require the
    // runtime offsets which are only available through the SDK API layer.
};
static_assert(offsetof(X4Component, id)         == 0x08, "X4Component::id offset mismatch");
static_assert(offsetof(X4Component, definition) == 0x30, "X4Component::definition offset mismatch");
static_assert(offsetof(X4Component, ref_count)  == 0x40, "X4Component::ref_count offset mismatch");
static_assert(offsetof(X4Component, class_id)   == 0x68, "X4Component::class_id offset mismatch");
static_assert(offsetof(X4Component, parent)     == 0x70, "X4Component::parent offset mismatch");
static_assert(offsetof(X4Component, children)   == 0xA8, "X4Component::children offset mismatch");
static_assert(offsetof(X4Component, exists)     == 0xD1, "X4Component::exists offset mismatch");

// ---- Production (class 78) field offsets ----
// Production-specific offsets. NOT present on X4Component base (shared prefix),
// NOT present on Processingmodule (class 77 uses +0x3B8 for its pause byte with no timestamp).
// WARNING: struct offsets — update when game build changes.
// Verified: build 606138. See docs/rev/PRODUCTION_MODULES.md §5.3.
// Use x4n::module::Module::is_paused() / paused_since() — do NOT read these offsets
// directly in consumer code (per project "no raw offsets in consumers" rule).
//
// FIND (paused_since @ +0x398):
//   PauseProductionModule @ 0x14017CB10 dispatches to its pause handler (sub_140726E50).
//   Inside the handler, look for `movsd [rcx+0x398], xmm1` — the instruction that stamps
//   player.age into the timestamp slot. Resume handler (sub_140727140) inverts: reads
//   *[rbx+0x398], adds elapsed to cumulative at +0x3A0, then writes the -1.0 sentinel
//   (0xBFF0000000000000) back to +0x398.
#define X4_PRODUCTION_PAUSED_SINCE_OFFSET            0x398  /* double, player.age at pause; -1.0 = not paused (sentinel) */

// FIND (cumulative_paused_time @ +0x3A0):
//   Resume handler sub_140727140. After reading +0x398, it computes elapsed = player.age
//   - *[rbx+0x398] and adds to *(double*)(this+0x3A0) before zeroing the sentinel.
//   The `addsd [rbx+0x3A0], xmm0` pattern is the cumulative counter update.
#define X4_PRODUCTION_CUMULATIVE_PAUSED_TIME_OFFSET  0x3A0  /* double, accumulated paused seconds across all cycles */

// FIND (recipe_index @ +0x3A8, state @ +0x3F0):
//   GetContainerWareProduction @ 0x1401AA3E0 module-iteration loop. Look for
//   `state = *(int32*)(module + 1008)` (= +0x3F0) and `*(int32*)(module + 936) != -1`
//   (= +0x3A8). Both appear in the "skip uninitialized / require recipe" guard before
//   accumulating the per-ware rate. See PRODUCTION_MODULES.md §6.1.
#define X4_PRODUCTION_RECIPE_INDEX_OFFSET            0x3A8  /* int32, -1 = no recipe */
#define X4_PRODUCTION_STATE_OFFSET                   0x3F0  /* int32, 1=starving/uninitialized, 8=active */

// FIND (paused_flag @ +0x3F4):
//   PauseProductionModule direct body (before dispatch to sub_140726E50). The byte write
//   `mov byte ptr [v5+0x3F4], 1` flips the flag. Also written (to 0) by
//   the resume path near the sentinel reset in sub_140727140.
#define X4_PRODUCTION_PAUSED_FLAG_OFFSET             0x3F4  /* uint8, redundant with sentinel above */

// FIND (processingmodule paused_flag @ +0x3B8):
//   Start from the `PauseProcessingModule` FFI entry (x4_game_func_list.inc:1526) — its
//   implementation tailcalls into the pause helper sub_14053CF60 (resume helper is
//   sub_14053D020 at the sibling call site). Pause handler writes
//   `byte ptr [this+0x3B8], 1`; resume handler clears it. No timestamp field — the
//   resume helper does NOT read/write any adjacent double, so the "paused since"
//   concept doesn't apply to Processingmodule.
#define X4_PROCESSINGMODULE_PAUSED_FLAG_OFFSET       0x3B8  /* uint8, class 77 only — no timestamp field */

// ---- Buildprocessor class offsets (class 10) ----
// Buildprocessor is a sub-entity attached to a Construction Vehicle (buildership)
// while it actively works on a BuildTask. It holds the runtime state of the current
// build phase — started_at timestamp, pause timestamp, stage counter, total stages.
// When the CV disengages (abandonbuildtime elapses, or build completes/aborts), the
// buildprocessor's started_at resets to the -1.0 sentinel.
//
// FIND (build_started_at @ +0x258):
//   GetBuildProcessorEstimatedTimeLeft @ 0x1401507C0 dispatches into
//   sub_14034FE60. Inside, look for `if ( *(double *)(a1 + 600) <= -0.9999 )` — the
//   sentinel check. The non-sentinel branch calls sub_14034FFD0 which reads
//   `result = *(double *)(a1 + 600);` and subtracts pause_at (+0x260) if set.
//   The stage-math helper uses the same field for stage math.
//   Set when the CV's buildprocessor begins work on a task; reset to -1.0 when
//   the CV abandons / completes.
// Verified: build 606138
#define X4_BUILDPROCESSOR_STARTED_AT_OFFSET          0x258  /* double, player.age at current phase start; -1.0 = not processing */

// FIND (build_paused_at @ +0x260):
//   sub_14034FFD0 (same function as above). After reading +0x258 as `result`, it
//   reads `v4 = *(double *)(a1 + 608);` and if `v4 > -0.9999` returns
//   `result + player.age - v4` — i.e. the pause-corrected start time. Same -1.0
//   sentinel idiom as Production's paused_since.
// Verified: build 606138
#define X4_BUILDPROCESSOR_PAUSED_AT_OFFSET           0x260  /* double, player.age at pause; -1.0 = not paused */

// FIND (build stage counters @ +0x268 / +0x26C):
//   GetCurrentBuildProgress body sub_14034FE60. Top-of-function guard reads
//   `if ( !*(_DWORD *)(a1 + 616) || !*(_DWORD *)(a1 + 620) ) return 0.0;` — the
//   current-stage (+0x268) and total-stages (+0x26C) counters. Final expression
//   `(stage_helper_return + (float)((current - 1) * 100.0)) / total` yields % done.
// Verified: build 606138
#define X4_BUILDPROCESSOR_CURRENT_STAGE_OFFSET       0x268  /* int32, 1-indexed current build stage */
#define X4_BUILDPROCESSOR_TOTAL_STAGES_OFFSET        0x26C  /* int32, total stages for this build */

// Component registry — opaque, accessed only via ComponentRegistry_Find.
typedef struct X4ComponentRegistry_ X4ComponentRegistry;

// --- X4FactionClass ---
// Engine-side per-faction runtime object. Lifetime = game session (one
// instance per faction; resolved by id-string via the FactionRegistry RBTree
// — see x4n::faction::find_class_by_id).
//
// Only the +0x280 field is reverse-engineered so far (RelationData pointer
// consumed by FactionRelation_GetFloat). Remaining bytes are opaque padding;
// extend as more fields are RE'd. Never embed this type by value — only use
// pointers, since the engine's full struct size is still TBD.
//
// @verified v9.00 build 606138 (RelationData offset confirmed by IDA at
// FactionRelation_GetFloat RVA 0x0030F700: `mov rcx, [rcx+280h]`)
#ifdef __cplusplus
struct X4FactionClass {
    uint8_t _opaque_prefix[0x280]; // +0x000..+0x27F: unresolved (faction header)
    void*   relation_data;          // +0x280 (=+640): RelationData* — boost map
                                   //   at +0x50 (boosts), base map at +0x10
                                   //   (RelationDataBase). Consumed by
                                   //   FactionRelation_GetFloat.
    // ... remaining fields not yet reverse-engineered.
};
static_assert(offsetof(X4FactionClass, relation_data) == 0x280,
    "X4FactionClass::relation_data offset mismatch");
#else
typedef struct X4FactionClass_ X4FactionClass;
#endif

// ---- Global data RVA: Component registry ----
// Add to imagebase to get absolute address. Dereference to get the actual value.
// WARNING: data address changes between builds. Re-verify on game updates.
// FIND: Any caller of ComponentRegistry_Find — first param (rcx) loaded via MOV rcx,[rip+disp].
// Verified: build 606138 (3 consistent callers)
#define X4_RVA_COMPONENT_REGISTRY       0x06C80600  /* void** — g_ComponentRegistry */

// ---- Global data RVA: Faction registry ----
// Singleton holding all FactionClass pointers, keyed by id-string FNV-1 hash.
// THE RVA HOLDS A POINTER SLOT (NOT THE STRUCT ITSELF) — same convention as
// X4_RVA_COMPONENT_REGISTRY. Consumers must deref once to get the struct base.
//
// Layout verified 2026-04-27 by IDA disassembly of SetFactionRelationToPlayerFaction
// (build 606138, RVA 0x0017F8D0): `mov rdx, cs:qword_146C80848` (explicit memory
// load = pointer slot), `add rdx, 10h` → registry+16 = sentinel,
// `mov rax, [rdx+8]` → registry+24 = root, `cmp [rax+20h], r11` → node key
// is full uint64 at +0x20, `lea rdi, [rcx+30h]` → FactionClass* at node+0x30.
//
// Layout (offsets are relative to the dereferenced registry struct):
//   +0x10  sentinel / end-of-tree node (struct member, not a pointer)
//   +0x18  RBTree root pointer (qword*)
//
// Each node:
//   +0x08  LEFT child  (qword*)
//   +0x10  RIGHT child (qword*)
//   +0x20  KEY  — FNV-1 hash of faction id, stored as uint64 (high 32 bits zero)
//   +0x30  VALUE — FactionClass* (FactionClass+0x280 = RelationData*, whose
//          +0x10 = RelationDataBase consumed by FactionRelation_GetFloat)
//
// FIND: Decompile any inlined faction-by-name lookup
// (SetFactionRelationToPlayerFaction or GetFactionRelationStatus2). The first
// immediate referenced via `MOV reg, [qword_<addr>]` (NOT `LEA`) is the slot.
// Verified: build 606138 — see docs/rev/FACTION_RELATIONS.md §2.2.
#define X4_RVA_FACTION_REGISTRY         0x06C80848  /* void** — pointer slot for g_FactionRegistry */
#define X4_FACTION_REGISTRY_TREE_BASE   0x10        /* registry+16 — sentinel / end-of-tree */
#define X4_FACTION_REGISTRY_TREE_ROOT   0x18        /* registry+24 — root node pointer */
#define X4_FACTION_REGISTRY_NODE_LEFT   0x08        /* node+8  — left child  */
#define X4_FACTION_REGISTRY_NODE_RIGHT  0x10        /* node+16 — right child */
#define X4_FACTION_REGISTRY_NODE_KEY    0x20        /* node+32 — uint64 FNV-1 hash key */
#define X4_FACTION_REGISTRY_NODE_VALUE  0x30        /* node+48 — FactionClass* */

// ---- Component base struct offsets ----
// IMPORTANT: Only offsets +0x00 through +0x44 are in the universal Component prefix.
// Offsets +0x68 and beyond are Object-hierarchy specific (stations, ships, satellites,
// gates, etc. — anything that IS-A "object"). They are NOT valid for Player, NPC,
// Positional, or other non-Object entities. Use vtable GetClassID (slot 567) to check.
// Layout confirmed by decompiling 15+ functions (see docs/rev/COMPONENT_SYSTEM.md §4).
// NOTE: struct offsets — update when game build changes.
// Verified: build 606138 (universal prefix + Object-specific)
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
                                                   /*          GetClassID is a vtable function (slot 567)        */
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
// Verified: build 606138 (spawntime @ 0x6E0 inferred from layout; price_factor shifted -0x10
//   from 0x7A0 to 0x790 in 606138 — re-verify spawntime if MD $container.spawntime misbehaves)
#define X4_CONTAINER_OFFSET_SPAWNTIME      0x6E0  /* double — game time when created/connected to universe */
                                                   /*          sentinel -1.0 = not set.                      */
                                                   /*          MD: $container.spawntime (class index 0x44C)   */
                                                   /*          MD: $container.age = gametime - spawntime      */

// FIND (price_factor @ +0x790):
//   SetContainerGlobalPriceFactor @ 0x1401B7440 decompile. After the IsClassID(110)
//   guard and the player-faction comparison (*vtable+5640), the clamp+write chain
//   ends in `v4[484] = result` where v4 is `_DWORD*` → 484*4 = 1936 = 0x790.
//   Clamp logic in the FFI: if arg < 0 → writes -1.0f sentinel (restore default);
//   else writes max(min(arg, 1.0), 0.0). Direct writes should use the SAME range.
//   Engine reads this field when computing trade offers (trade.find.free.xml →
//   find_sell_offer / find_buy_offer → container price table). The gate is
//   player-owned-only — direct-field writes bypass it (same pattern as
//   X4_PRODUCTION_PAUSED_SINCE_OFFSET for reads on NPC modules).
// Verified: build 606138 (shifted -0x10 from 0x7A0 in 605025)
#define X4_CONTAINER_PRICE_FACTOR_OFFSET   0x790  /* float — global price multiplier, clamp [0.0, 1.0].    */
                                                   /*          Engine sentinel -1.0 = no override (default). */

// ---- Space-class offsets (clusters, sectors, zones — IS-A Space(117)) ----
// Sunlight is stored on the CLUSTER (parent of sector), not on the sector.
// The game's Lua property handler walks sector→parent(+0x70) up to the first
// Space entity (= Cluster), then reads these offsets.
// FIND: sub_1407B51D0 (sunlight getter): while(!(entity+0x360)) walk parent.
//   Reads double at entity+0x368 when flag is set.
// @verified v9.00 build 606138 (offsets carry over; sunlight getter address drifts)
#define X4_SPACE_OFFSET_HAS_SUNLIGHT       0x360  /* uint8 — flag: sunlight value is set (not inherited) */
#define X4_SPACE_OFFSET_SUNLIGHT           0x368  /* double — sunlight intensity (0.0=dark, ~1.0=standard, up to ~3.7) */

// ---- Component main vtable slot offsets (byte offsets into vtable) ----
// The main vtable at +0x00 has ~800+ slots. Key slots:
// WARNING: vtable shifted +8 (one slot inserted) in build 603098 vs 602526.
// All offsets from GetClassID onward increased by 8.
// WARNING: vtable shifted +8 (one slot inserted before ClassID) in build 605025 vs 604402.
// WARNING: vtable shifted +8 (another slot inserted before ClassID) in build 606138 vs 605025.
// Verified: build 606138 (GetComponentClass @ 0x140151840: vtable+4544 for GetClassID,
//   AddCluster @ 0x14013DAD0: vtable+4552 for IsClassID,
//   SetObjectRadarVisible_Action @ 0x140B92620: vtable+4560 for IsDerived,
//   PauseProductionModule @ 0x14017CB10: vtable+5640 for player-faction getter)
#define X4_VTABLE_GET_CLASS_TYPE      136   /* slot 17:  GetClassType() -> uint */
#define X4_VTABLE_GET_CLASS_ID       4544   /* slot 568: GetClassID() -> uint (120=sentinel) */
#define X4_VTABLE_IS_CLASS_ID        4552   /* slot 569: IsClassID(classid) -> bool */
#define X4_VTABLE_IS_DERIVED_CLASS   4560   /* slot 570: IsOrDerivedFromClassID(classid) -> bool */
#define X4_VTABLE_GET_ID_CODE        4776   /* slot 597: GetIDCode() -> std::string* */
#define X4_VTABLE_SET_WORLD_XFORM    5160   /* slot 645: SetWorldTransform(...) */
#define X4_VTABLE_SET_POSITION       5200   /* slot 650: SetPosition(transform*) */
#define X4_VTABLE_DESTROY            5424   /* slot 678: Destroy(reason, flags) */
#define X4_VTABLE_GET_FACTION_ID     5624   /* slot 703: GetFactionID() -> int */

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
// Use ChildComponent_Enumerate to iterate; do not walk manually.
// Key internal addresses (for reference, not for direct use — addresses from build 603098, may drift):
//   ChildComponent_Enumerate:       0x1402F9B80 (61 callers)
//   ChildComponent_Iterator_Init:   0x1402FF740
//   ChildComponent_Iterator_Next:   0x1402F9AA0
//   ChildComponent_GetBucketCount:  0x1402E5120

// ---- Component ID decomposition (ComponentRegistry_Find @ 0x1400CE890, build 606138) ----
// UniverseID layout: bits 0-24 = slot index (1-based), bits 25-40 = generation counter.
// Registry has up to 32 pages, ~1M entries per page, 3 entries packed per 32-byte block.
// Third param to ComponentRegistry_Find is class mask (4 = general component lookup).

// ---- Galaxy global ----
// WARNING: data address changes between builds. Re-verify on game updates.
// FIND: Decompile GetPlayerID (PE export). First [rip+disp] load in .data/0x03CA range.
// Verified: build 606138 (GetPlayerID reads +560=player ptr; AddCluster reads +552=galaxy ptr)
#define X4_RVA_GAME_UNIVERSE            0x03CADE48  /* void** — g_GameUniverse */
#define X4_GAME_UNIVERSE_GALAXY_OFFSET  552         /* *(g_GameUniverse + 552) = galaxy component ptr */

// ======== SEED / HASH CONSTANTS ==========================================
// LCG formula and session seed global.
// Consumed by x4n_math.h (x4n::math::advance_seed).

// ---- Seed system constants (see docs/rev/WALKABLE_INTERIORS.md §16) ----
// LCG formula: next = ROR64(seed * multiplier + addend, 30)
// These are algorithm constants embedded in code, not data references.
// Likely stable across builds (PRNG design, not tunable), but verify on major engine changes.
// Found inside MD_EvalSeed_AutoAdvance (0x140C10590 in build 602526).
// Verified: build 606138 (algorithm constants, stable across builds)
#define X4_SEED_LCG_MULTIPLIER  0x5851F42D4C957F2DULL
#define X4_SEED_LCG_ADDEND     0x14057B7EF767814FULL
#define X4_SEED_LCG_ROTATE     30

// ---- Global data RVA: Session seed ----
// WARNING: data address changes between builds. Re-verify on game updates.
// FIND: Located 0x98 bytes before GAME_UNIVERSE (consistent across builds).
//   Alt: callers of Component_GetCombinedSeed that XOR with a global.
// Verified: build 606138 (predicted from GAME_UNIVERSE - 0x98)
#define X4_RVA_SESSION_SEED             0x03CADDB0  /* uint64* — g_SessionSeed */

// ======== WALKABLE INTERIORS =============================================
// Room type enum and room property offsets.
// Consumed by x4n_rooms.h (x4n::rooms::roomtype_name).

// ---- RoomType enum (see docs/rev/WALKABLE_INTERIORS.md §17) ----
// FIND: Enum init near 0x140752xxx, data table near 0x142479xxx, 22 entries.
// Verified: build 606138 (enum values unchanged, addresses may drift)
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
// Verified: build 606138
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
// FIND: PLAN_DB: decompile ExportMapConstructionPlan (PE export), scan for [rip+disp] in .bss/0x06C8.
//   MACRO_REG: decompile MacroRegistry_Lookup (find via 'Cannot find XML file component macro' string),
//   scan body for [rip+disp] in .bss range.
// Verified: build 606138 (ExportMapConstructionPlan @ 0x14018BBB0 uses qword_146C80870;
//   AddCluster @ 0x14013DAD0 calls MacroRegistry_Lookup with qword_146C80700)
#define X4_RVA_CONSTRUCTION_PLAN_DB     0x06C80870  /* void** — g_ConstructionPlanRegistry (RB-tree at +16) */
#define X4_RVA_MACRO_REGISTRY           0x06C80700  /* void*  — g_MacroRegistry (BST at +64) */

// ---- MacroData field offsets ----
// Returned by MacroRegistry_Lookup. Connection array is sorted by FNV-1a hash.
// WARNING: struct offsets — fragile across builds. Re-verify on game updates.
// Verified: build 606138
#define X4_MACRODATA_OFFSET_CONNECTIONS_BEGIN  0x170  /* void* — start of ConnectionEntry array */
#define X4_MACRODATA_OFFSET_CONNECTIONS_END    0x178  /* void* — end of ConnectionEntry array */

// ---- ConnectionEntry layout ----
// Each entry is 352 bytes (0x160 stride). Sorted by FNV-1a hash at +8.
// Name string at +16 is std::string (MSVC SSO: inline if len<16, heap ptr if >=16).
// Confirmed by GetNumPlannedStationModules (0x14019DD90) which reads ConnectionEntry+16
// to populate UIConstructionPlanEntry.connectionid.
// Verified: build 606138
#define X4_CONNECTION_ENTRY_SIZE    0x160  /* 352 bytes */
#define X4_CONNECTION_OFFSET_HASH   0x08   /* uint32 — FNV-1a hash of lowercased name */
#define X4_CONNECTION_OFFSET_NAME   0x10   /* std::string — connection name (e.g. "connection_room01") */

// ---- Dynamic Interior door selection ----
// Controllable::CreateDynamicInterior (0x140415D70, build 606138) selects a door connection from
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
// Allocate via GameAlloc, init via PlanEntry_Construct (0x140D0F770).
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
// Verified: build 606138 (PlanEntry_Construct @ 0x140D0F770 decompiled,
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
// Verified: build 606138 (radar_visible offset 0x400 confirmed in SetObjectRadarVisible_Action @ 0x140B92620)
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

// ======== SECTOR RESOURCE AREAS ===========================================
// ResourceArea children of a Sector, used for unfiltered resource yield access.
// GetDiscoveredSectorResources (FFI) has a player discovery filter at RA+0x1D0.
// Direct memory access bypasses it by iterating the sector's child vector.
// @verified v9.00 build 606138

// FIND: Decompile GetNumDiscoveredSectorResources (PE export). After sector class
//   validation (vtable IS-A check for class 87=Sector), find `mov rsi,[rcx+XXX]`
//   / `mov rbp,[rcx+YYY]` — these are the std::vector begin/end of ResourceArea
//   child nodes. Stride 8 (pointer per child). Each child: *(node)+0x08 = UniverseID.
//   ---
//   1. Find GetNumDiscoveredSectorResources (FFI export, PE export table)
//   2. In its body: `mov rsi, [rcx+XXX]` / `mov rbp, [rcx+YYY]` where rcx=Sector*
//      → XXX = RESAREA_VEC_BEGIN, YYY = RESAREA_VEC_END
//   3. Same function: `comisd xmm0, [rdi+ZZZ]` where rdi=ResourceArea*
//      → ZZZ = DISCOVERY_TIME
//   4. Same function: reads int value from `[ResourceArea + WWW]` for yield accumulation
//      → WWW = CURRENT_YIELD
//   5. Follows ptr at `[ResourceArea + 0x30]` → RegionYieldDef, then `[def + 0x20]` → WareClass
//   6. WareClass ware name: MSVC SSO string at +0x10 (buf/ptr) / +0x28 (capacity)
//      Cross-verify with GetRegionResourceWares which reads from same WareClass vector.
//   ---
#define X4_SECTOR_RESAREA_VEC_BEGIN     0x3F8  /* uintptr_t — vector begin (ptr to first element) */
#define X4_SECTOR_RESAREA_VEC_END       0x400  /* uintptr_t — vector end (ptr past last element) */

// --- WareClass (partial) ---
// FIND: From RegionYieldDef+0x20 → WareClass*. Ware name is an MSVC SSO
//   std::string at +0x10 (buf/ptr) / +0x28 (capacity). Cross-verify with
//   GetRegionResourceWares (PE export) which reads from same WareClass vector.
struct X4WareClass {
    char     _pad0[0x10];       // +0x00 — vtable + base fields
    char     name_buf[16];      // +0x10 — inline SSO buffer, OR char* if heap-allocated
    uint64_t name_len;          // +0x20 — string length
    uint64_t name_cap;          // +0x28 — SSO capacity; >= 16 means name_buf is a char*

    const char* name() const {
        if (name_cap >= 16)
            return *reinterpret_cast<const char* const*>(&name_buf[0]);
        return &name_buf[0];
    }
};
static_assert(offsetof(X4WareClass, name_buf) == 0x10, "WareClass name_buf offset");
static_assert(offsetof(X4WareClass, name_len) == 0x20, "WareClass name_len offset");
static_assert(offsetof(X4WareClass, name_cap) == 0x28, "WareClass name_cap offset");

// --- RegionYieldDef ---
// Static definition attached to each ResourceArea. Loaded from regionyields.xml.
// Contains ware type, yield amount, and mining/respawn tuning params.
//
// FIND: sub_140EA30F0 (XML init) populates all fields from XML attributes.
//   GetNumDiscoveredSectorResources: reads +0x20 (WareClass), +0x48 (respawn).
//   sub_140743160 (depletion): reads +0x48.
//
// XML schema: libraries/regionyields.xsd <definition> element.
// In-game verified: cur <= max_yield always; typical max values 250k–1M.
// Field-to-XML mapping verified via probe (hydrogen objectyieldfactor=1.0 /
//   gatherspeedfactor=0.7; ice objectyieldfactor=1.2 / gatherspeedfactor=1.0).
// @verified v9.00 build 606138
struct X4RegionYieldDef {
    // +0x00: definition ID std::string (MSVC layout: buf[16] + len + cap = 0x20 bytes)
    char          id_buf[16];            // +0x00 — SSO buffer or char* (MSVC SSO)
    uint64_t      id_len;               // +0x10
    uint64_t      id_cap;               // +0x18
    X4WareClass*  ware_class;           // +0x20 — XML "ware": ware type (ore, hydrogen, etc.)
    void*         boundary_data;        // +0x28 — parsed from <boundary> child element
    bool          random_rotation[3];   // +0x30 — XML randompitch/randomyaw/randomroll (order TBD)
    char          _pad0[0x05];          // +0x33 — alignment padding
    int64_t       max_yield;            // +0x38 — XML "yield": total yield amount. cur always <= max.
    float         object_yield_factor;  // +0x40 — XML "objectyieldfactor": per-object yield scale (mineral/scrap). Default 1.0.
    float         gather_speed_factor;  // +0x44 — XML "gatherspeedfactor": mining speed scale (gas). Default 1.0.
    float         respawn_delay;        // +0x48 — XML "respawndelay" × 60: seconds before respawn. -60 = no respawn.
    int32_t       rating;               // +0x4C — XML "rating": star rating × 3 (0–15). 15 = 5 stars.
    uint32_t      yield_tag_idx;        // +0x50 — XML "tag": compatibility tag index (global table lookup)
    char          _pad1[0x04];          // +0x54 — alignment padding
    void*         scan_effect;          // +0x58 — XML "scaneffect": effect definition ptr (scaneffectcolor baked in)
    int32_t       scan_effect_amount;   // +0x60 — XML "scaneffectamount": scan result effect count. Default 1.
    float         scan_effect_intensity;// +0x64 — XML "scaneffectintensity": scan result opacity/volume. Default 1.0.

    /// Read definition ID from the inline std::string.
    const char* id() const {
        if (id_cap >= 16)
            return *reinterpret_cast<const char* const*>(&id_buf[0]);
        return &id_buf[0];
    }
};
static_assert(offsetof(X4RegionYieldDef, ware_class)           == 0x20, "RegionYieldDef ware_class offset");
static_assert(offsetof(X4RegionYieldDef, boundary_data)        == 0x28, "RegionYieldDef boundary_data offset");
static_assert(offsetof(X4RegionYieldDef, random_rotation)      == 0x30, "RegionYieldDef random_rotation offset");
static_assert(offsetof(X4RegionYieldDef, max_yield)            == 0x38, "RegionYieldDef max_yield offset");
static_assert(offsetof(X4RegionYieldDef, object_yield_factor)  == 0x40, "RegionYieldDef object_yield_factor offset");
static_assert(offsetof(X4RegionYieldDef, gather_speed_factor)  == 0x44, "RegionYieldDef gather_speed_factor offset");
static_assert(offsetof(X4RegionYieldDef, respawn_delay)        == 0x48, "RegionYieldDef respawn_delay offset");
static_assert(offsetof(X4RegionYieldDef, rating)               == 0x4C, "RegionYieldDef rating offset");
static_assert(offsetof(X4RegionYieldDef, yield_tag_idx)        == 0x50, "RegionYieldDef yield_tag_idx offset");
static_assert(offsetof(X4RegionYieldDef, scan_effect)          == 0x58, "RegionYieldDef scan_effect offset");
static_assert(offsetof(X4RegionYieldDef, scan_effect_amount)   == 0x60, "RegionYieldDef scan_effect_amount offset");
static_assert(offsetof(X4RegionYieldDef, scan_effect_intensity)== 0x64, "RegionYieldDef scan_effect_intensity offset");

// --- ResourceArea ---
// Live resource node within a Sector. Child of the sector's RA vector.
// Inherits X4EntityBase (vtable + id) but diverges from X4Component after +0x10.
//
// No GameClass enum entry — identified via RTTI dynamic_cast to U::ResourceArea.
// Entity functions (get_class, is_a) work via the shared X4EntityBase.
//
// FIND: GetNumDiscoveredSectorResources: RA+0x30 = def ptr, RA+0x80 = yield,
//   RA+0x1D0 = discovery time. sub_140743160 (depletion): writes RA+0x80.
//   sub_1407425D0 (respawn): reads RA+0x38 (parent), writes RA+0x1D8 (job),
//   RA+0x1E0 (pending flag).
// @verified v9.00 build 606138
struct X4ResourceArea : X4EntityBase {
    X4RegionYieldDef*  definition;      // +0x30 — static definition (ware, max yield, params)
    void*              parent;          // +0x38 — parent/container ptr (used in respawn)
    char               _pad1[0x40];     // +0x40
    int64_t            current_yield;   // +0x80 — live yield (depletes with mining, regenerates)
    char               _pad2[0x148];    // +0x88
    double             discovery_time;  // +0x1D0 — player discovery timestamp (game time)
    void*              respawn_job;     // +0x1D8 — active respawn job ptr (null if not respawning)
    uint8_t            respawn_pending; // +0x1E0 — set to 1 when respawn queued
};
static_assert(offsetof(X4ResourceArea, id)              == 0x08,  "ResourceArea id offset");
static_assert(offsetof(X4ResourceArea, definition)      == 0x30,  "ResourceArea definition offset");
static_assert(offsetof(X4ResourceArea, parent)          == 0x38,  "ResourceArea parent offset");
static_assert(offsetof(X4ResourceArea, current_yield)   == 0x80,  "ResourceArea current_yield offset");
static_assert(offsetof(X4ResourceArea, discovery_time)  == 0x1D0, "ResourceArea discovery_time offset");
static_assert(offsetof(X4ResourceArea, respawn_job)     == 0x1D8, "ResourceArea respawn_job offset");
static_assert(offsetof(X4ResourceArea, respawn_pending) == 0x1E0, "ResourceArea respawn_pending offset");

// ======== MD EVENT SYSTEM =================================================
// Hooked by X4Native core via EventQueue_InsertOrDispatch (version_db).
// Fires on_md_before / on_md_after per event type_id.
// For typed callbacks, include x4_md_events.h (auto-generated).

typedef struct X4MdEvent {
    uint32_t  type_id;          // Event type (matches x4_md_events.h constants)
    uint64_t  source_id;        // EventSource entity Universe ID
    double    timestamp;        // Game time
    void*     raw_event;        // Raw event object (read fields via x4_md_events.h structs)
} X4MdEvent;


// ---- RadarVisibilityChangedEvent Layout ----
// Dispatched by the engine when radar visibility changes on an entity.
// Three dispatchers: SetForcedRadarVisible_Internal, SetObjectRadarVisible_Action,
// and the engine property change handler.
// FIND: Decompile SetObjectRadarVisible_Action (unique byte_sig). LEA reg,[rip+disp] in .rdata/0x02B range.
// Verified: build 606138
#define X4_RADAR_EVENT_VTABLE_RVA           0x02B44798  /* const U::RadarVisibilityChangedEvent::`vftable' */
#define X4_RADAR_EVENT_OFFSET_ENTITY_ID     24          /* uint64 — ComponentID of affected entity */
#define X4_RADAR_EVENT_OFFSET_VISIBLE       32          /* uint8  — new visibility state (0=left range, 1=entered range) */

// ---- Space-Class Visibility Offsets (type 15/86/107: clusters, sectors, zones) ----
// Different offsets from Object-class. No radar bytes (Space entities use known-to only).
// Full layout documented in docs/rev/VISIBILITY.md Section 9.
// Verified: build 606138
#define X4_SPACE_OFFSET_OWNER_FACTION_PTR       800    /* void* — owner faction context pointer */
#define X4_SPACE_OFFSET_KNOWN_READ              817    /* uint8 — encyclopedia "read" flag */
#define X4_SPACE_OFFSET_KNOWN_TO_ALL            818    /* uint8 — global known flag */
#define X4_SPACE_OFFSET_KNOWN_FACTIONS_ARR      824    /* 16 bytes — SSO faction pointer array */
#define X4_SPACE_OFFSET_KNOWN_FACTIONS_CAP      840    /* size_t — array capacity */
#define X4_SPACE_OFFSET_KNOWN_FACTIONS_COUNT    848    /* size_t — number of factions in known-to list */

// ======== ORDER SYSTEM ===================================================
// Order param value type IDs (from SetOrderParam Lua handler decompilation).
// Runtime lookup table at 0x1426F6FB0, populated by sub_141076080.
// The value struct is {int32 type, int32 pad, int64 data}.
// Internal functions (CreateOrderInternal, SetOrderParamInternal) are resolved
// via version_db — see x4_internal_func_list.inc and x4n_ship.h.
#define X4_ORDER_PARAM_TYPE_NULL        0   /* unset parameter */
#define X4_ORDER_PARAM_TYPE_INTERNAL    1   /* varies by uitype (ScriptValue) */
#define X4_ORDER_PARAM_TYPE_BOOL        2   /* data = int32 (0/1) */
#define X4_ORDER_PARAM_TYPE_NUMBER      3   /* data = int64 */
#define X4_ORDER_PARAM_TYPE_LENGTH      4   /* data = float (low 32 bits) */
#define X4_ORDER_PARAM_TYPE_TIME        5   /* data = float (low 32 bits) */
#define X4_ORDER_PARAM_TYPE_POSITION    7   /* nested list [[x,y,z], entity] */
#define X4_ORDER_PARAM_TYPE_WARE        8   /* data = void* (game hash lookup) */
#define X4_ORDER_PARAM_TYPE_DOUBLE      9   /* data = double as bits */
#define X4_ORDER_PARAM_TYPE_ENTITY      10  /* data = UniverseID */
#define X4_ORDER_PARAM_TYPE_MONEY       11  /* data = int64 */
#define X4_ORDER_PARAM_TYPE_FORMATION   12  /* data = uint32 enum */
#define X4_ORDER_PARAM_TYPE_LIST        13  /* data = vector of OrderParamValue */
#define X4_ORDER_PARAM_TYPE_MACRO       14  /* data = void* (game hash lookup) */
#define X4_ORDER_PARAM_TYPE_SECTOR      15  /* data = UniverseID */

#ifdef __cplusplus
}
#endif
