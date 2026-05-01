# Fixed Stations, Station Enumeration, Sector Control & Reveal Stubs

> **Binary:** X4.exe v9.00 | **Date:** 2026-04-19
>
> Reverses `GetFixedStations` / `GetNumFixedStations`, documents the call contract, and compares galaxy-wide station enumeration options available to any X4Native-based extension. Adds analysis of `GetSectorControlStation` (§9) and `RevealStations` (§10, retired stub).

---

## 1. TL;DR

`GetFixedStations` is **not** a generic sector/space station enumerator. It returns only stations whose *macro* has `isfixedstation="1"` — HQs, Data Vaults, story/unique plot buildings. Faction-agnostic, visibility-agnostic. Callers must filter with `IsKnownToPlayer` if they need fog-of-war semantics.

For a full galaxy station view, use the **per-faction path** (`GetAllFactionStations` + ancestor→sector map). `GetFixedStations(0)` is a useful **supplementary landmark query** (HQs etc.) — a small, high-signal set.

---

## 2. FFI Signatures

```c
uint32_t GetNumFixedStations(UniverseID spaceid);
uint32_t GetFixedStations(UniverseID* result, uint32_t resultlen, UniverseID spaceid);
```

| Field | Value |
|-------|-------|
| Export ordinal (Num) | 0x395 @ `RVA 0x00162FD0` → `0x140162FD0` |
| Export ordinal (Get) | 0x271 @ `RVA 0x001569A0` → `0x1401569A0` |
| Added | v9.00 (per `native/version_db/func_history.json`) |
| Stability | No known behavior change since v9.00 |
| SDK | Listed in `sdk/x4_game_func_list.inc:559,809` |

### Contract

1. Call `GetNumFixedStations(spaceid)` first — it refreshes an internal cache keyed on `spaceid` with a 10-second TTL.
2. Call `GetFixedStations(buf, cap, spaceid)` **with the same `spaceid`** within 10 seconds — it copies up to `cap` cached IDs into `buf` and returns the count copied.
3. Violating the contract (different `spaceid`, or stale cache) → the game logs `"Stored fixed stations do not match the parameters. Call GetNumFixedStations first."` and the call returns 0.

`spaceid` accepts any **space-derived** entity (class-id 117 = `space`, parent of `galaxy`, `cluster`, `sector`, `zone`). `spaceid == 0` falls back to the current galaxy root.

---

## 3. What "Fixed" Means (Semantics)

`isfixedstation` is a static attribute on station macros in `libraries/stationtypes.xml`. The canonical Lua usage is the in-game encyclopedia:

```lua
-- ui/addons/ego_detailmonitor/menu_encyclopedia.lua
local stationlibrary = GetLibrary("stationtypes")
for _, e in ipairs(stationlibrary) do
    if GetMacroData(e.id, "isfixedstation") then
        table.insert(menu.data["FixedStations"], { macro = e.id, ... })
    end
end
local n = C.GetNumFixedStations(0)
if n > 0 then
    local buf = ffi.new("UniverseID[?]", n)
    n = C.GetFixedStations(buf, n, 0)
    for i = 0, n - 1 do
        if C.IsKnownToPlayer(buf[i]) then  -- fog-of-war filter in UI layer
            table.insert(menu.data["FixedStations"],
                { component = buf[i],
                  name = ffi.string(C.GetComponentName(buf[i])) })
        end
    end
end
```

So "fixed station" = **unique, static, lore-tagged stations**:

- Argon Prime HQ, Teladi HQ, Split HQ, Paranid HQ, etc.
- Player HQ (PHQ)
- Data Vaults, Abandoned Ships' parent stations
- Terraforming project stations
- Scripted mission/plot buildings

It is **not**: shipyards, wharfs, equipment docks, trading stations, defence platforms, or faction-built production stations, unless those specific macros happen to have `isfixedstation="1"` in the XML.

---

## 4. Decompiled Walk

### 4.1 GetFixedStations (`sub_1401569A0`)

Paraphrased from the recovered pseudocode:

