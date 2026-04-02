# Resource Discovery Reverse Engineering

> **Binary:** X4.exe v9.00 | **IDB:** X4.exe.i64 | **Date:** 2026-04-01
>
> Deep investigation of `GetDiscoveredSectorResources` and the resource discovery system
> to determine whether resource discovery is tracked per-faction or player-only.
>
> **Verdict: PLAYER-ONLY.** The function filters by a single global discovery timestamp
> stored per ResourceArea, set by the player's Gravidar (fog-of-war) system. There is no
> per-faction discovery state for resources. NPC factions bypass discovery filtering entirely
> through different code paths.

---

## 1. Function Analysis

### GetNumDiscoveredSectorResources (0x140162670, size 0x27F)

```
Signature: uint32_t GetNumDiscoveredSectorResources(UniverseID sectorid)
```

**Decompiled logic:**

1. Resolves `sectorid` to a component via `sub_1400CE890` (component lookup by UniverseID, type=4 for sector).
2. Validates the component is a sector via virtual call `vtable[0x11B8](obj, 0x57)` -- class check with ID 0x57 (87).
3. Stores the sector ID in global `qword_1438B3A08` (paired-call scratch state).
4. Iterates the sector's resource area list: `sector+0x3F8` (begin) to `sector+0x400` (end), stride 8.
5. For each entry, dereferences a pointer chain: `entry -> entry+8 -> UniverseID lookup -> RTDynamicCast to U::ResourceArea`.
6. **The discovery filter** (address 0x140162788-0x140162797):
   ```asm
   movsd   xmm0, qword ptr [rdi+0E0h]    ; ResourceArea+0xE0 = discovery timestamp
   comisd  xmm0, qword ptr [r13+rbx+0]   ; TLS+0x300 = GetCurrentGameTime()
   ja      loc_14016289D                   ; if discovery_time > current_time, SKIP
   ```
7. For non-skipped resource areas, accumulates ware + yield data into the global scratch buffer `qword_1438B3A10`.
8. Returns the count stored in `qword_1438B3A38`.

### GetDiscoveredSectorResources (0x140154510, size 0x1B2)

```
Signature: uint32_t GetDiscoveredSectorResources(WareYield* result, uint32_t resultlen, UniverseID sectorid)
```

**Decompiled logic:**

1. Validates `sectorid == qword_1438B3A08` (must match prior GetNum call).
2. Validates the sector resolves correctly via `sub_14013CC50`.
3. Reads from the global scratch buffer `qword_1438B3A10` populated by the previous GetNum call.
4. Copies ware name pointer, current yield, and max yield into the caller's WareYield array.
5. Clears the scratch buffer state (`qword_1438B3A08 = 0`, `qword_1438B3A38 = 0`).

This is a paired-call pattern: GetNum populates, Get reads. **No additional filtering in Get -- all filtering happens in GetNum.**

### GetCurrentGameTime (0x140ab8760, size 0x32)

```c
double GetCurrentGameTime() {
    __int64 tls = *(_QWORD*)NtCurrentTeb()->ThreadLocalStoragePointer;
    return *(double*)(tls + 768);  // TLS + 0x300
}
```

Confirmed: TLS offset 0x300 (768 decimal) stores the current game simulation time as a `double`.

---

## 2. The Discovery Timestamp (ResourceArea+0xE0)

The field at `ResourceArea+0xE0` is a `double` representing the game-time at which this resource area was "discovered." The filter logic is:

```
if (resourceArea->discoveredTime > currentGameTime)
    skip;  // not yet discovered
```

**Key properties:**
- This is a **single scalar** -- not a per-faction table, not a bitfield.
- There is only one discovery timestamp per ResourceArea, period.
- An undiscovered resource area has `discoveredTime` set to a value greater than current game time (likely `DBL_MAX` or a far-future time).
- When the player's Gravidar system scans the area, this timestamp is set to the current game time.

---

## 3. The Gravidar (Fog of War) System

The Gravidar system runs on its **own dedicated thread** (function at 0x141063B10). Key findings:

- Thread is named "Gravidar" (via `SetThreadDescription`).
- Uses critical sections for synchronization with the main game thread.
- Processes sector scan results and creates `U::FogOfWarChangedEvent` objects (vtable ref at 0x141064BDE).
- The event includes the sector component pointer and the current game time (TLS+0x300).
- The Gravidar system is responsible for updating the discovery timestamps on ResourceArea objects.

