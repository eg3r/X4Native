# X4 Player & Entity API — Reverse Engineering Notes

> **Binary:** X4.exe v9.00 · **Date:** 2026-03
>
> All addresses are absolute (imagebase `0x140000000`). Subtract imagebase to get RVA.

---

## 1. Summary

Addresses and internal behavior for player-related and entity-related game API functions. These are exported functions (or Lua globals) that X4 extensions use to read/write player and entity state. Understanding their internals is critical for knowing when they're safe to call and what they actually do.

---

## 2. Player Identity Functions

The player has multiple identity entities depending on context (cockpit vs on-foot). The correct function depends on what you're trying to read.

### GetPlayerID — Actor Entity

**Address:** Reads from `player_slot[+8]`

Returns the player's **actor** entity. This entity may lack class 71 (positional) when on-foot -- do NOT use with `GetObjectPositionInSector`. Use `GetPlayerObjectID()` instead for position reads.

### GetPlayerObjectID — Positional Entity

**Address:** `0x14016C2D0` (RVA `0x16C2D0`)

Walks `player_slot[+112]` parent chain for **class 71** (positional entity). Returns the **avatar entity** when on-foot (the first class-71 ancestor). This is the correct entity for `GetObjectPositionInSector` when the player is walking.

### GetPlayerContainerID — Container (Station/Ship)

**Address:** `0x14016BD30` (RVA `0x16BD30`)

Returns the station/ship `UniverseID` the player is **inside**. Returns 0 in open space. Walks parent chain for **class 109** (container class). Works for both station and capital ship containers -- container-agnostic.

### GetPlayerOccupiedShipID — Piloted Ship

Returns the player's currently piloted ship ID. Returns 0 when on foot. Used to distinguish "piloting" from "walking" state.

### GetPlayerRoom — Room Entity (Lua Only)

**Lua global handler:** `sub_14024E920` (RVA `0x24E920`)

Walks `player_slot[0][+112]` parent chain for **class 82** (Room). Returns room `UniverseID` or nil. This is a bare Lua global -- call as `GetPlayerRoom()`, NOT `C.GetPlayerRoom()`.

Found via the Lua registration table at `sub_140236710`.

### Player Slot Global

**Address:** `0x143CA6D68` (+560 to slot pointer)

The player slot is the root data structure for all player-related queries. Multiple player API functions read fields from this structure at various offsets.

---

## 3. Position & Transform Functions

### GetObjectPositionInSector

Returns entity position relative to its sector origin. Internally walks `entity[+112]` parent chain for **class 86** (sector). Always returns sector-space coordinates regardless of entity nesting.

### SetObjectSectorPos — Entity Position Write

**Address:** `0x140180500` (RVA `0x180500`)

Sets an entity's position in sector space. Walks `entity[+112]` for **class 107** (zone) to determine zone containment. Works on any class 71 entity, including NPCs spawned via `SpawnObjectAtPos2`.

### GetPositionalOffset — Room-Local Position

**Address:** `0x14016CA80` (RVA `0x16CA80`)

With `spaceid=0`, returns position relative to the entity's direct parent (the room when on-foot). `GetPlayerID()` returns the player actor entity (class 75 = positional) which is parented to the room component when on foot. This is the correct API for room-local walking coordinates.

Internally calls `GetRelativeTransform` (`0x14039C3F0`, 380 callers -- core transform function).

### TeleportPlayerTo — Player Relocation

**Address:** `0x1401C8410` (RVA `0x1C8410`)

`TeleportPlayerTo(controllableid, allowcontrolling, instant, force) -> bool`

Moves the player into a controllable entity (ship/station seat).

---

## 4. Entity Lookup

### Component Table

**Global:** `qword_146C7A148` — component system root.

Entity lookup is O(1) index-based: `component_table[entity_id]`. No hashing, no tree traversal. Reading many entity positions per frame is extremely cheap (<0.1ms for 200 entities).

See [SUBSYSTEMS.md](SUBSYSTEMS.md) Section 8 for full details.

---

## 5. NPC & Character Functions

### GetNPCs — Lua Only

**Lua C impl:** `0x140253F00` (RVA `0x253F00`)

Returns all NPCs in/on a given component. Generic -- works for capital ships as well as stations.

### GetEnvironmentObject — Current Room

**Address:** `0x140AB4D00` (RVA `0xAB4D00`)

Reads `player->data[29496]`. Returns the current room the player is standing in. Returns 0 when in cockpit or open space. Persistent cached field, updated on room change, stable between frames.

**Runtime note:** Has been observed to return 0 at runtime in some contexts despite decompilation analysis suggesting it should work. Use `GetPlayerRoom()` (Lua) as a more reliable alternative.

---

## 6. Event & String Addresses

### MD Event Strings

| Event | Address | Notes |
|-------|---------|-------|
| `event_player_changed_activity` | `0x1429a9988` | Fires on mode changes (cockpit <-> walking, docking, etc.). 10+ MD script refs. |
| `event_object_docking_started` | `0x1429a9428` | Ship begins docking |

### Misc Addresses

| Item | Address | Notes |
|------|---------|-------|
| `ToggleScreenDisplayOption` thunk | `0x140095F90` | 110k+ xrefs. Logging/assert dispatcher. Avoid as starting point for reverse engineering. |

---

## 7. Function Reference

| Name | Address | Purpose |
|------|---------|---------|
| `GetPlayerObjectID` | `0x14016C2D0` | Class 71 positional entity for player |
| `GetPlayerContainerID` | `0x14016BD30` | Station/ship container (class 109) |
| `GetPlayerRoom` (Lua handler) | `0x14024E920` | Room entity (class 82) |
| `GetPositionalOffset` | `0x14016CA80` | Room-local position with spaceid=0 |
| `SetObjectSectorPos` | `0x140180500` | Entity position write (class 107 zone walk) |
| `TeleportPlayerTo` | `0x1401C8410` | Move player into controllable |
| `GetEnvironmentObject` | `0x140AB4D00` | Current room (cached field path) |
| Player slot global | `0x143CA6D68` | +560 to slot pointer |
| Component table global | `0x146C7A148` | O(1) entity lookup |

---

## 8. Related Documents

| Document | Contents |
|----------|----------|
| [SUBSYSTEMS.md](SUBSYSTEMS.md) | Component system, Lua registration table, event bus |
| [GAME_LOOP.md](GAME_LOOP.md) | Frame tick, subsystem update, UI event dispatch |
| [STATE_MUTATION.md](STATE_MUTATION.md) | Safety analysis for calling these functions from hooks |
| [WALKABLE_INTERIORS.md](WALKABLE_INTERIORS.md) | Interior system, WalkableModule, room hierarchy |