```c
uint32_t GetFixedStations(UniverseID* out, uint32_t cap, UniverseID spaceid)
{
    void* space;

    if (spaceid != 0) {
        space = sub_140131740(spaceid);  // ID->object, require class 117 "space"
        if (!space) { log("Failed to retrieve space..."); return 0; }
    } else {
        space = *(qword_143CAA948 ? qword_143CAA948 + 0x228 : &qword_143CAA728);
        if (!space) { log("Could not find the galaxy."); return 0; }
        if (!Component::IsOrDerivedFromClassID(space, 117)) space = 0;
    }
    if (!space) return 0;

    // Cache mismatch → caller skipped GetNumFixedStations
    if (qword_1438AF300 /*cached_space_id*/ != ((ComponentHeader*)space)->id) {
        log("Stored fixed stations do not match the parameters. Call GetNumFixedStations first.");
        return 0;
    }

    FixedStationCache* c = sub_14013CD00(&qword_1438AF300);  // returns cached vec
    UniverseID* begin = c->vec_begin;
    UniverseID* end   = c->vec_end;
    uint32_t n = 0;
    while (begin != end && n < cap) out[n++] = *begin++;
    return n;
}
```

### 4.2 GetNumFixedStations (`sub_140162FD0`)

```c
uint32_t GetNumFixedStations(UniverseID spaceid)
{
    void* space = /* same resolve as above */;
    if (!space) return 0;
    FixedStationCache* c = sub_14013CD00(&qword_1438AF300);  // rebuild if stale
    return (uint32_t)((c->vec_end - c->vec_begin));
}
```

### 4.3 Cache rebuild (`sub_14013CD00`)

```c
FixedStationCache* sub_14013CD00(FixedStationCache* cache /*, space, flag */)
{
    double now = thread_local_time();
    bool stale = (now >= cache->expiry) || (cache->key != space->id);

    if (stale) {
        cache->expiry  = now + 10.0;             // 10-second TTL
        cache->key     = space->id;
        cache->vec_end = cache->vec_begin;       // reset

        // space->+0xB0 is the children partition head.
        // sub_14045CB60 constructs a class-filtered recursive iterator
        // that yields every descendant of type station (class 97).
        StationIter iter;
        sub_14045CB60(&iter, space->children_root + 8, /*group*/ 2, &flag);

        while (iter.current()) {
            Station* s = iter.current();
            // Predicate: station has an IsFixedStation aspect AND its
            //            vtable-slot-12 predicate returns true.
            void* aspect = *(void**)((char*)s + 0x3E0);  // +992
            if (aspect && ((bool(*)(void*))(*(void**)aspect)[12])(aspect)) {
                cache->vec.push_back(s->id);
            }
            iter.advance();
        }
    }
    return cache;
}
```

**Notes on mechanics**

- `sub_14045CB60` is a shared **station-class tree iterator** used by several enumeration FFIs; its class filter is hard-coded to `97` (`station`). It walks the space's child-partition tree recursively, so `GetFixedStations(galaxy)` yields every fixed station in every sector — not just direct-children stations of the galaxy object.
- The predicate at `station + 0x3E0` is the station's fixed-station aspect. It is not one of the common production/build subsystems (which live at different offsets — see `PRODUCTION_MODULES.md` for an unrelated `+0x3E0` on *production modules*, a different class entirely).
- The cache is a single global (`qword_1438AF300`, ~40 bytes: `{ key u64, expiry f64, vec_begin*, vec_end*, vec_cap* }`). Because the cache is global and keyed on one space at a time, concurrent queries for different spaces in the same 10-second window force repeated rebuilds.

---

## 5. Galaxy-Wide Station Enumeration Options

When an extension needs "every station in the galaxy" or "every station in a sector", there are three practical paths. Summary: only Option A is FFI-complete today.

### Option A — Per-faction, then map to sector

```cpp
uint32_t nf = g->GetNumAllFactions(true);
std::vector<const char*> factions(nf);
g->GetAllFactions(factions.data(), nf, true);

for (const char* fid : factions) {
    uint32_t n = g->GetNumAllFactionStations(fid);
    std::vector<UniverseID> stations(n);
    n = g->GetAllFactionStations(stations.data(), n, fid);
    for (auto sid : stations) {
        UniverseID sector = x4n::entity::find_ancestor(sid, x4n::GameClass::Sector);  // via GetContextByClass
        // bucket (sector, sid) as needed
    }
}
```

- Covers every station belonging to a faction.
- `GetAllFactions(true)` returns ~30 factions in a vanilla galaxy, so this is ~30 enumeration calls + O(stations) ancestor lookups.
- **Gap**: stations with no faction owner are not in any faction bucket. `x4n::galaxy::rebuild_cache()` handles the equivalent gap for sectors by also querying `"ownerless"`, `"player"`, and `""`. An extension that needs derelict/abandoned stations should mirror the same list.

### Option B — Per-sector direct enumeration

**Not available via FFI.** No `GetSectorStations`, `GetSpaceStations`, or equivalent export exists. The only space-scoped station enumerator is `GetFixedStations`, which is narrower (fixed-only) and saddled with the call-order contract.