**RTTI class hierarchy found:**
- `AI::Gravidar` -- singleton managing the system
- `AI::GravidarUnit` / `AI::ActiveGravidarUnit` -- per-ship scanner units  
- `AI::GravidarListener` -- event consumer
- `U::FogOfWarChangedEvent` -- generated when scan reveals new areas
- `U::SetDetectedResourceAreaOverlapFractionEvent` -- updates resource area detection state
- `U::PlayerDiscoveredComponentEvent` -- player-specific discovery event
- `U::PlayerTradeDiscoveredEvent` -- player-specific trade discovery

The RTTI names are unambiguous: all "discovered" events are **Player**-prefixed. There is no `FactionDiscoveredEvent` or `NPCDiscoveredEvent`.

---

## 4. How NPC Factions Access Resources (No Discovery Filter)

### 4.1 AI Script Pattern: `useundiscovered`

The `find_closest_resource` MD/AI script command has a `useundiscovered` attribute:

```xml
<!-- common.xsd line 26780 -->
<xs:attribute name="useundiscovered" type="booleanexpression" use="optional">
  <xs:documentation>
    Whether resources not yet discovered by the player are allowed to be found (defaults to true)
  </xs:documentation>
</xs:attribute>
```

Note the documentation: "discovered **by the player**" -- not "by the owning faction."

The mining routine (`order.mining.routine.xml`) uses this consistently:

```xml
<!-- Line 1121 -->
<find_closest_resource sector="$sector" ... useundiscovered="not this.isplayerowned">

<!-- Line 1246 -->
<find_closest_resource sector="$sector" ... useundiscovered="not this.isplayerowned"/>

<!-- Line 1251 -->
<find_closest_resource sector="$sector" ... useundiscovered="not this.isplayerowned">

<!-- Line 1269, 1273, 1372, 1377 -- same pattern throughout -->
```

**Translation:** `useundiscovered="not this.isplayerowned"` means:
- Player-owned ships: `useundiscovered = false` -- respect discovery filter
- NPC faction ships: `useundiscovered = true` -- **bypass discovery filter entirely**

This is the game's deliberate design: NPC factions have perfect knowledge of all resources. Only the player needs to "discover" resource areas via resource probes or mining ship scanning.

### 4.2 Sector Properties (No Discovery Filter)

The sector script properties are also unfiltered:

```xml
<!-- scriptproperties.xml -->
<property name="resources" result="All region resource wares" type="warelist" />
<property name="yieldrating.{$ware}" result="Current yield rating (0-15)" type="integer" />
<property name="bestyieldrating.{$ware}" result="Best yield rating (0-15)" type="integer" />
```

Usage in vanilla NPC AI:
- **khaak_activity.xml** (line 1137): `$Sector.bestyieldrating.{$Resource}` -- Khaak faction directly queries best yield, no faction filter.
- **gm_find_resources.xml** (line 893): `$Space.bestyieldrating.{$ResourceTryList.{$i}}` -- mission generation uses unfiltered best yield.
- **order.mining.routine.xml** (line 1234): `$gatheringspaces.{$i}.resources.{$ware}.exists` -- checks resource existence without faction filter.

### 4.3 factionlogic_economy.xml

The vanilla faction economy logic (`factionlogic_economy.xml`) does NOT check resource discovery at all. It reasons about:
- Production chains and supply/demand (`$Ware.raceresources`)
- Insufficient resource scoring (`$InsufficientNeighbourMultiplier`)
- Mining ship availability and laser checks

It assigns mining ships to sectors based on ware demand, not based on resource discovery. The actual resource location is delegated to the mining order AI script, which uses `useundiscovered=true` for NPC ships.

---

## 5. Complete Discovery Architecture

```
                    PLAYER PATH                         NPC PATH
                    ===========                         ========

    Resource Probe deployed                  Mining order assigned by
    or ship scans area                       factionlogic_economy.xml
           |                                          |
           v                                          v
    Gravidar thread detects                  find_closest_resource
    overlap with ResourceArea                useundiscovered="true"
           |                                          |
           v                                          |
    ResourceArea+0xE0 set                             |
    to current game time                              |
           |                                          |
           v                                          v
    FogOfWarChangedEvent           NPC ships go directly to any
    fires -> UI updates            resource area in their sector
           |                       range, regardless of discovery
           v
    GetDiscoveredSectorResources
    now includes this area
    (timestamp <= currentTime)
```

---

