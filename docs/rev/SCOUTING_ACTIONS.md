# Research: Scouting, Exploration, and Intel-Gathering Actions

**Date**: 2026-04-01
**Topic**: Complete inventory of scouting/intel game systems and V4 action catalog alignment
**Conclusion**: `order_recon` + `deploy_satellites` cover the faction AI use cases.
**Sources Used**: Game AI scripts, MD faction logic, SDK function list

---

## 1. Complete Inventory of Scouting/Intel Systems in X4

### 1.1 AI Script Orders (Game-Defined)

The game defines **five** order-level scripts in the recon family. They all share a common engine -- `order.move.recon.xml` -- with different wrapper scripts controlling the mode:

| Order ID | Script File | Category | Purpose |
|----------|-------------|----------|---------|
| `Recon` | `order.move.recon.xml` | `internal` | Core recon engine. Scans targets, reports threats, deploys satellites/probes. Not directly user-accessible. |
| `Explore` | `order.move.recon.explore.xml` | `navigation` | Player/NPC sector exploration. Wraps Recon with `blindtourist=true` for player ships. |
| `ExploreUpdate` | `order.move.recon.update.xml` | `trade` | Refresh intel on known stations. Wraps Recon with `exploreupdate=true`. |
| `Police` | `order.move.recon.police.xml` | `combat` | Police patrol and criminal investigation. Wraps Recon with `police=true`. |
| `DeployObjectAtPosition` | `order.deployobjectatposition.xml` | `internal` | Deploy any deployable (satellites, mines, nav beacons, resource probes, laser towers) at a specific position. |

**Key insight**: There is no standalone "scout" or "survey" order. All scouting behavior flows through the single `order.move.recon.xml` engine, which operates in different modes based on internal boolean flags.

### 1.2 The Recon Engine Modes (`order.move.recon.xml`)

The core Recon script (1,880+ lines) has five mutually exclusive operating modes, set by internal parameters:

| Mode | Flag | What It Does | Who Uses It |
|------|------|-------------|-------------|
| **Military Recon** | `(default)` | Scans ships and stations in enemy/unknown space. Reports threat levels per faction. Returns to HQ to deliver intel. Deploys satellites at POIs if `deploysatellites=true`. | Faction AI via `factionsubgoal_recon.xml` |
| **Explore** | `blindtourist=true` | Explores without cheating (no omniscient target knowledge). Visits zones, discovers stations/gates. Earns XP for discoveries. | Player `Explore` order |
| **ExploreUpdate** | `exploreupdate=true` | Visits zones with known stations to refresh trade data and docking info. | Player `ExploreUpdate` order |
| **Police** | `police=true` | Scans ships for illegal cargo, contraband, criminal status. Engages or confiscates. | Police faction ships |
| **Resource Scout** | `resourcescout=true` | Searches for asteroids, collectables, lockboxes. Deploys resource probes near mineral fields. | Mining routine (`order.mining.routine.xml`) |

**Parameters shared across all modes** (from `order.move.recon.xml` lines 4-31):

```
targetspace       : object  (sector, gate, or highway entry gate)
radius            : length  (only used by blindtourist mode)
timeout           : time    (-1s = infinite, 0s = one cycle)
hq                : object  (station/ship to report back to -- military recon & police only)
targetclasses     : list    (default [class.ship, class.station])
targetpurposes    : list    (default [])
police            : bool    (false)
exploreupdate     : bool    (false)
resourcescout     : bool    (false)
blindtourist      : bool    (false)
deploysatellites  : bool    (false)
cannotdock        : bool    (false)
```

### 1.3 Deployable Types

The game has four classes of deployable relevant to intel:

| Deployable | Object Class | Ware ID | Macro | Radar Range | Purpose |
|-----------|-------------|---------|-------|-------------|---------|
| Satellite Mk1 | `class.satellite` | `satellite_mk1` | `eq_arg_satellite_01_macro` | 30km | Basic sector coverage. Reveals ships/stations in range on map. |
| Satellite Mk2 (Advanced) | `class.satellite` | `satellite_mk2` | `eq_arg_satellite_02_macro` | 75km | Extended sector coverage. Same function, larger range. |
| Resource Probe | `class.resourceprobe` | `resourceprobe_01` | `eq_arg_resourceprobe_01_macro` | ~64km | Reveals mineral resources and yield data in asteroid fields. |
| Nav Beacon | `class.navbeacon` | `waypointmarker_01` | `eq_arg_waypointmarker_01_macro` | N/A | Waypoint marker. No intel function. |

**Deployment mechanisms**:

1. **Ship launches from ammo storage**: `launch_satellite` / `launch_resourceprobe` MD commands. Requires the deployable to be in the ship's ammo storage.
2. **Direct `create_object` spawn** (faction AI satellites only): No ship needed. Used by `factionlogic.xml` for satellite network maintenance.
3. **`DeployObjectAtPosition` order**: Ship flies to a position and deploys specified macros from its ammo storage. Supports all five deployable types.

### 1.4 SDK Functions for Deployables

From `x4_game_func_list.inc`:

