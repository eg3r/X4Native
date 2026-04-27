// ---------------------------------------------------------------------------
// X4 Internal (Non-Exported) Function List — X-Macro
//
// Same pattern as x4_game_func_list.inc but for reverse-engineered functions
// that aren't in X4.exe's export table. Addresses are resolved at runtime
// from native/version_db/internal_functions.json (RVA per game build).
//
// Format:  X4_FUNC(return_type, function_name, (param_list))
//
// Example:
//   X4_FUNC(void, PhysicsUpdate, (float deltaTime))
//   X4_FUNC(bool, IsEntityVisible, (UniverseID entity, UniverseID observer))
//
// The JSON database must have a matching entry:
//   "PhysicsUpdate": { "900": { "rva": "0x00A1B2C3" } }
// ---------------------------------------------------------------------------

// -- Add reverse-engineered function declarations below this line --

// Game loop frame tick — called once per frame from the message pump.
// a1 = engine context (opaque, 736-byte struct), a2 = true when suspended/minimized.
// Verified: v9.00 build 900, RVA 0x00F4A2A0
X4_FUNC(void, X4_FrameTick, (void* engineCtx, bool isSuspended))

// ComponentRegistry_Find — universal UniverseID to Object* converter.
// 1,901 callers in the binary. Returns the internal component pointer for a given ID.
// access_type: 4 = read access (most common). Returns nullptr if not found.
// g_ComponentRegistry global at 0x146C73D30 (RVA 0x06C73D30) is the first arg.
// Verified: v9.00 build 600626, RVA 0x000CE6B0
X4_FUNC(void*, ComponentRegistry_Find, (void* registry, uint64_t universe_id, int access_type))

// Component_GetCombinedSeed — returns the pre-computed combined seed (raw + session).
// This is exactly what MD's $Station.seed returns.
// 2-instruction function: mov rax,[rcx+3C0h]; retn
// Takes the component Object* (from ComponentRegistry_Find). Returns int64.
// Verified: v9.00 build 600626, RVA 0x00315260
X4_FUNC(int64_t, Component_GetCombinedSeed, (void* component))

// MacroRegistry_Lookup — resolves a macro name to its internal MacroData pointer.
// registry = g_MacroRegistry (RVA 0x06C73E30), sv = {const char* data, size_t len},
// silent = 1 to suppress error logs on miss. Returns MacroData* or nullptr.
// Uses FNV-1a hash with lowercased input. 200+ callers.
// Verified: v9.00 build 600626, RVA 0x009E72B0
X4_FUNC(void*, MacroRegistry_Lookup, (void* registry, const void* string_view, uint8_t silent))

// ConstructionDB_CreatePlanDirect — allocates EditableConstructionPlan (296 bytes),
// initializes fields, sets vtable, registers in g_ConstructionPlanRegistry.
// Params: registry = *g_ConstructionPlanDB, source_type = 2 (local),
//         id/name/desc are X4String* (Release ABI std::string, 32 bytes each).
// Returns plan pointer on success, nullptr on failure.
// Verified: v9.00 build 600626, RVA 0x00D0E680
// PlanEntry_Construct — initializes a 528-byte PlanEntry in-place.
// Caller must pre-allocate 528 bytes (via GameAlloc). If id==0, auto-assigns from
// global counter under critical section. loadout_ptr=0 calls default loadout init.
// transform is pointer to 4x __m128 (64 bytes). Returns this.
// Verified: v9.00 build 600626, RVA 0x00D09C90
X4_FUNC(void*, PlanEntry_Construct, (void* this_ptr, void* macro_ptr, void* connection_ptr, void* predecessor_ptr, void* pred_connection_ptr, const void* transform_4x16, void* loadout_ptr, bool is_fixed, bool is_modified, bool is_bookmark, int64_t id))

// GameAlloc — game's internal memory allocator (C++ operator new equivalent).
// size = byte count, alignment via last param (typically 16).
// Middle params (a2-a4) are register passthrough noise, ignored by allocator.
// Returns allocated pointer or nullptr.
// Verified: v9.00 build 600626, RVA 0x0145CB90
X4_FUNC(void*, GameAlloc, (size_t size, int _unused1, int _unused2, int _unused3, int alignment))

// Note: id/name/desc are std::string* (MSVC ABI, 32 bytes each). Declared as void* for C compat.
// Callers must build with matching ABI (Release or RelWithDebInfo, NOT Debug).
X4_FUNC(void*, ConstructionDB_CreatePlanDirect, (void* registry, int source_type, const void* id_str, const void* name_str, const void* desc_str))