The internal `sub_14045CB60` iterator could enumerate any space's class-97 stations, but it is an internal helper, not exported. Exposing it would require calling the helper directly via `version_db` — more invasive than warranted when Option A covers the same ground.

### Option C — Walk the children partition tree manually

`x4n::entity::find_ancestor()` gives child → parent. There is no published `for_each_child_of_type`. Walking the children partition tree manually requires `space + 0xB0` (children head) plus the partition layout documented in `COMPONENT_SYSTEM.md` §3 — feasible but brittle across patches, and duplicates what the game's internal iterator already does. Not worth building until Option A demonstrably underserves a use case.

---

## 6. Recommendations

For extension authors needing a full station view:

1. **Use Option A** (`GetAllFactionStations` per faction, then `find_ancestor(sid, Sector)`). Complete for faction-owned stations, uses only stable FFIs.
2. **Include ownerless/player/empty-string owners** to catch derelicts and pre-assigned stations, mirroring the approach in `x4n::galaxy::rebuild_cache()`.
3. **Use `GetFixedStations(0)` additively for landmarks only.** ~10–30 lore stations in a vanilla galaxy — useful if the extension wants to tag HQs, Data Vaults, or plot stations as special. Do not use `GetFixedStations(sectorid)` as a general sector enumerator: it contradicts the API's intent and only returns the fixed subset.

### Contract checklist for `GetFixedStations`

```cpp
// UI-thread only (no protection vs concurrent mutation on other threads).
// Galaxy-scoped, cheap — the cache rebuild walks stations once per 10s per queried space.
uint32_t n = g->GetNumFixedStations(spaceid);
if (n == 0) return;                  // empty or space resolve failed
std::vector<UniverseID> buf(n);
uint32_t got = g->GetFixedStations(buf.data(), n, spaceid);
// got may be < n if another caller evicted the cache between calls —
// re-call GetNumFixedStations+GetFixedStations if exact equality matters.
for (auto sid : buf) {
    // No implicit fog-of-war filter; apply IsKnownToPlayer if needed.
    ...
}
```

---

## 7. Suggested SDK Addition

If an extension adopts landmark enumeration, a single SDK helper removes the risk of mis-applying the call contract:

```cpp
// x4n_galaxy.h (proposed)
namespace x4n::galaxy {
    /// Galaxy-wide fixed/unique landmark stations (HQs, Data Vaults, etc.).
    /// Returns raw UniverseIDs — caller must filter visibility.
    inline std::vector<UniverseID> fixed_stations(UniverseID space = 0) {
        auto* g = game();
        if (!g) return {};
        uint32_t n = g->GetNumFixedStations(space);
        if (n == 0) return {};
        std::vector<UniverseID> out(n);
        uint32_t got = g->GetFixedStations(out.data(), n, space);
        out.resize(got);
        return out;
    }
}
```

One helper, one call site, contract honored. Parked here for reference — not added to the SDK until there is a consumer.

---

## 8. References

- FFI list: `sdk/x4_game_func_list.inc:559,809`
- Lua usage: `reference/game/ui/addons/ego_detailmonitor/menu_encyclopedia.lua:578-593,1171-1174,1264-1269`
- Class IDs: `reference/class_ids.csv` (97=station, 117=space, 15=cluster, 87=sector, 46=galaxy, 108=zone, 43=entity, 110=container)
- Related: `VISIBILITY.md` §17.7, `SUBSYSTEMS.md` §10, `COMPONENT_SYSTEM.md` §3 (child partition tree)
- Addresses: `GetFixedStations @ 0x1401569A0`, `GetNumFixedStations @ 0x140162FD0`, cache global `qword_1438AF300`, iterator helper `sub_14045CB60`, cache rebuild `sub_14013CD00`.

---

## 9. GetSectorControlStation

### 9.1 Signature & Address

```c
UniverseID GetSectorControlStation(UniverseID sectorid);
```

| Field | Value |
|-------|-------|
| Export ordinal | 0x4BF @ `RVA 0x0016EBC0` → `0x14016EBC0` |
| Added | v9.00 (per `native/version_db/func_history.json`) |
| SDK | Listed in `sdk/x4_game_func_list.inc:1071` |

### 9.2 What It Returns

The UniverseID of the **station IN the queried sector that owns the first/primary build admin module** — i.e. the claim asset that actually asserts sector ownership. Per-sector-distinct: different sectors of the same faction return different stations. NOT the faction's HQ (unless the sector happens to be the HQ sector), and NOT the faction-rep station (the rep lives on HQ and is a different concept from per-sector claim authority).

