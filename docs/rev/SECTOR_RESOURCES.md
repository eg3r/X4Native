# X4 Sector Resource & Environment Systems

> **Binary:** X4.exe v9.00 | **Date:** 2026-04-02
>
> Research into how X4 represents per-sector resources, sunlight, hazards, and environmental properties.
> Sources: SDK function list, game XML/XSD schemas, Lua UI code, MD faction scripts.

---

## Resource Types

7 natural mineable resources + 2 scrap types, defined in `libraries/wares.xml` with `minable` tag:

| Ware ID | Group | Storage | Category |
|---------|-------|---------|----------|
| `ore` | minerals | solid | Asteroid |
| `silicon` | minerals | solid | Asteroid |
| `nividium` | minerals | solid | Asteroid |
| `ice` | ice | solid | Asteroid |
| `hydrogen` | gases | liquid | Nebula |
| `helium` | gases | liquid | Nebula |
| `methane` | gases | liquid | Nebula |
| `rawscrap` | refined | solid | Wreckage |
| `rawkhaakscrap` | - | solid | Khaak wreckage |

**Source**: `libraries/wares.xml`, `libraries/regionyields.xsd`

---

## Resource Data Architecture (v9.00+)

As of v9.00, the resource system was overhauled. From `region_definitions.xml` line 22:

> "As of 9.00 Resources node is not used except maybe for some special cases, instead add `<resourceareas>` in the SECTOR data set under `<properties>` in mapdefaults.xml"

Per-sector resource definitions in `mapdefaults.xml`:

```xml
<dataset macro="Cluster_01_Sector001_macro">
  <properties>
    <resourceareas>
      <resourcearea amount="7" ref="sphere_medium_hydrogen_medium"/>
      <resourcearea amount="5" ref="sphere_small_hydrogen_high"/>
      <resourcearea amount="6" ref="sphere_medium_silicon_medium"/>
    </resourceareas>
    <area sunlight="1.23" economy="0.5" security="0.25"/>
  </properties>
</dataset>
```

Each `<resourcearea>` references a yield definition from `regionyields.xml` with: ware, yield, rating (0-15, displayed as 0-5 stars), respawndelay, boundary shape.

---

## SDK Functions - Resource Queries

### Level 1: Sector-Aggregate (Strategic)

```c
uint32_t GetNumDiscoveredSectorResources(UniverseID sectorid);
uint32_t GetDiscoveredSectorResources(WareYield* result, uint32_t resultlen, UniverseID sectorid);

typedef struct {
    const char* ware;    // "hydrogen", "ore", etc.
    int32_t current;     // current total yield (depleted by mining)
    int32_t max;         // maximum total yield
} WareYield;
```

Same functions the game's encyclopedia and map UI use (`menu_encyclopedia.lua` line 2819, `menu_map.lua` line 14765). Respects fog of war - only discovered resources appear.

### Level 2: Point-Specific (Tactical, Mining Orders)

```c
uint32_t GetNumMineablesAtSectorPos(UniverseID sectorid, Coord3D position);
uint32_t GetMineablesAtSectorPos(YieldInfo* result, uint32_t resultlen, UniverseID sectorid, Coord3D position);

typedef struct {
    const char* wareid;
    int32_t amount;
} YieldInfo;
```

### Level 3: Region Definitions (Static Metadata)

```c
uint32_t GetNumRegionDefinitions(void);
uint32_t GetRegionDefinitions(RegionDefinition* result, uint32_t resultlen);
uint32_t GetRegionResources(RegionResource* result, uint32_t resultlen, const char* regiondefinition);
uint32_t GetRegionBoundaries(RegionBoundary* result, uint32_t resultlen, const char* regiondefinition);
const char* GetRegionDefinition(UniverseID regionid);
```

### Resource Probe Functions

```c
bool IsResourceProbe(const UniverseID componentid);
void LaunchResourceProbe(UniverseID defensibleid, const char* resourceprobemacroname);
```