```cpp
// Query
uint32_t GetNumAllSatellites(UniverseID defensibleid);
uint32_t GetAllSatellites(AmmoData* result, uint32_t resultlen, UniverseID defensibleid);
uint32_t GetNumAllResourceProbes(UniverseID defensibleid);
uint32_t GetAllResourceProbes(AmmoData* result, uint32_t resultlen, UniverseID defensibleid);
bool IsAdvancedSatellite(const UniverseID componentid);
bool IsResourceProbe(const UniverseID componentid);
bool IsSatellite(const UniverseID componentid);
uint32_t GetDefensibleDeployableCapacity(UniverseID defensibleid);
uint32_t GetMacroDeployableCapacity(const char* macroname);
bool IsDeployableMacroCompatible(UniverseID containerid, const char* macroname, const char* deployablemacroname);

// Launch (direct from ship)
void LaunchResourceProbe(UniverseID defensibleid, const char* resourceprobemacroname);
void LaunchSatellite(UniverseID defensibleid, const char* satellitemacroname);
```

**Note**: `LaunchSatellite` and `LaunchResourceProbe` are FFI functions that launch from a ship's ammo storage. They require the ship to actually have the deployable in inventory. They do NOT support spawning deployables out of thin air -- that requires `create_object` via MD.

### 1.5 Relevant MD Events

From `common.xsd`:

| Event | Fires When | Params |
|-------|-----------|--------|
| `event_object_launched_resourceprobe` | Ship launches a resource probe | object = launching defensible, param = probe |
| `event_resourceprobe_launched` | Resource probe deployed in a sector | object = sector, param = launcher, param2 = probe |
| (satellite equivalent not named but follows same pattern via `launch_satellite`) | | |

---

## 2. How Vanilla Faction AI Gathers Intel

### 2.1 Recon Sub-Goal (`factionsubgoal_recon.xml`)

The faction AI's only intel-gathering mechanism. Spawned as a sub-goal by both **Hold Space** and **Invade Space** strategic goals.

**Trigger**: Automatic. Hold Space maintains 2 concurrent Recon sub-goals per claimed sector. Invade Space spawns recon before invasion.

**Ship selection priority** (from `factionsubgoal_recon.xml` lines 262-270):
1. Pass 1: Ships tagged `[tag.scout, tag.factionlogic]` or `[tag.envoy, tag.factionlogic]`
2. Pass 2: Ships tagged `[tag.military, tag.factionlogic]` with `primarypurpose=purpose.fight`, class S or M only

**Order created** (line 540-543):
```xml
<create_order id="'Recon'" object="$Ship">
    <param name="targetspace" value="$Target"/>
    <param name="timeout" value="[10min,15min,20min].random"/>
    <param name="deploysatellites" value="true"/>
</create_order>
```

**Intel produced**: The Recon engine fires a signal on completion (line 1861):
```xml
<set_value name="$reconresult" exact="[
    this.ship.trueowner,       <!-- index 1: own faction -->
    'recon update',            <!-- index 2: signal type -->
    $EnemyFaction,             <!-- index 3: detected faction -->
    $targetspace,              <!-- index 4: sector -->
    [$ThreatLevel, player.age] <!-- index 5: [threat score, timestamp] -->
]"/>
<signal_objects object="player.galaxy" param="$reconresult"/>
<signal_objects object="this.ship" param="$reconresult"/>
```

This updates the faction's `$Intel` list: `[$Space, $ThreatLevel, $LastUpdate]` per sector. Unknown sectors default to threat level 10.

### 2.2 Satellite Network Maintenance (`factionlogic.xml`)

Completely separate from Recon. Runs as a periodic maintenance cycle.

**Trigger**: Every 1800 seconds (30 min), if `$SatelliteNetworkGoal > 0`.

**Per-faction satellite goals** (from `factionlogic.xml` lines 78-284):

| Faction | Satellite Goal |
|---------|---------------|
| Argon | 20 |
| Paranid | 17 |
| Teladi | 8 |
| HOP | 7 |
| Split (ZYA) | 5 |
| Antigone | 5 |
| Ministry (Teladi Gov) | 4 |
| Free Families | 3 |
| Terran | 2 |
| Boron | 2 |
| Segaris Pioneers | 2 |
| Xenon | 0 |

**Mechanism** (lines 1335-1388):
1. **Decay**: Existing satellites lose hull each cycle. Mk1: 3-8 HP per cycle. Mk2 (advanced): 0-3 HP per cycle. Destroyed at 0 HP.
2. **Replenish**: Spawn satellites directly via `create_object` (no ship involvement) at random owned stations or sector entry points. Max 15 spawns per cycle. Macro randomly chosen between Mk1 and Mk2.
3. **Tracking**: Satellites added to `$FactionSatellites` group.

**Critical detail**: Faction satellites are NOT deployed by ships. They are directly spawned into the game world by the MD script. This is why vanilla faction AI never issues `DeployObjectAtPosition` orders for satellites.

### 2.3 Resource Discovery (Mining Routine)

NPC factions do NOT proactively deploy resource probes. Resource probes are exclusively a player-facing mechanic.

**How NPC miners find resources** (from `order.mining.routine.xml`):
- Mining routine has a `resourcescout` parameter (default false)
- When `resourcescout=true`, calls into the Recon engine with `resourcescout=true` mode
- Recon engine in resource scout mode: finds zones with asteroids, moves to them, and can deploy resource probes from ammo storage (line 1344-1358)
- But NPC miners typically skip this -- they use `find_closest_resource` to locate resources directly, which does not require a resource probe

**Player-facing resource probes**: Used in tutorials and story missions. The `upkeep_management.xml` script checks for player-deployed resource probes near mining operations and signals `upkeep_resource_probe_present` for efficiency bonuses.