## 6. Implications for X4Strategos

### The Problem

`GetDiscoveredSectorResources` is **player-perspective only**. If X4Strategos calls it for NPC faction intelligence gathering:
- All NPC factions see exactly what the player has discovered
- Factions managing sectors the player has never visited see zero resources
- Factions "learn" about resources when the player deploys probes, even in distant sectors

This completely breaks the faction AI model.

### Available Alternatives

| Approach | Pros | Cons |
|----------|------|------|
| **A. Sector script properties** (`bestyieldrating`, `resources`) | Global, unfiltered, matches NPC AI behavior | Not callable from native C++ (MD/Lua only); no current/max yield split |
| **B. Static mapdefaults.xml** data | Complete resource definitions per sector; available at load time | No depletion tracking; requires parsing game data files |
| **C. GetMineablesAtSectorPos** | Per-position, not discovery-filtered | Requires knowing positions; very tactical, not strategic |
| **D. Companion-side resource model** | Full control, per-faction fog-of-war | Must bootstrap from static data; must track depletion ourselves |
| **E. GetDiscoveredSectorResources with deferred use** | Simple API, includes depletion data | Player-only; breaks fog-of-war; all factions see same data |

### Recommended Approach

**Option D (Companion-side resource model) combined with Option B (static data bootstrap):**

1. **Bootstrap from mapdefaults.xml:** At game load, parse the `<resourceareas>` entries per sector to build a static resource catalog. Each sector gets a list of (ware, yield_rating, resource_area_count).

2. **Track depletion via GetDiscoveredSectorResources:** When the player discovers resources in a sector, the companion receives the current/max yield data. This gives us depletion deltas for sectors the player has visited.

3. **Per-faction discovery in companion:** The companion maintains its own per-faction discovery state:
   - Factions "know" about resources in sectors they own or have had mining operations in.
   - Factions can "discover" resources via their mining ships operating in a sector (synthetic discovery).
   - The companion's LLM brain decides where to explore based on static resource potential.

4. **Yield ratings via MD bridge (if needed):** For runtime yield data without discovery filtering, we can query `$Sector.bestyieldrating.{$ware}` via the MD/Lua bridge. This returns unfiltered data accessible to all factions.

### What NOT to Do

- Do NOT use `GetDiscoveredSectorResources` as the primary resource data source for NPC factions.
- Do NOT assume the function can be "tricked" by passing different parameters -- there is no faction parameter anywhere in the call chain.
- Do NOT try to set `ResourceArea+0xE0` directly -- the Gravidar system manages this on its own thread, and writing to it from the UI thread would be a race condition.

---

## 7. Struct Summary

### ResourceArea (partial, from decompilation)

| Offset | Type | Field | Notes |
|--------|------|-------|-------|
| 0x00 | vtable* | vftable | U::ResourceArea vtable |
| 0x30 | ptr | ware_info | Points to ware data structure |
| 0x80 | int32 | max_yield | Maximum yield value |
| 0xE0 | double | discovered_time | Game-time when player discovered this area. `> currentTime` means undiscovered. |

### Sector Resource Container

| Offset | Type | Field |
|--------|------|-------|
| sector+0x3F8 | ptr | resource_areas_begin |
| sector+0x400 | ptr | resource_areas_end |

Each entry is 8 bytes (pointer to resource area link node). The link node contains a pointer to the actual component, which is resolved via UniverseID lookup and RTDynamicCast to `U::ResourceArea`.

### Scratch Buffer (Global State)

| Address | Type | Purpose |
|---------|------|---------|
| `0x1438B3A08` | uint64 | Last queried sector ID |
| `0x1438B3A10` | vector | Scratch buffer for results |
| `0x1438B3A30` | uint64 | Vector storage mode flag |
| `0x1438B3A38` | uint64 | Result count |

The paired-call pattern (GetNum then Get) uses these globals. This means the functions are **not thread-safe** and **not reentrant** -- consistent with the requirement that all game function calls happen on the UI thread.

---

## 8. Address Information

| Address | Annotation |
|---------|-----------|
| 0x140162670 | Function purpose, player-only discovery filter, scratch buffer usage |
| 0x140162788 | Discovery timestamp comparison explained |
| 0x140154510 | Paired-call reader, no additional filtering |
| 0x140ab8760 | GetCurrentGameTime TLS+0x300 identity |
| 0x141063B10 | Gravidar thread, FogOfWarChangedEvent generation |
