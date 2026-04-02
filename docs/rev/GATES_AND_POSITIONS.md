# X4 Gate Entities & Position Resolution

> **Binary:** X4.exe v9.00 | **Date:** 2026-04-02
>
> Research into how X4 represents gates as entities, position querying for spatial objects,
> and how the vanilla AI references positions in strategic decisions.
> Sources: SDK function list, game MD scripts (factiongoal_hold_space, factiongoal_invade_space,
> factionlogic_staticdefense), game types header, Lua UI code.

---

## Gates Are First-Class Entities

Gates in X4 are spatial entities with UniverseIDs, just like ships and stations. They have:

- Entity class: `class.gate` (jump gates, TOAs) or `class.highwayexitgate` / `class.highwayentrygate`
- A `.destination` property pointing to the paired gate entity on the other side
- `.destination.sector` to get the target sector
- `.knownname` for display name
- `.isactive` (some gates start inactive until story activates them)
- Position in sector: queryable via `GetObjectPositionInSector(gate_id)`

**Source**: `scriptproperties.xml`, vanilla MD scripts

---

## Position Resolution SDK Functions

### Primary: Entity Position in Sector

```c
UIPosRot GetObjectPositionInSector(UniverseID objectid);

typedef struct {
    double x, y, z;       // sector-relative position
    double pitch, yaw, roll;  // orientation
} UIPosRot;
```

Works for ANY spatial entity: ships, stations, gates, deployables. Returns position relative to the sector origin. Single call, in-process, microsecond latency.

### Gate-Specific Functions

```c
// Gate connection management (used by game engine)
void AddGateConnection(UniverseID gateid, UniverseID othergateid);
void RemoveGateConnection(UniverseID gateid, UniverseID othergateid);
```

### Entity Enumeration

```c
// Get all objects of a class in a sector
uint32_t GetContainerAllSpaceObjects(UniverseID* result, uint32_t resultlen, UniverseID containerid);

// Check entity class
bool IsComponentClass(UniverseID componentid, const char* classname);
```

To enumerate gates in a sector: iterate zone children, filter by `IsComponentClass(id, "gate")`.

---

## How Vanilla AI References Positions

The vanilla AI **never uses raw coordinates in decision logic**. It always finds gates first, then computes positions relative to gates.

### Defend Sector (factiongoal_hold_space)

```xml
<!-- Find gates in sector leading to enemy space -->
<find_gate name="$EntryPoints" destination="$TargetSector"
    owner="$Faction" multiple="true"/>

<!-- Position ships near each gate -->
<create_order id="'ProtectPosition'" object="$Ship" default="true">
    <param name="destination" value="[$Sector, $EntryPoint.$Position]"/>
    <param name="radius" value="$DefendArea.$Range"/>
</create_order>
```

### Invade Sector (factiongoal_invade_space)

```xml
<!-- Find gates in own sector leading to target -->
<find_gate name="$LocalEntryPoints" destination="$Target"
    space="$OwnSector" multiple="true"/>

<!-- Stage fleet near the gate before pushing through -->
<create_order id="'ProtectPosition'" object="$Commander">
    <param name="destination" value="[$OwnSector, $StagingGate.$Position]"/>
</create_order>
```

### Static Defense (factionlogic_staticdefense)

```xml
<!-- Find gates in sector as deployment reference -->
<find_object name="$Gates" class="[class.gate, class.highwayexitgate]"
    space="$Sector" multiple="true"/>

<!-- Deploy laser towers / mines near gates -->
<!-- Position computed as offset from gate position -->
```

**Pattern**: Find entity → `entity.$Position` → use as ProtectPosition destination. The AI never generates coordinates from scratch.

---

## Gate Enumeration for GAME_DATA

To include gate IDs in the sector map (sent once at connection), the DLL:

1. For each sector: iterate zones → iterate objects → filter `IsComponentClass(id, "gate")`
2. For each gate: read `GetObjectPositionInSector(gate_id)` for position
3. Read gate destination: `.destination` property gives the paired gate → `.destination.sector` gives target sector

### Gate Destination Query

MD scripts use `.destination` (a property). No direct `GetGateDestination(gate_id)` found in SDK exports.

**Workaround options**:
1. Build lookup table during GAME_DATA collection by zone/sector hierarchy traversal
2. Use Lua bridge: `AddUITriggeredEvent` → MD reads gate.destination → returns via event
3. IDA investigation of `GetComponentDetails` for gate-class entities may reveal destination in the struct

~130 gates in the full universe. One-time cost at session start. Any approach is fast enough.

---

## Safe Position Generation

When placing stations or deployables near a reference point, the game uses:

```xml
<get_safe_pos result="$SafePos" sector="$Sector" value="$ReferencePosition"
    radius="5km" ignored="$IgnoredObject"/>
```

This finds a position near the reference that doesn't collide with existing objects. Should use the equivalent function when resolving "build station near gate X" to actual coordinates.

---

## Summary

| Entity Type | Has UniverseID | Position Queryable | In GAME_DATA |
|-------------|:-:|:-:|:-:|
| Gate (jump gate) | Yes | `GetObjectPositionInSector()` | Yes (GateConnection.gate_id) |
| Gate (highway) | Yes | `GetObjectPositionInSector()` | Yes (GateConnection.gate_id) |
| Station | Yes | `GetObjectPositionInSector()` | In GAMESTATE (per-faction) |
| Ship | Yes | `GetObjectPositionInSector()` | In GAMESTATE (FleetInfo) |
| Resource field | No (region, not entity) | Not directly | Via `GetDiscoveredSectorResources` (aggregate) |
| Deployable (satellite) | Yes | `GetObjectPositionInSector()` | In GAMESTATE (visibility) |

Resource fields are regions, not entities -- they don't have UniverseIDs. Position-specific resource queries use `GetMineablesAtSectorPos()` with coordinates. For strategic AI, the sector-aggregate `GetDiscoveredSectorResources()` is sufficient (no position needed).
