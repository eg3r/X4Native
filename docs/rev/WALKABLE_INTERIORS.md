# X4 Walkable Interior System — Reverse Engineering Notes

> **Binary:** X4.exe v9.00 (build 900) · **Date:** 2026-03
>
> All addresses are absolute (imagebase `0x140000000`). Subtract imagebase to get RVA.

---

## 1. Summary

X4 uses a **unified `WalkableModule` system** for all walkable interiors. Both station modules and capital ship modules inherit from the same `WalkableModule` C++ class. The engine makes no distinction between station interiors and ship interiors at the API level -- both are just containers with walkable rooms.

---

## 2. WalkableModule Class

**Vtable:** `0x143196708`

Shared by station and ship interiors. The `WalkableModule` class is the base for all walkable geometry in the game.

### Component Hierarchy

```
Container (station or capital ship)
  +-- WalkableModule (station module or ship module)
        +-- Room (class 82)
              +-- Actor entities (NPCs, player avatar)
```

### Component Class IDs

| Class ID | Name | Notes |
|----------|------|-------|
| 82 | Room | Walkable room entity |
| 109 | Container | Station or ship (docking container) |
| 118 | WalkableModule | Walkable module within a container |

---

## 3. Capital Ship Interiors

Capital ships (L-class and XL-class: `ShipLarge`, `ShipExtraLarge`) have walkable interiors. Confirmed by multiple IDA findings:

### Evidence

| Finding | Address | Detail |
|---------|---------|--------|
| Capital ship spawn fallback | `0x142b43b30` | String: `"No capital ship found to spawn player. Player will spawn in player ship!"` -- the engine explicitly tries to find a capital ship room |
| `crewquarters` room type | `0x1429baff8` | **20+ data xrefs** to ship component definitions -- these are ship macro entries defining crew quarter rooms |
| `ShipFilterByIsCapitalShip` RTTI | `0x1431a0560` | Dedicated capital-ship filter used in room/walkable queries |
| `lastdockwalkablearea` property | `0x14298f008` | **20+ data xrefs** -- tracks the walkable area entered after docking inside a capital ship |
| `iscapitalship` property | `0x1429b31b0` | Component property identifying capital ship class |

### Station vs Ship Interiors

Station interiors are **dynamic** -- loaded on demand via `create_dynamic_interior` MD action. Ship interiors may be **statically loaded** (always present on capital ships), but this is unconfirmed.

---

## 4. Room System

The engine has a full `RoomDB` and `RoomGroupDB` (RTTI confirmed). Rooms are **class 82** entities in the component hierarchy.

### Room Types

| Type | String Address | Notes |
|------|---------------|-------|
| `interiorcorridor` | `0x1429c5040` | Corridor room type |
| `interiordoor` | `0x1429c5058` | Door connecting rooms |
| `interiorroom` | `0x1429c5068` | Generic room type |
| `crewquarters` | `0x1429baff8` | Crew quarters (capital ships, 20+ xrefs) |

### Room Lifecycle (MD Signals)

Found in strings:
```
create_dynamic_interior
add_room_to_dynamic_interior
attach_component_to_interior
remove_dynamic_interior
set_dynamic_interior_private
set_dynamic_interior_persistent
```

---

## 5. FirstPersonController

The walk state is managed by a **FirstPersonController** object:

| Item | Address | Notes |
|------|---------|-------|
| Constructor | `0x140D2E070` | Allocates 960-byte walk controller |
| Vtable | `0x142BCBCB8` | Type 22 |
| Location | `player_slot[0][+512]` | Offset within player slot structure |

### Input Context

When the player is on foot, the game switches to first-person controller mode:

- Input context: `INPUT_CONTEXT_REQUESTER_FIRSTPERSONCONTROLLER`
- Input state: `INPUT_STATE_FP_WALK`
- Input range: `INPUT_RANGE_FP_WALK`
- Actions: `lock_firstperson_walk_input`, `start_actor_walk`, `stop_actor_walk`

---

## 6. Transporter System

Capital ships and large stations have **transporters** -- in-world teleporters that move characters between rooms:

| Function/Signal | Description |
|-----------------|-------------|
| `GetRoomForTransporter` | Returns the room a transporter leads to |
| `GetTransporterLocationName/Component` | Destination name and component |
| `GetValidTransporterTargets` / `GetValidTransporterTargets2` | List of reachable destinations |
| `activate_transporter` (MD signal) | Player uses a transporter |
| `performtransportertransition` (MD signal) | Transition executing |
| `find_player_transporter_slot` (MD signal) | Find entry point for player |

---

## 7. Component Properties

Properties queryable via `GetComponentData` for walkable interiors:

| Property | Description |
|----------|-------------|
| `isavatar` | True on the player's physical character entity when on foot |
| `avatarentity` | Reference to the player's avatar component |
| `walkable` | Component can be walked in |
| `walkablemodule` | A walkable module (station module OR capital ship module) |
| `haswalkableroom` | Component has at least one walkable room |
| `iscurrentlywalkable` | Currently loaded and active for walking |
| `iswalkable` | Static property -- supports walking at all |
| `enterable` | Room can be entered (may differ from walkable) |
| `dynamicinterior` | Interior that loads on demand (stations) |
| `interiortype` | Type classification of an interior |
| `iscapitalship` | Component property identifying capital ship class |
| `lastdockwalkablearea` | Last walkable area entered after docking (ship property) |

---

## 8. Function Reference

| Name | Address | RVA | Purpose |
|------|---------|-----|---------|
| WalkableModule vtable | `0x143196708` | — | Shared by station and ship interiors |
| FirstPersonController ctor | `0x140D2E070` | `0xD2E070` | 960-byte walk controller allocation |
| FirstPersonController vtable | `0x142BCBCB8` | — | Type 22 |
| `crewquarters` string | `0x1429baff8` | — | 20+ ship macro data xrefs |
| `lastdockwalkablearea` string | `0x14298f008` | — | 20+ ship component data xrefs |
| `iscapitalship` string | `0x1429b31b0` | — | Capital ship property |
| Capital ship fallback string | `0x142b43b30` | — | "No capital ship found..." |
| `ShipFilterByIsCapitalShip` RTTI | `0x1431a0560` | — | Capital ship filter |
| `interiorcorridor` string | `0x1429c5040` | — | Room type string |
| `interiordoor` string | `0x1429c5058` | — | Room type string |
| `interiorroom` string | `0x1429c5068` | — | Room type string |

---

## 9. Related Documents

| Document | Contents |
|----------|----------|
| [PLAYER_ENTITY_API.md](PLAYER_ENTITY_API.md) | Player identity functions, GetPlayerRoom, GetPositionalOffset |
| [SUBSYSTEMS.md](SUBSYSTEMS.md) | Component class IDs, universe hierarchy |
| [GAME_LOOP.md](GAME_LOOP.md) | Frame tick -- walk updates happen post-frame |
