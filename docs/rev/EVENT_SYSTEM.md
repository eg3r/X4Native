# X4 MD Event Dispatch System (Binary Analysis)

Reverse engineering of X4's internal event dispatch system, focusing on how MD event cues
(like `event_object_destroyed`, `event_faction_relation_changed`) are generated and delivered.

**Game Version**: 9.0
**Image Base**: `0x140000000`
**Source**: binary analysis (RTTI tracing, vtable mapping, decompilation)

---

## Architecture Overview

X4 has **three separate event systems** that do not cross-talk:

1. **MD Event System** (documented here) — Internal typed events dispatched to Mission Director
   cue listeners. Uses integer type IDs (0-555) and polymorphic C++ event objects.
   Hooked by X4Native via `EventQueue_InsertOrDispatch` for `on_md_before/after`.

2. **Lua UI Event System** — String-named events dispatched via `genericEvent` contract to
   `RegisterEvent()` subscribers. Used for UI lifecycle events only. See GAME_LOOP.md.

3. **C++ Typed Events** — RTTI-based subsystem callbacks (e.g., `U::MoneyUpdatedEvent`).
   Internal to specific engine subsystems.

The only bridge between MD and Lua is the MD action `<raise_lua_event>`, which is a one-way
MD-to-Lua push. X4Native's `bridge_lua_event()` provides the reverse (Lua-to-C++).

---

## Key Functions

### EventQueue_InsertOrDispatch

| Property | Value |
|---|---|
| VA | `0x140958390` |
| RVA | `0x00958390` |
| Signature | `void __fastcall(event_source* rcx, event_object* rdx, double xmm2_timestamp, char r9_immediate)` |
| Thread | Game main thread only |

Central event queue entry point. All MD events pass through here. Behavior:
- If `immediate=true` AND `timestamp <= current_game_time + 0.0001`, dispatches immediately
  via `EventQueue_ImmediateDispatch`.
- Otherwise inserts into a timestamp-sorted priority queue for deferred dispatch.
- Increments reference count on event_object when queuing.

### EventQueue_ImmediateDispatch

| Property | Value |
|---|---|
| VA | `0x14095A410` |
| RVA | `0x0095A410` |
| Signature | `void __fastcall(event_source* rcx, event_object* rdx)` |

Iterates MD cue listeners stored in `qword_143886AF0` (up to 2048 entries).
Gets event type ID via `event_object->vtable[1]()`.
Calls each matching listener via `listener->vtable[4](listener, source, ctx, typeID, event)`.

### PropertyChangeDispatcher (Jump Table)

| Property | Value |
|---|---|
| VA | `0x14095AA50` |
| RVA | `0x0095AA50` |
| Size | ~15,056 bytes |

Massive switch/jump table with 557 cases indexed by type_id (0-555). Each case calls a
type-specific builder function to construct the corresponding event object. The type_id
is resolved from short property names (e.g., `"killed"` -> type_id 233) via binary search
in the PropertyChange Type Table at `0x142540C20`.

Note: MD descriptor IDs (e.g., 1191 for `event_object_destroyed`) are a separate namespace
used by the MD scripting layer. The dispatcher uses type_ids directly, not descriptor IDs.

---

## Event Object Layout

### Common Base (All Events)

```
+0x00: vtable pointer (8 bytes)        -- polymorphic dispatch
+0x08: reference count (int32)          -- starts at 0, incremented on queue insertion
+0x0C: state (int32)                    -- 1=pending, 2=dispatched, 3=consumed
+0x10: reserved (8 bytes)
+0x18: [event-specific payload begins]
```

### Virtual Function Table (All Events)

```
vtable[-1]: RTTI Complete Object Locator pointer
vtable[0]:  destructor
vtable[1]:  GetEventTypeId() -> uint32_t  (always trivial: mov eax, IMM32; ret)
vtable[2]:  (varies)
vtable[3]:  (varies)
```

---

## Event Type ID Table

### Primary Strategic Events

| Type ID | C++ Class (RTTI) | MD Cue Name | Size | VTable VA |
|---|---|---|---|---|
| 29 | AttackedEvent | event_object_attacked | 48 | 0x142B48300 |
| 55 | BuildFinishedEvent | event_build_finished | 40 | 0x142B4A398 |
| 184 | FactionRelationChangedEvent | event_faction_relation_changed | 52 | 0x142B47538 |
| 233 | KilledEvent | event_object_destroyed | 64 | 0x142B4A168 |
| 268 | ObjectChangedOwnerEvent | event_object_changed_owner | 56 | 0x142B48990 |
| 302 | ObjectSignalledEvent | event_object_signalled | 48 | 0x142B46BD0 |
| 409 | SectorChangedOwnerEvent | event_contained_sector_changed_owner | 48 | 0x142B48E28 |
| 513 | UnitDestroyedEvent | event_unit_destroyed | 32 | 0x142B46D58 |

### Secondary / Related Events

| Type ID | C++ Class | Notes |
|---|---|---|
| 31 | AttackStartedEvent | Combat start transition |
| 32 | AttackStoppedEvent | Combat end transition |
| 54 | BuildCancelledEvent | Build cancelled |
| 60 | BuildStartedEvent | Build started |
| 77 | ChangedOwnerEvent | Base ownership change class |
| 78 | ChangedOwnershipContestedEvent | Sector contested state |
| 85 | ChangedTrueOwnerEvent | True owner change base |
| 171 | EntityChangedOwnerEvent | Contained entity owner change |
| 172 | EntityChangedTrueOwnerEvent | Contained entity true owner change |
| 185 | FactionRelationRangeChangedEvent | Relation crosses range boundary |
| 204 | GodCreatedFactoryEvent | God-spawned factory |
| 206 | GodCreatedShipEvent | God-spawned ship |
| 207 | GodCreatedStationEvent | God-spawned station |
| 218 | HullDamagedEvent | Hull damage event |
| 229 | JobShipActivatedEvent | Job ship activated |
| 234 | KilledComponentEvent | Killer-side destruction event |
| 271 | ObjectChangedTrueOwnerEvent | Object true owner change |
| 382 | RelationChangedEvent | Base relation change class |
| 383 | RelationRangeChangedEvent | Base range change class |
| 410 | SectorChangedTrueOwnerEvent | Sector true owner change |
| 421 | ShipBuiltEvent | Player ship built |
| 445 | StationBuiltEvent | Player station built |