// ConstructionDB_AddPlan — registers a plan into the construction plan RB-tree.
// Validates: plan != null, plan id length > 0, source_type matches plan's +240.
// Computes FNV-1a of plan id string, inserts into tree at registry+16.
// Returns 1 on success, 0 if already exists or validation fails.
// Verified: v9.00 build 600626, RVA 0x00D0E4E0
X4_FUNC(bool, ConstructionDB_AddPlan, (void* registry, int source_type, void* plan))

// RadarVisibilityChanged_BuildEvent — creates a RadarVisibilityChangedEvent when the
// engine's property system detects a radar visibility transition (entering OR leaving range).
// Called from the sector update property change handler (case 378 in the jump table).
// Returns the event object pointer (or 0 on alloc failure). The event is dispatched by
// the caller via EventDispatch_PriorityQueue after this returns.
// Event layout: +0 vtable, +8 refcount, +16 reserved, +24 entity ComponentID (uint64), +32 visible (uint8).
// Hook AFTER to capture: entity = *(uint64*)(retval+24), visible = *(uint8*)(retval+32).
// Verified: v9.00 build 600626, RVA 0x00A2D810
X4_FUNC(void*, RadarVisibilityChanged_BuildEvent, (void* property_data))

// IsRadarVisible_ReadByte — reads the radar_visible byte from an Object-class component.
// Returns *(uint8_t*)(component + 0x400). Tiny 2-instruction function.
// Only 2 callers: GetComponentData("isradarvisible") and MD property evaluator.
// Takes raw component pointer (from ComponentRegistry_Find), NOT a UniverseID.
// Verified: v9.00 build 600626, RVA 0x0011BE30
X4_FUNC(uint8_t, IsRadarVisible_ReadByte, (void* component))

// CreateOrderInternal — creates an AI order on a Controllable entity.
// Bypasses the player-ownership check in the exported CreateOrder3.
// entity = X4Component* (must be Controllable-derived, from ComponentRegistry_Find)
// string_view = pointer to {const char* data, size_t length} struct (16-byte aligned)
// mode: 3 = Default (replaces entity[160], loops when queue empty)
//       1 = Queue   (pushes to entity[150..152] vector, is_temp=false, persists)
//       0 = Immediate (pushes to queue, is_temp=true, destroyed on completion)
//       2 = same as 0 (game checks ==3 and ==1 only)
// override = bool (is override order, stored at AIOrder+43)
// Returns AIOrder* (112 bytes, vtable AI::AIOrder). nullptr on unknown order ID.
// Called by exported CreateOrder3 after the ownership check.
// See also: removal logic at sub_141034E00, AIOrder layout in STATE_MUTATION.md.
// Verified: v9.00 build 603098, RVA 0x00424A90
X4_FUNC(void*, CreateOrderInternal, (void* entity, void* string_view, int mode, char override_flag))

// SetOrderParamInternal — sets a parameter value on an order object.
// order = order object pointer (from CreateOrderInternal)
// param_idx = 0-based parameter index within the order definition
// value = pointer to OrderParamValue struct {int32 type, int32 pad, int64 data}
// Known types: 10 = entity (data = UniverseID)
// Returns bool success.
// Called by Lua SetOrderParam handler at sub_1402897D0.
// Verified: v9.00 build 603098, RVA 0x0102BEE0
X4_FUNC(char, SetOrderParamInternal, (void* order, size_t param_idx, void* value))

// SetObjectRadarVisible_Action — MD action handler for `set_object_radar_visible`.
// Writes bool to component+0x400, dispatches RadarVisibilityChangedEvent, propagates to children.
// This is the SCRIPT writer (story missions). The engine writes +0x400 via the property system.
// Takes: a1 = MD action context (object ref at +40), a2/a3 = condition context.
// Verified: v9.00 build 600626, RVA 0x00B8BD30
X4_FUNC(void, SetObjectRadarVisible_Action, (void* action_ctx, void* param2, void* param3))

// Controllable::CreateDynamicInterior — creates a corridor+room interior on a station.
// Called by npc_instantiation during savegame load and by MD create_dynamic_interior action.
// Params 9-14 are variadic in the original, but on x64 MSVC they occupy the same stack
// slots as fixed params, so we declare all 14 as fixed for hooking.
// See docs/rev/WALKABLE_INTERIORS.md §14 for full analysis.
// Verified: v9.00 build 600626, RVA 0x004153A0
X4_FUNC(void, CreateDynamicInterior, (void* station, void* output, void* corridor_macro, void* door_conn, void* room_macro, int roomtype, void* interior_name, void* module, int64_t reserved, int64_t seed, uint8_t persistent, uint8_t private_flag, uint8_t unknown_flag, void* display_name))