In practice, the game uses it to route sector-level interactions to the asset that owns sector control:

- **Money destination** when the player buys a build plot in that sector. Both `menu_station_configuration.lua:3411-3412` and `ego_detailmonitor/menu_map.lua:4241-4242` call `GetSectorControlStation(sector)` and route the plot price via `TransferPlayerMoneyTo`. Money goes to the in-sector claim-bearing admin because that's the station collecting plot revenue for the territory.
- **Representative endpoint** for AI behaviors that target `this.trueowner.representative` (`interrupt.disengage.xml:98-99`, `fight.attack.object.bigtarget.xml:115-116`). Resolves to the in-sector control station as the local face of the faction in that sector.

It is **not** "the biggest station in the sector" and **not** "a generic / random station in the sector". Specifically the station bearing the first build admin / claim module.

### 9.2.1 Implications for sector transfer (§4.5)

`GetSectorControlStation` IS the right primitive for sector ownership transfer. Returns the in-sector primary claim-bearing station; transferring its ownership flips sector ownership.

A future simplification of `LIB_FireSectorTransfer` (currently uses `find_station_by_true_owner` + per-station `canclaimownership` filter to enumerate ALL stations of the from-faction in the sector) could:
1. Call `GetSectorControlStation(target_sector)` to identify the primary claim-bearer.
2. Transfer it unconditionally — sector ownership flips.
3. Iterate other in-sector stations for the disposition (`Keep`/`Transfer`/`Remove`) axis.

Today's enumeration approach is correct and doesn't need this optimization for behaviour, but the SDK shortcut would make "transfer = sector flips" explicit at the wire layer instead of emergent from per-station claim logic.

### 9.2.2 Decompile reinterpretation needed

The decompiled flow at §9.3 below walks `galaxy+0x3A8` indexed by `owner` faction, then walks the result's container chain. My original write-up labeled the lookup as a "law-enforcement representative" registry (matching the function's log message strings) — but empirically the function returns a per-sector station, not the faction's singular global rep/HQ. So the data structure at `galaxy+0x3A8` is NOT a flat `faction → rep` map; it's something more complex (per-(faction, sector), or a structure that gets walked to surface the in-sector claim-bearer). The "rep" terminology in the log strings is misleading. Re-investigation needed to map the actual structure.

### 9.3 Decompiled Flow

```c
UniverseID GetSectorControlStation(UniverseID sectorid)
{
    // 1) Resolve sectorid → component, must be class 87 (sector).
    void* sector = sub_1400CE7F0(qword_146C812E0, sectorid, /*kind*/ 4);
    if (!sector || !IsOrDerivedFromClassID(sector, 87)) {
        log("Failed to retrieve sector with ID '%llu'"); return 0;
    }

    // 2) Resolve owner faction (two-step, sentinel-guarded).
    //    sector->vtbl[+5632]() is the primary owner getter.
    //    sector->vtbl[+5616]() is the secondary (used only if the
    //    primary returns a non-sentinel).
    void* primary = (*(vmethod*)(*(vtbl*)sector + 5632))(sector);
    void* owner   = (primary != qword_1438863D0)
                    ? (*(vmethod*)(*(vtbl*)sector + 5616))(sector)
                    : qword_1438863D0;  // sentinel "no owner"
    if (!owner) {
        log("Failed to retrieve owner of sector with ID '%llu'"); return 0;
    }

    // 3) Find the faction registry entry in the galaxy (galaxy+0x3A8). Must be class 43 (entity).
    void* galaxy = *(qword_143CAA948 ? qword_143CAA948 + 0x228 : &qword_143CAA728);
    if (!galaxy) { log("Failed to retrieve galaxy"); return 0; }
    void* rep = sub_140525400(galaxy + 0x3A8, owner);
    if (!rep || !*(void**)rep || !IsRealComponentClass(*(void**)rep, 43)) {
        log("Failed to retrieve law enforcement representative of faction '%s' for sector with ID '%llu'", owner_name, sectorid);
        return 0;
    }

    // 4) Walk up the representative's container chain (link at +0x70)
    //    until we find a class-110 (container) ancestor. Return its
    //    UniverseID (at offset +8).
    void* node = *((void**)*rep + 14 /*+0x70*/);
    while (node && !IsRealComponentClass(node, 110))
        node = *((void**)node + 14);
    if (!node) {
        log("Failed to retrieve container of law enforcement representative of faction '%s' for sector with ID '%llu'", owner_name, sectorid);
        return 0;
    }
    return *((UniverseID*)node + 1 /*+0x8*/);
}
```