---

## Per-Event Struct Layouts

### KilledEvent (TypeID 233)

```
+0x18: kill_method (int32)              -- kill method enum (from property 638)
+0x20: killer_id (u64)                  -- entity ID of killer (from property 559)
+0x28: kill_weapon_type (int32)         -- weapon type enum (from property 757)
+0x30: parent_entity_id (u64)           -- parent entity ID (from property 756)
+0x38: was_parent_killed (bool)         -- parent also killed (from property 934)
```

Builder: `0x1409603E0`. Fires recursively through subordinate hierarchy via `0x1403A6A70`.

### FactionRelationChangedEvent (TypeID 184)

```
+0x18: faction1_ptr (u64)              -- faction descriptor pointer (FNV-1a hash lookup)
+0x20: faction2_ptr (u64)              -- other faction descriptor pointer
+0x28: new_relation (float32)          -- new relation value
+0x2C: old_relation (float32)          -- previous relation value
+0x30: reason_id (int32)               -- relation change reason
```

Builder: `0x14097EDE0`. Faction pointers resolved from global table at `qword_146C868E0 + 16`.

### SectorChangedOwnerEvent (TypeID 409)

```
+0x18: sector_id (u64)                 -- sector entity ID
+0x20: new_owner_faction_ptr (u64)     -- new owner faction descriptor
+0x28: old_owner_faction_ptr (u64)     -- previous owner faction descriptor
```

Builder: `0x140745AA0` (direct fire, walks parent hierarchy), `0x140973380` (from property data).

### ObjectChangedOwnerEvent (TypeID 268)

```
+0x18: object_id (u64)                 -- changed object entity ID
+0x20: component_type (int32)          -- component class ID (120 if null)
+0x28: new_owner (u64)                 -- new owner entity/faction
+0x30: old_owner (u64)                 -- previous owner entity/faction
```

Builder: `0x1409767F0`.

### BuildFinishedEvent (TypeID 55)

```
+0x18: buildprocessor_id (u64)         -- build processor entity ID
+0x20: buildtask_id (u64)              -- build task reference
```

Builder: `0x1409631E0`.

### ObjectSignalledEvent (TypeID 302)

```
+0x18: signal_name_ptr (u64)           -- signal name (refcounted object, move semantics)
+0x20: param2_ptr (u64)                -- param2 (refcounted object)
+0x28: param3_ptr (u64)                -- param3 (refcounted object)
```

Builder: `0x140988DB0`. Note: Fields use move semantics (source zeroed after transfer).
The objects are NOT simple strings -- they are refcounted game objects with class IDs.

### AttackedEvent (TypeID 29)

```
+0x18: attack_method (int32)           -- attack method enum
+0x20: attacker_id (u64)               -- attacker entity ID
+0x28: attacked_component_id (u64)     -- specific component hit
```

Builder: `0x140975100`.

### UnitDestroyedEvent (TypeID 513)

```
+0x18: field_18 (int32)                -- from property 172 (likely unit type)
+0x1C: field_1C (int32)                -- from property 665 (likely unit ID)
```

Builder: `0x140A33CC0`. Fires for units/drones/deployables, not ships or stations.

---

## Property Descriptor ID Mapping

Maps MD cue string names to integer IDs used by the PropertyChangeDispatcher:

| MD Event Name | Prop ID |
|---|---|
| event_object_attacked | 1170 |
| event_object_hull_damaged | 1177 |
| event_object_changed_owner | 1184 |
| event_object_changed_true_owner | 1187 |
| event_object_destroyed | 1191 |
| event_object_killed_object | 1192 |
| event_contained_sector_changed_owner | 1203 |
| event_contained_sector_changed_true_owner | 1204 |
| event_player_killed_object | 1205 |
| event_player_owned_destroyed | 1209 |
| event_object_attacked_object | 1215 |
| event_build_started | 1355 |
| event_build_finished | 1356 |
| event_player_relation_changed | 1388 |
| event_faction_relation_changed | 1389 |
| event_unit_destroyed | 1394 |
| event_object_signalled | 1401 |

Property descriptor table located at `0x142262000` range, entries are 16-byte pairs:
`[string_ptr (u64)] [property_id (u64)]`.

---

## Hooking Considerations

### Single Central Hook

Hook `EventQueue_InsertOrDispatch` to intercept all events. Safe to call `vtable[1]()` in
a before-hook to get type ID. Performance impact is minimal (type ID check + switch is ~10
cycles for non-matching events). All callers are on the game main thread.

### Per-Builder Hooks

Hook individual builder functions for specific events. More surgical but requires one hook
per event type and multiple construction sites exist for some events (KilledEvent has 6+
sites, AttackedEvent has 3).

### Version Sensitivity

All addresses (functions, vtables) are version-specific and shift with game patches.
Event type ID values are historically stable across minor versions but should be verified.
Struct layouts (field offsets) are stable within a major version but could change if fields
are added. MD bridge via `<raise_lua_event>` avoids all version sensitivity.