// SetCommander — assigns a ship as subordinate to a commander with a role.
// PE export with no FFI signature. Signature derived via IDA decompilation.
// controllableid = ship to assign, commanderid = station or capital ship,
// assignment = role string ("defence", "trade", "mining", "attack", etc.)
// Returns true on success. No ownership check — safe for NPC factions.
// Verified: v9.00 build 603098, RVA 0x001C61F0
X4_FUNC(bool, SetCommander, (UniverseID controllableid, UniverseID commanderid, const char* assignment))

// SetAssignment — change subordinate role without changing commander.
// PE export with no FFI signature. Signature derived via IDA decompilation.
// Verified: v9.00 build 603098, RVA 0x001C5FC0
X4_FUNC(void, SetAssignment, (UniverseID controllableid, const char* assignment))

// GetFactionBuildMethod — returns a faction's effective production method.
// Resolves: faction → buildrules override → primary race default → "default".
// PE export (ordinal 617) with no FFI signature. Signature derived via IDA decompilation.
// Returns method string: "default", "terran", "xenon", "teladi", "paranid", "split",
// "boron", "closedloop". Used by MRP for BOM explosion to select correct ware recipes.
// Verified: v9.00 build 603098, RVA 0x00194D40
X4_FUNC(const char*, GetFactionBuildMethod, (const char* factionid))

// EventQueue_InsertOrDispatch — central MD event dispatch funnel.
// ALL MD events (1245+ call sites) pass through this function. If immediate==true
// and timestamp <= current_time + epsilon, dispatches immediately to cue listeners.
// Otherwise inserts into a timestamp-sorted priority queue for deferred dispatch.
// Hooked by X4Native core to fire on_md_before/on_md_after per event type_id.
// Verified: v9.00 build 603098, RVA 0x00958390
X4_FUNC(void, EventQueue_InsertOrDispatch, (void* event_source, void* event_object, double timestamp, char immediate))

// FactionRelation_LookupReasonID — resolves a `relationchangereason.*` string
// to the integer reason ID consumed by FactionRelation_ApplyMutation.
// Performs a memcmp-based binary search over a sorted table at 0x143C9A530
// (BSS, populated by FactionRelation_InitReasonTable at startup). Each table
// entry is a 24-byte {char* str_ptr, uint64_t str_len, int32_t reason_id}.
// The first arg is documented as `void* unused` — pass nullptr; the second
// arg is a string_view (pointer + length). Returns 0 on miss (default
// reason). See `X4Native/docs/rev/FACTION_RELATIONS.md §3.2`.
//
// Use case: drift-protection wrapper for the companion-side fallback string
// table in `companion/src/validation.rs::VALID_RELATION_REASONS`. Calling
// the FFI lets us stay in lockstep with vanilla even if Egosoft adds new
// reasons — fallback table remains as the offline-data path.
//
// Verified: v9.00 build 606138 docs at FACTION_RELATIONS.md (§3.2 + §10
// table — note the table at line 499 lists 0x002CBF00; §3.2 header lists
// 0x002CAE60). Per-build RVA must come from version_db. §3.2.F-4.
X4_FUNC(int32_t, FactionRelation_LookupReasonID, (void* unused, const char* reason_str, uint64_t reason_len))

// FactionRelation_GetFloat — direct NPC-NPC relation float reader.
// Returns the [-1.0, +1.0] internal float between two FactionClass pointers.
// Layered: probes the boost map (decay-checked) at FactionClass+640+80, falls
// back to the base map at FactionClass+640+16. Returns 0.0 if neither has an
// entry (true neutral). Returns ~+1.0 (constant Y) when a == b.
//
// SIDE-EFFECT: ages out expired boosts via FactionRelation_RemoveBoost during
// the read (cleanup-on-read). Safe for UI-thread polling but means matrix
// polling continuously gardens the boost map.
//
// Caller convention: rcx=a, rdx=b, returns float in xmm0. NOT exported.
// Use case: galaxy-wide NPC-NPC relation matrix snapshot for X4Strategos
// §4.1 Session 2a (~21 factions × 21 = 441 reads per cycle = trivial cost).
//
// Verified: v9.00 build 606138, RVA 0x0030F700. Per-build RVA must come from
// version_db. Function size 0x37 bytes. See docs/rev/FACTION_RELATIONS.md §2.2.
//
// Declared with void* for SDK ABI compat (the X4_FUNC table is included into
// game_api.cpp without x4_manual_types.h's full struct declarations). The
// type-safe wrapper in x4n_faction.h casts to/from X4FactionClass*.
X4_FUNC(float, FactionRelation_GetFloat, (void* faction_a, void* faction_b))