---

## Sunlight

Per-sector float, static, defined in `mapdefaults.xml` `<area sunlight="X">`.

```
// MD script property (scriptproperties.xml line 1237):
<property name="sunlight" result="Sunlight value for this space" type="largefloat" />

// Lua access:
local sunlight = GetComponentData(sectorid, "sunlight")
```

Range ~0.4 to ~3.67. Vanilla AI threshold for solar viability:

```xml
<!-- factionlogic_economy.xml line 159 -->
<do_if value="$Ware == ware.energycells and $Sector.sunlight lt 0.4f">
  <return value="false"/>
</do_if>
```

| Range | Significance |
|-------|-------------|
| < 0.4 | Solar not viable (vanilla AI refuses) |
| 0.4-0.8 | Marginal |
| 0.8-1.3 | Standard (baseline) |
| 1.3-2.0 | Good |
| > 2.0 | Excellent |

---

## Yield Ratings

0-15 integer rating per sector per ware. Displayed as 0-5 stars (rating / 3).

```xml
<!-- scriptproperties.xml lines 1301-1302 -->
<property name="yieldrating.{$ware}" result="Current yield rating (0-15)" type="integer" />
<property name="bestyieldrating.{$ware}" result="Best yield rating (0-15)" type="integer" />
```

`yieldrating` changes with depletion. `bestyieldrating` is static maximum. The Khaak activity script uses `bestyieldrating` to score sectors for resource richness.

---

## Hazardous Regions

```xml
<!-- scriptproperties.xml lines 1297-1299 -->
<property name="hashazardousregion" result="True if sector has hazardous region" type="boolean" />
<property name="hashazardousregionat.{$position}" result="Position inside hazardous region" type="boolean" />
```

Types found in `region_definitions.xml`:
- Radiation damage fields (100 noshield / 500 shield per tick)
- Electrostatic fields (1 shield per tick)
- Gravidar-obscuring regions

---

## Sector Environment Properties

Static per-sector, from `mapdefaults.xml` `<area>` attributes:

| Property | Type | Source | Notes |
|----------|------|--------|-------|
| `sunlight` | float | `GetComponentData(sector, "sunlight")` | Solar intensity |
| `economy` | float (0-1) | `GetComponentData(sector, "economy")` | NPC economic activity |
| `security` | float (0-1) | `GetComponentData(sector, "security")` | NPC patrol density |
| `hashazardousregion` | bool | Script property | Hazardous regions present |

---

## Resource Discovery / Fog of War

**CRITICAL: Discovery is PLAYER-ONLY.** See `RESOURCE_DISCOVERY_RE.md` for full decompilation proof.

`GetDiscoveredSectorResources` returns only resources discovered **by the player**. The function filters by a single timestamp (`ResourceArea+0xE0`) compared against current game time. There is no per-faction discovery state.

**Player discovery requires:**
- Resource probes (deployable, reveal resource data in area)
- Mining ships (discover resources in vicinity while mining)
- Satellites/radar do NOT discover resources

**NPC factions bypass discovery entirely:**
- `find_closest_resource` with `useundiscovered="not this.isplayerowned"` - NPC ships set this to `true`
- Sector properties (`bestyieldrating`, `resources`) are unfiltered globals
- Vanilla faction AI (factionlogic_economy.xml) never checks resource discovery

**Implication for mods:** Do NOT use `GetDiscoveredSectorResources` for NPC faction resource intelligence. Use sector script properties (`bestyieldrating.{$ware}`, `resources`) or static data from `mapdefaults.xml` instead.

---

## What Does NOT Exist

| Feature | Status |
|---------|--------|
| Continuous radiation scalar | Absent - hazards are binary per-region |
| Nebula density affecting travel | Absent at strategic level |
| Tide (Avarice) as sector property | Absent - it's an event system, not a data field |
| Temperature / atmospheric data | Not modeled |
| Gravity affecting operations | Local force fields only, not sector-wide |
