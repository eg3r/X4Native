# X4 Visibility, Fog of War, and Radar Coverage Systems

> **Binary:** X4.exe v9.00 (builds 600626, 602526) | **Date:** 2026-03-26
>
> All addresses are absolute (imagebase `0x140000000`). Subtract imagebase to get RVA.
>
> Consolidates findings from IDA decompilation, game script analysis, and runtime testing.

---

## 1. System Overview

X4 uses six independent subsystems to control whether an entity appears on the map, radar, HUD, or in 3D rendering. Each operates on different data, at different offsets, and can be set independently.

```
+============================================================================+
|                       X4 VISIBILITY SYSTEM MAP                              |
+============================================================================+
|                                                                            |
|  +-----------------+    +-----------------+    +----------------------+    |
|  |  KNOWN-TO LIST  |    | RADAR VISIBLE   |    | FORCED RADAR VISIBLE |    |
|  |  (per-faction)  |    | (game engine)   |    | (FFI override)       |    |
|  |  offset +858/+888|   | offset +1024    |    | offset +1025         |    |
|  +---------+-------+    +--------+--------+    +-----------+----------+    |
|            |                     |                         |               |
|            v                     v                         v               |
|  +------------------------------------------------------------+           |
|  |  MAP VISIBILITY GATE:  isknown AND isradarvisible          |           |
|  |  (menu_map.lua:7471)                                       |           |
|  +-----+------------------------------------------------------+           |
|        |                                                                   |
|        v                                                                   |
|  +-------------+                                                           |
|  |  MAP / HUD  |                                                           |
|  +-------------+                                                           |
|                                                                            |
|  +-----------------------+   +---------------------+                       |
|  | FACTION DISCOVERY     |   | ENCYCLOPEDIA ITEMS  |                       |
|  | (MD set_faction_known)|   | (AddKnownItem Lua)  |                       |
|  +-----------+-----------+   +----------+----------+                       |
|              |                          |                                  |
|              v                          v                                  |
|  +--------------+              +------------------+                        |
|  | Diplomacy UI |              | Encyclopedia UI  |                        |
|  +--------------+              +------------------+                        |
|                                                                            |
|  +------------------------------------------------------------------+     |
|  |  3D RENDERING: Purely spatial. Entities in loaded zones ALWAYS    |     |
|  |  render regardless of known/radar state. No filter at all.       |     |
|  +------------------------------------------------------------------+     |
|                                                                            |
+============================================================================+
```

**Summary table:**

| System | Controls | Storage | Set Via | Read Via |
|--------|----------|---------|---------|----------|
| Component known-to | Map `isknown` | Faction ptr array at +864 (obj) / +824 (space) | `SetKnownTo` FFI, MD `<set_known>` | `IsObjectKnown` FFI, `GetComponentData("isknown")` |
| Radar visible | Map `isradarvisible` | Byte at +1024 | MD `set_object_radar_visible`, game engine | `GetComponentData("isradarvisible")` (Lua only) |
| Forced radar visible | Overrides +1024 | Byte at +1025 | `SetObjectForcedRadarVisible` FFI | No direct reader |
| Faction discovery | Diplomacy UI | MD-level flag | MD `<set_faction_known>` only | No C FFI reader |
| Encyclopedia entries | Encyclopedia/diplomacy UI | Player entity array | Lua `AddKnownItem` only | Lua `IsKnownItem` |
| 3D rendering | Visual mesh | Zone load state | Spatial (automatic) | N/A |

---

## 2. The Gravidar System (Radar Coverage Model)

X4 calls its radar system the **"gravidar"**. This is the unified detection system that ships, stations, and satellites use to detect entities.

### 2.1 How Gravidar Works

The gravidar is a per-object scanning system. Each defensible object (ships, stations, satellites) has a radar range that determines how far it can detect entities. When an entity falls within the gravidar range of any player-owned or player-accessible asset, the entity becomes radar-visible.

```
GRAVIDAR DETECTION MODEL
========================

  Player Ship (maxradarrange = 40km)
         |
         v
  +------+------+
  | SCAN RADIUS  |  <-- determined by ship equipment + mods
  |   40km       |
  |              |
  |   [NPC Ship] |  <-- within range: radar_visible = 1
  |              |
  +--------------+

  Player Station (maxradarrange = varies)
         |
         v
  +------+------+
  | SCAN RADIUS  |  <-- from station radar modules
  |              |
  +--------------+

  Deployed Satellite (fixed range)
         |
         v
  +------+------+
  | SCAN RADIUS  |  <-- fixed per satellite type
  |              |      Advanced satellites have larger range
  +--------------+
```

### 2.2 Radar Range Properties

Ships and stations have two radar range properties:

| Property | Type | Description |
|----------|------|-------------|
| `maxradarrange` | `length` | Maximum radar range of the object (from equipment/class) |
| `currentradarrange` | `length` | Current range accounting for `gravidarfactor` (reduced in nebulae) |

The game reads radar range via **vtable offset +8032** on the entity. The HUD renderer at `HUD_RenderRadarRangeRings` (`0x140DA1AA0`) calls this vtable method to get the base range, then multiplies by the local gravidar factor to compute effective range.

Radar range is affected by:
- **Ship class and equipment:** Each ship type has a base radar range in its macro definition.
- **Equipment mods:** `UIShipMod2.RadarRangeFactor` (float multiplier). Found in `equipmentmods.xml` (e.g., `mod_ship_radarrange_01_mk1`: +5% to +20%).
- **Gravidar factor:** Nebulae, anomalies, and "gravidar-obscuring regions" reduce the effective range. Queried per-position via `gravidarfactorat.{$position}` (available on cluster and sector objects).
- **Radar cloak factor:** `UIShipMod2.RadarCloakFactor` on mods can reduce how detectable a ship is.

### 2.3 Gravidar Factor (Region-Based Reduction)

Sectors can contain gravidar-obscuring regions. The `gravidarfactor` at any position ranges from 0.0 (no radar) to 1.0 (full radar). The effective radar range is:

```
effective_range = maxradarrange * gravidarfactor
```

Key properties:
- `$sector.hasgravidarobscuringregion` -- boolean, true if sector has any
- `$sector.gravidarfactorat.{$position}` -- float factor at specific position
- `$cluster.gravidarfactorat.{$position}` -- float factor at cluster level
- `$object.gravidarfactor` -- current factor for a scanning object

### 2.4 Gravidar Scan Events

```
event_gravidar_has_scanned
  - Fires when: a gravidar on an object completes a scan cycle
  - Parameter: object = the scanning object
  - Used by: AI combat scripts to check for nearby enemies
```

AI scripts use `find_gravidar_contact` to query what a specific object can detect:

```xml
<find_gravidar_contact name="$Contacts" object="this.ship"
    class="class.defensible" docked="false" maybeattackedby="this.ship"
    multiple="true">
    <match_distance object="this.ship" max="20km"/>
</find_gravidar_contact>
```