**Vtable slot references**

- `sector + 5616` and `sector + 5632` are sector-class owner getters (two-tier: primary + fallback). The sentinel `qword_1438863D0` represents "no explicit owner".
- `+0x70` on the representative object chains through enclosing contexts (NPC → room → deck → module → station).
- Class filter `110 = container` matches station, ship, and any other class derived from container, so the returned UniverseID might be a ship if the rep happens to live on one. In practice the rep lives on a station.

### 9.4 Fail Modes & Return Values

| Return | Condition |
|--------|-----------|
| `0` | `sectorid` doesn't resolve or isn't a sector |
| `0` | Sector has no owner (ownerless/contested with no fallback) |
| `0` | Galaxy root not yet initialized |
| `0` | Owner faction has no registry entry or wrong class |
| `0` | Faction has no law-enforcement representative set |
| `0` | Representative has no class-110 container ancestor |
| valid id | Success — the container hosting the rep |

All fail paths also log a descriptive message to the game log. Treat `0` as "no control station" and do not substitute a different station.

### 9.5 Usage Notes for Extensions

- **Call on the game thread** (uses per-thread time and singleton globals via vtable dispatch).
- **No caching of your own needed** — the function is a pointer chase plus one registry lookup; sub-microsecond.
- Good uses: identifying the sector's administrative/policing anchor, routing plot transactions, targeting a faction's "face" station for AI dialogue or reputation plays.
- Bad uses: picking a "random representative station" (returns 0 for ownerless sectors), building a list of stations per sector (use Option A from §5), or assuming it's the largest/most defended station.

---

## 10. RevealStations (Retired Stub — No-Op)

### 10.1 Signature & Address

```c
void RevealStations(void);
```

| Field | Value |
|-------|-------|
| Export ordinal | 0x72B @ `RVA 0x00095F00` → `0x140095F00` |
| Added | v9.00 (per `native/version_db/func_history.json`) |
| SDK | Listed in `sdk/x4_game_func_list.inc` (proto) |

### 10.2 Finding: Empty Body, Shared With Retired Cheats

`RevealStations` is a **no-op stub**. The export resolves to `0x140095F00`, which is an empty function whose body is a single `ret`. Multiple retired cheat/debug exports share the same address:

```
RevealStations
RevealMap
RevealEncyclopedia
EnableAllCheats
MakePlayerOwnerOf
MovePlayerToSectorPos
CheatDockingTraffic
CheatLiveStreamViewChannels
RequestLanguageChange
SetMouseVRPointerAllowed
SetMouseVRSensitivityPitch / Yaw
SetSavesCompressedOption
SetVRControllerAutoHide
SetVRVivePointerHand
SetVRViveTouchpadLockTime
SetVRWindowMode
ToggleScreenDisplayOption     // primary symbol name in the binary
```

All of these point to the same single-`ret` function. This is the classic "keep the export for compatibility, strip the implementation" pattern: the symbols remain callable (so older scripts don't crash the loader) but do nothing.

Decompiled body:

```c
void ToggleScreenDisplayOption(const wchar_t* a1, const wchar_t* a2, const wchar_t* a3)
{
    // empty — single ret instruction at 0x140095F00
}
```

### 10.3 Implications for Extensions

- **Calling `RevealStations()` has zero effect.** It does not make any station visible to the player or to any faction. Do not use it.
- If you need to reveal a station / sector / map area, the live mechanisms are:
  - `SetObjectForcedRadarVisible(id, true)` — type-71 objects only (see `VISIBILITY.md` §17).
  - `SetKnownTo(id, "player")` — marks an object as known to the given faction (via vtable+6016).
  - MD scripts using `<reveal_object>` and friends, or the visibility subsystem documented in `VISIBILITY.md`.
- The retired-stub pattern is a warning sign: if an FFI's name suggests a high-impact operation but the ordinal points to an address shared with unrelated names, it's likely a stub. Verify by checking xrefs and body before relying on it.

### 10.4 How to Detect Stubs Like This

For any suspicious export, an extension author can sanity-check by:

1. Looking up the export address in the PE (via `reference/x4_exports.txt`).
2. Searching `reference/x4_exports.txt` for other symbols mapped to the same RVA.
3. If several unrelated-sounding names share one RVA, the function is almost certainly a stub — confirm by disassembling (expect a single `ret` or `xor eax, eax; ret`).