This is a **per-object query** that returns entities within that object's gravidar range, optionally filtered by distance, class, ownership, etc.

### 2.5 Gravidar Access (Shared Vision)

Diplomacy events can grant gravidar access to the player:

```xml
<add_player_gravidar_access_request object="$Destination"/>
<remove_player_gravidar_access_request object="$Destination"/>
```

When the player has gravidar access to an NPC object (e.g., via a trade deal or diplomacy), the player can see through that object's gravidar. This is used in `diplomacy.xml` when visiting foreign stations.

### 2.6 `isinliveview` and `cansee` Properties (IDA-Confirmed)

| Property | Meaning |
|----------|---------|
| `$object.cansee.{$target}` | True if the target is within this object's gravidar range |
| `$object.isinliveview` | True if the object is visible on the player's gravidar or by any player-owned object |

`isinliveview` is the high-level "is this entity currently detected by any player asset" check. It is more comprehensive than the simple `isradarvisible` byte because it accounts for gravidar access grants and player-owned assets.

#### IDA-Confirmed Internals (2026-03-26)

**Handler:** GetComponentData hash `0x32EDC1D9173B5D59` at `0x14023FDE6`. Requires type 71 (Object).
Calls `IsInLiveView` @ `0x140695B50`, returns boolean.

**`IsInLiveView` is NOT a byte read.** It is a three-branch priority evaluation:

```
IsInLiveView(component) @ 0x140695B50
===========================================

Branch 1: IsPlayerOwnedOrTracked(component)?  --> return TRUE
  |  Checks: owner == player faction
  |          OR is player's current ship (type 115)
  |          OR GlobalTrackTable_Lookup finds it (binary tree @ context+28544)
  |
Branch 2: vtable+0x1758(component)?  --> return component+968 byte
  |  (likely "is in same zone as player" spatial check)
  |  Reads stored byte at offset +0x3C8
  |
Branch 3: GetActiveMonitor() non-null?  --> return component+969 byte
  |  (remote observation/camera system active)
  |  Reads stored byte at offset +0x3C9
  |
Default: return FALSE
```

**Key offsets:**
| Offset | Type | Meaning | Persistence |
|--------|------|---------|-------------|
| +968 (`0x3C8`) | `BYTE` | Local gravidar visibility (set when entity scanned in player's zone) | **DYNAMIC** — clears when entity leaves gravidar range |
| +969 (`0x3C9`) | `BYTE` | Remote monitor visibility (set when entity visible via remote observation) | **DYNAMIC** — clears when remote observation ends |

Unlike the `+0x400` byte (which persists forever once set), these two bytes are **live proximity state** maintained by the gravidar system. They clear when the entity leaves detection range, making them true "currently visible" indicators.

**Implication:** Cannot replicate `isinliveview` by copying a byte. It is a computed property requiring player ownership state, zone proximity, and tracking table lookups. For replication, evaluate on host and transmit the boolean result.

### 2.7 Gravidar C++ Internals (IDA-Confirmed, Build 602526)

The gravidar system runs its own tick loop on a **worker thread**, independent of the UI thread. Key functions:

| Function | Address | Purpose |
|----------|---------|---------|
| `Gravidar_Update` | `0x141062030` | Main gravidar tick entry. Runs on worker thread. |
| `Gravidar_ScanProcessing` | `0x141062170` | Scan processing pass within the gravidar tick. |
| `Gravidar_UpdateInternal` | `0x141062580` | Internal update — uses **double-buffered state** to avoid tearing. |
| `Gravidar_ProcessContacts` | `0x14106C080` | Processes the live contact list after scan completes. |

**Threading model:** `Gravidar_Update` acquires a `CriticalSection` at **component+26976** (`0x6960`) before mutating state. This means the gravidar operates independently from the UI thread and must not be called directly from game function hooks.

**Double-buffered state:** `Gravidar_UpdateInternal` maintains two copies of the contact list, swapping them atomically so the holomap renderer always reads a consistent snapshot while the gravidar writes the next frame's results.

**Relationship to +0x400:** The gravidar system does NOT write the `+0x400` (`isradarvisible`) byte. It operates independently, maintaining its own live contact lists that are read directly by the holomap renderer. The `+0x400` byte is a persistent discovery flag managed by MD scripts and save/load (see Section 3.3). The `+0x3C8`/`+0x3C9` bytes (Section 2.6) are the live proximity state that the gravidar system updates.

---

## 3. Radar Scan Pipeline (IDA-Confirmed)

The radar scan is performed by the game engine, not by scripts. The key function pipeline:

```
RADAR SCAN PIPELINE
====================

  SaveLoader_MultiPass (0x1409A77B0)
        |
        |  (during load)
        v
  RadarScan_DiscoverEntities (0x140956660)    <-- main scan function
        |                                         4 callers total
        |  (iterates entities in range)
        v
  RadarScan_DispatchDiscovery (0x140956F80)   <-- per-entity dispatch
        |
        |  (checks discovery eligibility)
        v
  vtable+32 discovery callback                <-- game logic
        |
        v
  SetKnownToFaction (vtable+6016)             <-- marks entity known
        |
        v
  SetKnownToPlayer_Handler (0x1406E0640)      <-- events + achievements
```

### 3.1 RadarScan_DiscoverEntities (`0x140956660`)

**Size:** 2,331 bytes (550 instructions, 150 basic blocks)
**Callers:** 4 total:
1. `PostLoadProcessing` (`0x1409A4840`) -- after save load
2. `SaveLoader_MultiPass` (`0x1409A77B0`) -- during multi-pass load
3. `sub_1409B6CF0` (`0x1409B6CF0`) -- periodic sector update (called from `sub_1411CEB40`)
4. `sub_1411B1820` (`0x1411B1820`) -- universe import/initialization

**Logic:**
1. Gets the scanning entity's radar range by calling vtable+8032 (returns `double`)
2. If range <= 0, returns immediately (no radar = no scan)
3. Walks parent hierarchy to find the containing sector (type 86, via vtable+4536 type check loop)
4. If a sector is found, applies the gravidar factor: `effective_range = base_range * gravidar_factor`
5. Iterates a time-sorted red-black tree of entities (at global `qword_142EED5F0`)
6. For each entity within effective range:
   - Checks discovery eligibility via vtable+4488
   - Adds to the global discovery queue (`unk_143916190`, 56-byte entries, max 65536)
   - Each entry: entity pointer, discovery object pointer, timestamp
7. After iteration, calls `RadarScan_DispatchDiscovery` for each queued entity
8. Continues in a loop (the `do...while(v3)` at the end) if the discovery flag was set

**Key observation:** This function is called during LOAD and during periodic SECTOR UPDATE passes. It is NOT called every frame -- it runs when the game decides a sector needs its radar state refreshed.

### 3.2 RadarScan_DispatchDiscovery (`0x140956F80`)

For each discovered entity:
1. Gets entity type ID via vtable+8
2. Checks if the scanning object can detect this entity type (vtable+4488)
3. Builds a list of entities associated with the discovered one (via `sub_1409545F0`)
4. For each entity in the list, calls `sub_1409548D0` (discovery eligibility check)
5. If eligible, calls vtable+32 (the entity's discovery callback)

The global discovery queue at `qword_1438736B0` can hold up to 2048 inline entries; beyond that, a secondary heap buffer at `qword_143C961B8` is used.

### 3.3 `+1024` Byte Writers

The `isradarvisible` byte at component+1024 is written by:

1. **MD action `set_object_radar_visible`** (handler: `SetObjectRadarVisible_Action` at `0x140B8BD30`)
   - Evaluates condition expression via `EvalConditionToBool`
   - Writes result (0 or 1) to component+1024
   - Dispatches `RadarVisibilityChangedEvent`
   - Propagates to type-107 children

2. **Entity constructors** (initialization only)
   - Entity creation sets +1024 = 0 (default state)
   - Entity teardown/destruction zeros the entire QWORD at +1024 (vtable method at `0x1407429C0`)

**NOTE (IDA-confirmed, build 602526):** Exhaustive byte-pattern search (all x86-64 register
encodings for `mov byte/dword/qword [reg + 0x400]`) found exactly these four write sites:

| Address | Function | Action |
|---------|----------|--------|
| `0x140513322` | Entity constructor (with source) | Init to 0 |
| `0x140513930` | Entity constructor (default) | Init to 0 |
| `0x14068CE76` | `Object_Import` (`0x14068C760`) | Save/load deserialization: reads property 1287 |
| `0x140B8DDF0` | `SetObjectRadarVisibleAction_Execute` | MD script action write |

**There is NO gravidar/proximity code path that writes +0x400.** The byte is a **persistent
discovery flag** written ONLY by these three sources:
1. **Entity constructors** — initialize to 0
2. **`Object_Import`** — restore from save data (property 1287)
3. **`SetObjectRadarVisibleAction_Execute`** — MD script action `set_object_radar_visible`

No proximity detection, no gravidar scan, and no radar range check ever writes this byte.
It is NOT a proximity tracker.

The byte never clears to 0 during gameplay (no clearing code path found). Entities disappear
from the map because the **C++ holomap renderer** stops rendering them based on live gravidar
proximity checks (see Section 4), not because +0x400 is cleared.

Only 2 functions READ this byte:
- `LuaGlobal_GetComponentData` (`0x14023E190`) -- the Lua property dispatcher
- `MD_VariantPropertyEvaluator` (`0x140CA50E0`) -- MD condition expression evaluator

### 3.4 `+1025` Byte Writers

The forced radar visible byte at component+1025 is written by:

1. **`SetObjectForcedRadarVisible` FFI** (`0x14017F5A0`)
   - Type 71 (object class) check
   - Resolves component, calls `SetForcedRadarVisible_Internal`

2. **`SetForcedRadarVisible_Internal`** (`0x1406A3AC0`)
   - Writes byte at component+1025
   - Dispatches `RadarVisibilityChangedEvent`
   - Propagates to children (type 107 iteration)
   - Called from: FFI wrapper (above), MD action handler (`0x140B8BFA0`)

3. **MD action `set_object_forced_radar_visible`** (handler: `0x140B8BFA0`)
   - Evaluates condition, calls `SetForcedRadarVisible_Internal`

### 3.5 RadarVisibilityChangedEvent Dispatch Analysis (IDA-Confirmed, Build 602526)

**Vtable:** `RadarVisibilityChangedEvent` vtable at `0x142B40060`.

Only **3 vtable references** found in the entire binary:

| # | Function | Address | Purpose |
|---|----------|---------|---------|
| 1 | `RadarVisibilityChanged_BuildEvent` | case 375 in event switch | Serialization/deserialization factory for save/load |
| 2 | `SetObjectRadarVisibleAction_Execute` | `0x140B8DD80` | MD `set_object_radar_visible` action handler |
| 3 | `SetForcedRadarVisible_Internal` | `0x1406A50F0` | `SetObjectForcedRadarVisible` FFI internal |

**Critical finding:** Function #1 (`BuildEvent`) is a **serialization factory** used only during save/load event reconstruction. It is **NEVER called during normal gameplay** — it exists solely to rebuild event objects from serialized data.

Functions #2 and #3 create `RadarVisibilityChangedEvent` objects **inline** (stack-allocated, vtable pointer written directly) and dispatch them via `EventSource_DispatchEvent` at `0x140956B50`. There is no centralized "create event" factory for live radar changes.

**WARNING — Dead hook target:** Hooking `RadarVisibilityChanged_BuildEvent` (the case-375 factory) will NOT intercept live radar visibility changes. It only fires during save/load deserialization. To intercept live changes, hook `SetObjectRadarVisibleAction_Execute` and/or `SetForcedRadarVisible_Internal` directly.

---

## 4. Entity Visibility Lifecycle

Complete lifecycle from spawn to map/radar appearance:

```
ENTITY VISIBILITY LIFECYCLE
============================

  Entity Spawned (CreateShip / SpawnStationAtPos / etc.)
        |
        v
  Initial State:
    known_to_all = 0
    known_factions_count = 0
    radar_visible (+1024) = 0
    forced_radar_visible (+1025) = 0
        |
        |--- Owner is player? ---> IsObjectKnown = TRUE (condition 2: owner check)
        |                          BUT NOT radar-visible yet!
        |
        |--- SetKnownTo(id, "player") called?
        |         |
        |         v
        |    known_factions_count = 1
        |    IsObjectKnown = TRUE (condition 3)
        |    SetKnownToPlayerEvent dispatched
        |    Achievement counters updated
        |    Children propagated (stations -> modules)
        |         |
        |         v
        |    Entity appears in GetAllFactionStations / GetAllFactionShips queries
        |
        |--- Entity enters player radar range?
        |         |
        |         v
        |    Gravidar system detects entity (live contact list)
        |    +0x3C8 / +0x3C9 bytes set (dynamic, see Section 2.6)
        |    Holomap renderer sees entity via live proximity check
        |    NOTE: +1024 byte is NOT written by proximity code.
        |      It is only set by MD scripts (set_object_radar_visible).
        |         |
        |         v
        |    MAP CHECK: isknown AND isradarvisible
        |    If BOTH true: entity appears on map and HUD
        |
        |--- SetObjectForcedRadarVisible(id, true) called?
        |         |
        |         v
        |    +1025 = 1
        |    RadarVisibilityChangedEvent dispatched
        |    Entity treated as radar-visible REGARDLESS of +1024
        |         |
        |         v
        |    MAP CHECK passes (forced flag overrides normal check)
        |    Entity appears on map/HUD
        |
        |--- Entity leaves radar range?
        |         |
        |         v
        |    +0x3C8 / +0x3C9 bytes CLEAR (dynamic proximity state)
        |    Holomap renderer stops showing entity (live gravidar check fails)
        |    +1024 byte is NOT cleared (persists forever, no clearing code path)
        |    Entity disappears from MAP because renderer uses live proximity,
        |      NOT the +1024 byte
        |    Still "known" (known-factions persists)
        |    Entity remains in sector queries, just not displayed on map
        |
        v
  Entity Destroyed / Removed
    All state cleared with component (QWORD zero at +1024)
```

**Map display mechanism:** The C++ holomap renderer (`GetMapRenderedComponents` at `0x1401F3250`)
maintains its own list of entities to display, computed from live gravidar proximity checks.
When an entity leaves radar range of all player assets, the holomap stops rendering it. The
Lua `isObjectValid` filter (`isknown AND isradarvisible`) is a secondary check for the sidebar
object list. Both must pass for an entity to appear in the sidebar, but the holomap rendering
is the primary visibility gate for map icons.

**Critical implication:** An entity can be "known" (isknown = true) and still have +1024 = 1
after leaving radar range (the byte persists). The entity disappears from the map because
the holomap renderer stops showing it, not because +1024 is cleared. The entity is still
enumerable via `GetAllFactionShips` etc.

---

## 5. Data Flow Diagram

```
COMPLETE DATA FLOW: Entity -> Map Display
==========================================

  +-------------------+
  | ENTITY IN SECTOR  |
  +--------+----------+
           |
     +-----+------+
     |             |
     v             v
+----------+  +-----------+
| KNOWN TO |  | IN RADAR  |
| PLAYER?  |  | RANGE?    |
+----+-----+  +-----+-----+
     |               |
     v               v
+----------+  +-----------+       +--------------------+
| known_to |  | +1024 byte|  OR   | +1025 forced byte  |
| count > 0|  | == 1      |       | == 1               |
| OR owner |  +-----------+       +--------------------+
| == player|        |                     |
+----+-----+        +----------+----------+
     |                         |
     v                         v
+----------+            +-------------+
| isknown  |            | isradarvis  |
| == true  |            | == true     |
+----+-----+            +------+------+
     |                         |
     +------+     +------------+
            |     |
            v     v
    +-------------------------------+
    | PRIMARY GATE:                 |
    | C++ Holomap Renderer          |
    | (GetMapRenderedComponents)    |
    | Uses live gravidar proximity  |
    | checks, NOT the +1024 byte   |
    +--------+----------------------+
             |
     +-------+-------+
     |               |
     v               v
  RENDERED       NOT RENDERED
     |               |
     v               v
+-----------+    +-----------+
| SECONDARY |    | HIDDEN    |
| FILTER:   |    | from map  |
| isknown   |    | (no icon, |
| AND       |    | no sidebar|
| isradarvis|    | entry)    |
| (Lua side-|    +-----------+
| bar only) |
+-----+-----+
      |
  +---+---+
  |       |
  v       v
PASS    FAIL
  |       |
  v       v
+------+ +--------+
|SHOWN | |HIDDEN  |
|in    | |from    |
|sidebar |sidebar |
+------+ +--------+

PARALLEL (INDEPENDENT):

+-----------+          +-------------------+
| 3D RENDER |          | FACTION DISCOVERY |
| Purely    |          | <set_faction_     |
| spatial.  |          |  known> (MD only) |
| Loaded    |          +-------------------+
| zones     |                  |
| always    |                  v
| render.   |          +-------------------+
+-----------+          | Diplomacy UI      |
                       +-------------------+

+-----------+
| ENCYCLOP. |
| AddKnown  |
| Item (Lua)|
+-----------+
      |
      v
+-----------+
| Encyclop. |
| UI screen |
+-----------+
```

---

## 6. Satellite and Deployable Radar Coverage

### 6.1 Satellite Types

X4 has standard and advanced satellites. Both are deployed objects (type 71, subclass satellite) that provide radar coverage.

| Property | API |
|----------|-----|
| `IsSatellite(id)` | C FFI -- checks if entity is a satellite |
| `IsAdvancedSatellite(id)` | C FFI -- checks if advanced type |
| `GetAllSatellites(result, len, defensibleid)` | C FFI -- list satellites on a ship |
| `GetNumAllSatellites(defensibleid)` | C FFI -- count satellites on a ship |
| `LaunchSatellite(defensibleid, macroname)` | C FFI -- deploy a satellite |

### 6.2 Satellite Coverage Model

Deployed satellites function as fixed radar sources. They:
1. Have a fixed `maxradarrange` determined by their macro
2. Are affected by `gravidarfactor` at their deployment position
3. Provide radar coverage to their owner faction
4. Are marked with `SetObjectForcedRadarVisible` on deployment (always visible on owner's map)

The map has a UI toggle `"rendersatelliteradarrange"` (default: ON) that draws satellite radar range circles. This calls `C.SetMapRenderSatelliteRadarRange(holomap, bool)`.

### 6.3 Player Coverage

The `iscovered` property (used by `Helper.isPlayerCovered()` in the UI) checks if the player is currently within the radar coverage of ANY player-owned asset:

```lua
function Helper.isPlayerCovered()
    local player = ConvertStringTo64Bit(tostring(C.GetPlayerID()))
    if player and (player ~= 0) then
        return GetComponentData(player, "iscovered")
    end
    return false
end
```

When `iscovered` is true, the map shows additional information. The map also has a `SetUICoverOverride` checkbox that forces full information display even without coverage.

### 6.4 Satellite Coverage and Multiplayer

Satellites are deployed per-player and are world objects. In a host-client model:
- The HOST has satellites deployed in the world
- The CLIENT does not have the same satellites
- Replicated entities should use `SetObjectForcedRadarVisible` rather than relying on satellite coverage
- The `iscovered` property on the client will reflect the CLIENT's own coverage, not the host's

---

## 7. Sector-Level Visibility

### 7.1 Sector Discovery

Sectors become "known" when:
1. `SetKnownTo(sector_id, "player")` is called (explicit discovery)
2. The player's ship enters the sector (automatic discovery via `DiscoverComponent_Internal`)
3. MD script calls `<set_known object="$Sector" known="true"/>`
4. A player-owned entity is spawned in the sector (triggers `UpdatePlayerOwnedTracking`)

**Requirement:** A sector's parent CLUSTER must also be known. `GetSectors(cluster)` filters by known-to-player. If the cluster is unknown, the sector won't appear even if it is marked known.

### 7.2 Known vs Explored

"Known" and "explored" are independent concepts:

| Concept | Meaning | Storage | Filter API |
|---------|---------|---------|------------|
| **Known** | Player knows the sector exists | Faction array on component | `GetClusters(false)` / `GetSectors(id)` always filter by known |
| **Explored** | Player has visited the sector | Macro defaults at +400 | `GetClusters(true)` adds explored filter |

The explored flag is stored in **macro defaults** (offset +400 on the macro defaults structure), NOT on the component instance. This means it persists as part of macro data.

`GetClusters(onlyexplored)` and `GetSectors(sectorid)`:
- With `false`: returns all sectors known to the player
- With `true`: returns only sectors that are both known AND explored

The `IsExploredCheck` virtual (vtable+6344, `0x1407B2DC0`) reads this flag from macro defaults. Only implemented for Space-class entities (clusters, sectors), not for stations/ships.

### 7.3 `GetClusters` and `GetSectors` Filters

**Critical behavior (IDA-confirmed):** `GetClusters` (Lua global at `0x140262FC0`) and `GetSectors` (at `0x140263220`) ALWAYS filter by "known to player faction", even when called with `false`. The boolean parameter only controls the ADDITIONAL "explored" filter. Components created by `AddCluster`/`AddSector` are NOT automatically marked as "known" -- that must be done separately.

---

## 8. 3D Rendering and Radar

### 8.1 3D Rendering Is Purely Spatial

Entities in loaded zones ALWAYS render in 3D regardless of their known/radar state. There is no visibility filter at the 3D rendering level based on known-to or radar flags. If a zone is loaded (player is near enough), all entities in it render.

The game uses a zone/LOD system for spatial management:
- **Loaded zones:** Full 3D simulation and rendering
- **Unloaded zones:** OOS (out-of-sector) simulation only

The transition between loaded and unloaded is based on player proximity and game engine decisions, not on known/radar state.

### 8.2 Cockpit Radar Widget

The cockpit HUD radar is rendered by the radar render target system:

| Function | Purpose |
|----------|---------|
| `SetRadarRenderTarget2(texture, ref_component, mapmode)` | Set up radar render |
| `SetRadarRenderTargetOnTarget2(texture, focus, ref_component, mapmode)` | Radar focused on target |
| `UnsetRadarRenderTarget()` | Clear radar render |
| `SetRadarMousePosition(x, y)` | Mouse interaction with radar |
| `HandleTargetMouseClickOnRadar(instant, interact)` | Click handling |
| `IsHUDRadarActive()` | Check if radar HUD is on |
| `SetHUDRadarActive(bool)` | Toggle radar HUD |
| `GetHUDSeparateRadar()` | Is radar in separate mode |

The cockpit radar shows entities based on the same gravidar detection model -- entities within the player ship's radar range appear as blips. The HUD render ring function `HUD_RenderRadarRangeRings` (`0x140DA1AA0`) draws the range circles.

### 8.3 Render Distance / LOD

X4 uses a standard LOD system where entity detail decreases with distance. Far entities render as simple icons or dots. This is an engine-level rendering decision, not related to the visibility/known system.

---

## 9. Object Layout -- Visibility-Related Fields

### Station/Ship (Object Class, Type 71)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +840 | 8 | `owner_faction_ptr` | Owner faction context pointer |
| +857 | 1 | `known_read` | Encyclopedia "read" flag (byte) |
| +858 | 1 | `known_to_all` | Global known flag (byte, not normally set) |
| +864 | 16 | `known_factions_arr` | Small-array-optimized faction pointer list |
| +880 | 8 | `known_factions_cap` | Array capacity (2 = inline, >2 = heap ptr at +864) |
| +888 | 8 | `known_factions_count` | Number of factions in known-to list |
| +1024 (0x400) | 1 | `radar_visible` | Normal radar visibility byte |
| +1025 (0x401) | 1 | `forced_radar_visible` | Forced radar visibility byte (overrides +1024) |

### Cluster/Sector (Space Class)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +800 | 8 | `owner_faction_ptr` | Owner faction context pointer |
| +817 | 1 | `known_read` | Encyclopedia "read" flag |
| +818 | 1 | `known_to_all` | Global known flag |
| +824 | 16 | `known_factions_arr` | Small-array-optimized faction pointer list |
| +840 | 8 | `known_factions_cap` | Array capacity |
| +848 | 8 | `known_factions_count` | Faction count |
| N/A | | explored | Stored in MACRO DEFAULTS at +400, not component |

### Small-Array Optimization

When `cap == 2`, the two faction pointers are stored inline at `known_factions_arr`. When `cap > 2`, the first 8 bytes become a heap pointer to the array. The `IsKnownToFaction` check branches on `cap == 2` to determine whether to dereference.

---

## 10. Function Reference

### C FFI Exports

| Function | Address | Signature | Notes |
|----------|---------|-----------|-------|
| `SetKnownTo` | `0x14017F0D0` | `void(UniverseID, const char* factionid)` | Hashes factionid via FNV-1a, looks up in `g_FactionRegistry`, calls vtable+6016. Use `"player"`. |
| `IsObjectKnown` | `0x140ABF6E0` | `bool(UniverseID)` | Calls vtable+5976 |
| `IsKnownToPlayer` | `0x140179D00` | `bool(UniverseID)` | Calls vtable+5992 with `g_PlayerFactionContext` |
| `SetObjectForcedRadarVisible` | `0x14017F5A0` | `void(UniverseID, bool)` | Sets +1025, dispatches event, propagates to children. Type 71 only. |
| `IsKnownRead` | `0x140179CB0` | `bool(UniverseID)` | Calls vtable+6024 |
| `SetKnownRead` | `0x14017F080` | `void(UniverseID, bool)` | Calls vtable+6032 |
| `SetComponentOwner` | `0x14017E170` | `void(UniverseID, const char* factionid)` | Changes +840 owner. NEVER modifies known state. |
| `GetNumContainedKnownSpaces` | `0x140160B50` | `uint32_t(UniverseID)` | Walks child spaces, filters by known + explored |
| `ReadAllKnownItems` | `0x14017C490` | `void()` | Marks all encyclopedia items as "read" |
| `GetComponentKnownName` | `0x140150AE0` | `const char*(UniverseID)` | Display name when known |
| `IsSatellite` | `0x14017xxxx` | `bool(UniverseID)` | Check if entity is a satellite |
| `IsAdvancedSatellite` | `0x14017xxxx` | `bool(UniverseID)` | Check if advanced satellite |
| `LaunchSatellite` | `0x14017xxxx` | `void(UniverseID, const char*)` | Deploy satellite from ship |
| `GetAllSatellites` | `0x14017xxxx` | `uint32_t(AmmoData*, uint32_t, UniverseID)` | List satellites on defensible |
| `SetMapRenderSatelliteRadarRange` | in FFI | `void(UniverseID, bool)` | Toggle satellite range rings on map |

### Lua-Only Functions (NOT C FFI)

| Function | Address | Notes |
|----------|---------|-------|
| `AddKnownItem(library, itemid)` | `0x140262AF0` | Adds to player encyclopedia |
| `IsKnownItem(library, itemid)` | `0x140262D70` | Checks encyclopedia |
| `GetComponentData(id, ...)` | `0x14023E190` | Master property dispatcher (32KB, FNV-1 hash dispatch) |

### Internal Functions (IDA-Named)

| Function | Address | Size | Purpose |
|----------|---------|------|---------|
| `IsRadarVisible_ReadByte` | `0x14011BE30` | small | Returns `*(uint8_t*)(component + 0x400)` |
| `SetObjectRadarVisible_Action` | `0x140B8BD30` | ~400B | MD action handler for `set_object_radar_visible` |
| `MDAction_set_object_forced_radar_visible` | `0x140B8BFA0` | ~77B | MD action handler for `set_object_forced_radar_visible` |
| `SetForcedRadarVisible_Internal` | `0x1406A3AC0` | ~350B | Writes +1025, dispatches event, propagates children |
| `EvalConditionToBool` | `0x140C7F620` | | MD condition evaluator (232 callers) |
| `RadarScan_DiscoverEntities` | `0x140956660` | 2331B | Main radar scan. Iterates entities in range, queues discovery. |
| `RadarScan_DispatchDiscovery` | `0x140956F80` | ~500B | Dispatches discovery for a single entity, builds entity lists |
| `RadarScan_ProcessDiscoveryList` | `0x140957250` | ~900B | Processes discovery list with critical section (thread-safe), checks scan ranges |
| `HUD_RenderRadarRangeRings` | `0x140DA1AA0` | 1489B | Radar range ring renderer. vtable+8032 for range. Builds shader params. |
| `SetKnownToPlayer_Handler` | `0x1406E0640` | ~1050B | Dispatches SetKnownToPlayerEvent, propagates to children, achievement counters |
| `UpdatePlayerOwnedTracking` | `0x14069F290` | | Zone hierarchy known-state (asymmetric: gain=propagate, lose=no-op) |
| `FNV1_Hash64` | `0x1400EA3B0` | | Property name hash for GetComponentData dispatch |
| `MD_VariantPropertyEvaluator` | `0x140CA50E0` | 0x1F053 | MD condition/property evaluator (reads `isradarvisible` for conditions) |
| `Gravidar_Update` | `0x141062030` | | Main gravidar tick (worker thread, CriticalSection at component+26976) |
| `Gravidar_ScanProcessing` | `0x141062170` | | Scan processing pass within gravidar tick |
| `Gravidar_UpdateInternal` | `0x141062580` | | Internal update with double-buffered state |
| `Gravidar_ProcessContacts` | `0x14106C080` | | Processes live contact list after scan |
| `EventSource_DispatchEvent` | `0x140956B50` | | Generic event dispatch (used by radar visibility events) |
| `SetObjectRadarVisibleAction_Execute` | `0x140B8DD80` | | MD action execute for `set_object_radar_visible` (dispatches RadarVisibilityChangedEvent) |
| `IsInLiveView` | `0x140695B50` | | Three-branch priority eval: player-owned/tracked, zone proximity (+0x3C8), remote monitor (+0x3C9) |

### Known-State Vtable Map

All offsets from primary vtable base.

| Vtable Offset | Name | Signature | Station/Ship Impl | Cluster/Sector Impl |
|---|---|---|---|---|
| +5976 (0x1758) | `IsObjectKnown` | `bool(self)` | `0x140694190` | `0x1407AB4F0` |
| +5984 (0x1760) | `IsKnownToAnyFaction` | `bool(self, factionListCtx)` | `0x140694230` | `0x1407AB590` |
| +5992 (0x1768) | `IsKnownToFaction` | `bool(self, factionCtx)` | `0x1406941C0` | `0x1407AB520` |
| +6000 (0x1770) | `SetKnownToAll` | `void(self, bool)` | `0x140694240` | `0x1407AB5A0` |
| +6008 (0x1778) | `SetKnownToFactions` | `void(self, factionArr, bool)` | `0x140694350` | `0x1407AB6B0` |
| +6016 (0x1780) | `SetKnownToFaction` | `void(self, factionCtx, bool)` | `0x1406942C0` | `0x1407AB620` |
| +6024 (0x1788) | `IsKnownRead` | `bool(self)` | `0x1406944B0` | `0x1407AB810` |
| +6032 (0x1790) | `SetKnownRead` | `void(self, bool)` | `0x1406944C0` | `0x1407AB820` |
| +6344 (0x18C8) | `IsExploredCheck` | `bool(self, int)` | N/A | `0x1407B2DC0` |
| +8032 | `GetRadarRange` | `double(self)` | impl varies | N/A |

### Ownership Vtable (Station vs Ship)

| Vtable Offset | Purpose | Station | Ship | Same? |
|--------|---------|---------|------|-------|
| +5696 | `OnChangedTrueOwner` | `0x1407B7300` | `0x14078D860` | **Different** |
| +5720 | `SetOwner` | `0x140693D30` | `0x140693D30` | Same |
| +6016 | `SetKnownToFaction` | `0x1406942C0` | `0x1406942C0` | Same |

Station vtable: `0x142B05590`. Ship vtable: `0x142B13980`.

---

## 11. Event/Hook Points

### Events Dispatched

| Event Class | RTTI | When |
|-------------|------|------|
| `SetKnownToPlayerEvent` | `0x1431A7360` | Entity becomes known to player faction |
| `PlayerDiscoveredComponentEvent` | `0x1431B7340` | Player proximity discovers an entity (includes discovery_level int) |
| `RadarVisibilityChangedEvent` | in `SetForcedRadarVisible_Internal` | Forced or normal radar visibility toggled |
| `ChangedOwnerEvent` | in `SetOwner` | Entity owner changed |
| `ChangedTrueOwnerEvent` | in `SetOwner` | Entity true owner changed |

### MD Events

| Event | Fires When |
|-------|------------|
| `event_gravidar_has_scanned` | A gravidar completes a scan cycle on an object |
| `event_player_discovered_object` | Player discovers an object (proximity, scan) |

### Hookable Functions

| Function | What You Get | Best For |
|----------|-------------|----------|
| `SetKnownToFaction` (vtable+6016) | ALL discovery events from all code paths | Intercepting discovery on host |
| `SetObjectForcedRadarVisible` (FFI export) | Forced visibility changes | Intercepting forced visibility |
| `SetObjectRadarVisible_Action` (MD action) | Normal radar visibility changes | Less useful (MD only) |
| `RadarScan_DiscoverEntities` | The entire scan sweep | Very invasive, not recommended |

**Single best hook:** `SetKnownToFaction` (vtable+6016) captures automatic discovery, MD `<set_known>`, and FFI `SetKnownTo` -- all three paths that mark entities as known.

---

## 12. Global State References

| Global | Address | Purpose |
|--------|---------|---------|
| `g_PlayerFactionContext` | `0x1438776C8` | Player faction context pointer (863 refs) |
| `g_FactionRegistry` | (in `SetKnownTo`) | BST of faction contexts, keyed by FNV-1a hash |
| `g_ComponentRegistry` | `0x143877670` | Component lookup (all FFI exports) |
| `g_GameUniverse` | `0x14314CE48` | Universe singleton (galaxy at +552, player at +560) |
| `qword_142EED5F0` | `0x142EED5F0` | Radar scan entity tree sentinel |
| `qword_142EED5F8` | `0x142EED5F8` | Radar scan entity tree root |
| `qword_142EED618` | `0x142EED618` | Radar scan entity tree count |
| `qword_1438736B0` | `0x1438736B0` | Discovery queue (inline, 2048 entries) |
| `qword_143C961B8` | `0x143C961B8` | Discovery queue (overflow heap) |

---

## 13. GetComponentData Property Dispatch

`GetComponentData` at `0x14023E190` is a 32KB Lua C function (~6923 instructions) that dispatches property reads via FNV-1 hash.

### Hash Algorithm

**FNV-1** (NOT FNV-1a). Multiply THEN xor.

- Seed: `0x811C9DC5`
- Prime: `0x01000193`
- 64-bit accumulator despite 32-bit constants

Assembly: `imul rax, 0x1000193` then `xor rax, r9`.

### Key Property Hashes

| Property | FNV-1 Hash | Handler Address | Reads |
|----------|-----------|-----------------|-------|
| `isradarvisible` | `0xF601BD3789210E2B` | `0x1402457B0` | `*(uint8_t*)(component + 0x400)` via `IsRadarVisible_ReadByte` |
| `isknown` | `0xF662FE1ABE10E84E` | `0x1402457FE` | vtable+5976 (`IsObjectKnown`) |

The `isradarvisible` handler checks type 71 (object class) first, then casts and reads.

---

## 14. Scan State / Reveal Percentage (Third Visibility Layer)

> **Date:** 2026-03-27 | **Source:** IDA + game script analysis

X4 has **three independent visibility layers** that ALL must be set for full map/UI presentation:

| Layer | Controls | API | Default |
|-------|----------|-----|---------|
| **Known** | Entity appears on map | `SetKnownTo(id, "player")` / MD `set_known` | Unknown |
| **Radar** | Gravidar detection, live tracking | `SetObjectForcedRadarVisible(id, true)` | Not visible |
| **Scanned** | Info reveal %, icon brightness, entity name | MD `set_object_scanned` only | 0% revealed |

### Reveal Percentage

Each entity has a `revealedpercentage` (float, 0.0-1.0). The game's `infounlocklist.xml` defines thresholds per info type:

| Info Type | Required % | Effect if unmet |
|-----------|-----------|-----------------|
| Entity name | 1% | Shows "Unknown Object" |
| Hull/shield | ~25% | Hidden in info panel |
| Cargo | ~50% | Hidden in trade UI |
| Full details | 100% | Some stats hidden |

### Map Icon Brightness

The holomap renderer and `menu_map.lua` use reveal percentage for icon rendering:
- **0% revealed:** `Color["text_inactive"]` (dim/grey icon)
- **Partially revealed:** Intermediate alpha
- **100% revealed:** Full brightness icon

`IsInfoUnlockedForPlayer` at `0x14017a870` checks vtable+5976 (has scan data?) then calls `CheckInfoUnlockThreshold` at `0x140605350` which reads reveal level via vtable+6416 and compares against per-info-type thresholds from `infounlocklist.xml`.

### Reveal Percentage Is Computed, Not Stored

**Critical finding:** The reveal percentage is NOT a stored field on entities. It is **computed dynamically** from two values:
1. **Scan level** (int, 0-6) -- stored in a red-black tree at `engine+77792`, keyed by entity ID
2. **Secrecy level** (int, 0-3) -- static per entity type, from macro data at offset +1216

The percentage is looked up from a global 4x4 table at `qword_146C7A4C0`, loaded from `infounlocklist.xml`:

| | scan=0 | scan=1 | scan=2 | scan=3+ |
|---|---|---|---|---|
| **secrecy=0** | 100 | 100 | 100 | 100 |
| **secrecy=1** | 50 | 100 | 100 | 100 |
| **secrecy=2** | 33 | 66 | 100 | 100 |
| **secrecy=3** | 25 | 50 | 75 | 100 |

Formula: `percentage = table[scan_level + 4 * secrecy_level]`, capped at 100. Player-owned entities always return 100.

**Stations** cache the aggregate percentage at entity offset +3432 (DWORD). This is the "average revealed percentage of all info points" from scriptproperties.xml. Each station module has its own scan state.

### Key Functions (IDA-confirmed)

| Function | Address | Role |
|----------|---------|------|
| Object_GetRevealPercentage | `0x1404E7A10` | vtable[802]: computed reveal % |
| Ship_GetRevealPercentage | `0x1404053B0` | Ship override: thin wrapper |
| Station_GetRevealPercentage | `0x1407AA8E0` | Station: cached aggregate at +3432 for level=-1 |
| GetEntityScanLevel | `0x1406EB860` | Reads scan level from RB tree (engine+77792) |
| GetSecrecyLevel | `0x1404E79C0` | vtable[789]: static from macro data +1216 |
| ApplyScanToEntity | `0x1406EB900` | **Core setter:** validates + schedules reveal + sets known |
| ScanEntity_Full | `0x1406F6500` | Full scan for station modules |
| GetPlayerScannerLevel | `0x1406F5C50` | Player's max scan level from equipment |
| SetObjectScannedAction_Execute | `0x140B92150` | MD action handler |
| ScheduleScanReveal | `0x1406E04F0` | Creates timed reveal in ScanManager |
| CheckInfoUnlockThreshold | `0x140605350` | Compares reveal% against info thresholds |

### `set_object_scanned` (MD Action)

MD script action (no direct C++ FFI equivalent):
```xml
<set_object_scanned object="$entity" />
```

**Correction:** Does NOT always set to 100%. Sets scan level to the **player's scanner equipment level** (from `GetPlayerScannerLevel`). The resulting percentage depends on the entity's secrecy level. For secrecy-3 entities with a level-0 scanner, this only achieves 25%.

Handler: `SetObjectScannedAction::Execute` at `0x140B92150`:
- For single objects (type 2304): triggers scan visual + dispatches PlayerDetectedObjectEvent
- For stations: iterates ALL child modules, calls `ScanEntity_Full(engine, module, scanner_level)` per module

Internal path: `ScanEntity_Full` -> `ApplyScanToEntity(engine, entity, scan_level)` -> `ScheduleScanReveal(&entityID, 2, scan_level)` -> creates timed entry in ScanManager at `qword_146C7AEC0`.

### C++ Direct Scan (Alternative to MD)

`ApplyScanToEntity` at `0x1406EB900` can be called directly:
```
Signature: bool ApplyScanToEntity(int64 engine, int64 entity, int scan_level)
```
- `engine`: from `*(qword_143CA6D68 + 560)` or `qword_143CA27B8`
- Validates: entity alive (+104==0), not player-owned, permission check
- Calls ScheduleScanReveal, SetKnownTo, dispatches PlayerDiscoveredComponentEvent
- Best hook point for intercepting scan events (delta replication)

### Vanilla Pattern

Every entity the game spawns for the player gets all three calls:
```xml
<set_known object="$entity" />
<set_object_scanned object="$entity" />
<!-- optionally: -->
<set_object_forced_radar_visible object="$entity" />
```

### Implication for Multiplayer

**Host side:** Hook `ApplyScanToEntity` (`0x1406EB900`) to detect scan level changes. Broadcast `(entity_id, scan_level)` as a compact int (3 bits, 0-6).

**Client side:** Use MD cue `<set_object_scanned object="$entity" />` for V1 (sets to client's scanner level). For exact fidelity, call `ApplyScanToEntity` directly with the host's scan level.

Proxy entities on the client must call `set_object_scanned` via an MD cue after spawn. Without it, replicated entities appear dim on the map and show "Unknown Object" instead of their real name.

---

## 15. Encyclopedia / Faction Discovery

### Encyclopedia Known-Item System

The player entity stores 50 "library" categories at offset `+24*(categoryIdx + 1177)`. Each category is a vector of 16-byte entries (bytes 0-7: item identifier, byte 8: "read" flag).

| Function | API | Notes |
|----------|-----|-------|
| `AddKnownItem(library, itemid)` | Lua global only | Adds item to encyclopedia |
| `IsKnownItem(library, itemid)` | Lua global only | Checks if item known |
| `ReadAllKnownItems()` | C FFI | Marks all items as read |
| `IsKnownItemRead(library, itemid)` | Lua global only | Checks read flag |

Auto-population: `targetmonitor.lua` calls `AddKnownItem` when the player targets entities.

### Faction Discovery (MD-Only)

`<set_faction_known faction="..." known="true"/>` -- no C FFI equivalent. This is separate from per-entity known-to and from encyclopedia entries. All three must be set for complete faction awareness. See `X4Native/docs/rev/FACTION_RELATIONS.md`.

---

## 16. Edge Cases

### AI-Suppressed Ships

Ships with suppressed AI (e.g., proxies without assigned pilots) do NOT provide radar coverage. Radar coverage requires an active scanning system, which requires the ship to be "operational" with crew. However, deployed satellites do not require crew -- they provide coverage by existing.

### Unpowered / Damaged Stations

Stations provide radar coverage through their radar modules. If the station is severely damaged but still exists as an entity, it continues to provide coverage (the radar range is a property of the macro, not of operational status).

### Forced Radar vs Normal Radar

Setting `SetObjectForcedRadarVisible(id, true)` does NOT modify the +1024 byte. Both bytes are independent. The map filter logic treats the entity as radar-visible if EITHER byte is nonzero. The forced flag is specifically designed for entities that should always appear (satellites, nav beacons, mission objects).

### Ownership Change and Known State

Changing entity ownership (`SetComponentOwner`) NEVER modifies the known-to list. An entity that was known to the player before ownership change remains known after. However, `UpdatePlayerOwnedTracking` asymmetry means zones may lose their loaded state (see Section 16.3).

### Zone Known State

Zones (tempzones) that contain player-owned entities are marked known via `UpdatePlayerOwnedTracking`. If the entity loses player ownership, the zone is NOT unmarked -- but nothing else keeps the zone loaded, so the station may become invisible in 3D (still on map though).

---

## 17. Replication Implications

For multiplayer mods that replicate visibility across instances. This section is factual about what the engine provides -- specific mod implementation choices are outside this document's scope.

### 16.1 Making Spawned Entities Visible

Call `SetKnownTo(id, "player")` + `SetObjectForcedRadarVisible(id, true)` to guarantee map/radar visibility. The forced flag at +1025 bypasses the normal +1024 flag entirely.

### 16.2 Reading Host Visibility State

No C FFI reader for `isradarvisible`. Options:
1. **Direct memory read:** `*(uint8_t*)(component + 0x400)` via `x4n::visibility::get_radar_visible()` -- fast but version-fragile
2. **`is_map_visible()`:** Combined check (IsObjectKnown AND radar/forced byte) via `x4n::visibility::is_map_visible()`
3. **IsObjectKnown proxy:** Use `IsObjectKnown(id)` (FFI) as primary filter for persistent discovery state.

**NOTE:** The +1024 byte may persist after entities leave radar range (no engine clearing found).
Reading +1024 therefore tells you "has this entity ever been radar-scanned", not "is this entity
currently in radar range." For current-range detection, the holomap rendering system uses live
gravidar proximity checks internally. There is no public API to query "is entity currently in
gravidar range of any player asset" from C++. MD scripts can use `$object.isinliveview` for this.
The C++ implementation is `IsInLiveView` @ `0x140695B50` — see Section 2.6 for the three-branch algorithm.

### 16.3 Station Ownership and Visibility

NPC-owned stations spawned via `SpawnStationAtPos` are invisible due to the `UpdatePlayerOwnedTracking` asymmetry. After `SetComponentOwner(station, npc_faction)`, manually re-apply `SetKnownTo` on the station + zone + sector hierarchy.

### 16.4 Satellite Coverage Divergence

Host and client have different deployed satellites. Client cannot rely on host satellite coverage for `iscovered` or normal `isradarvisible`. Use `SetObjectForcedRadarVisible` on client to bypass this entirely.

### 16.5 Sector Discovery Requirement

Sectors must have their parent cluster also marked known. Gates and highways need individual `SetKnownTo` calls. See `lib_generic.xml` `SetupInitialMap` pattern.

### 16.6 Hook Opportunities

- **`SetKnownToFaction` (vtable+6016):** Single hook captures ALL discovery from all code paths.
- **`RadarVisibilityChangedEvent`:** Subscribe via event system for radar changes. **WARNING:** Do NOT hook `RadarVisibilityChanged_BuildEvent` (case 375 in the event switch) — it is a save/load serialization factory and never fires during normal gameplay. Hook `SetObjectRadarVisibleAction_Execute` (`0x140B8DD80`) or `SetForcedRadarVisible_Internal` directly instead. See Section 3.5.
- **`SetObjectForcedRadarVisible` (FFI):** Hook to intercept forced visibility changes.

### 16.7 Batch Enumeration

No "get all known objects" API exists. Enumerate per-faction: `GetAllFactionStations(faction)` / `GetAllFactionShips(faction)`, then filter by `IsObjectKnown()`.

---

## Related Documents

| Document | Contents |
|----------|----------|
| [SUBSYSTEMS.md](SUBSYSTEMS.md) Section 15 | Known-state overview (cross-references this doc) |
| [FACTION_RELATIONS.md](FACTION_RELATIONS.md) | Faction relation system, three "known" subsystems for factions |
| [STATE_MUTATION.md](STATE_MUTATION.md) | Safe state mutation patterns |
| [OOS_SIMULATION.md](OOS_SIMULATION.md) | Out-of-sector simulation (loaded vs unloaded zones) |
