# X4 Production Module Internals — Reverse Engineering Notes

> **Binary:** X4.exe v9.00 · **Date:** 2026-04

---

## 1. Summary

Production modules are station sub-entities (class 78 Production, class 77 Processingmodule) that manufacture wares. The game exposes FFI functions for enumeration, pause/resume, and production rate queries. Key constraint: `PauseProductionModule` FFI enforces player-ownership.

---

## 2. Class Hierarchy

```
Component (21)
  └─ Entity (43)
       └─ Object (72)
            └─ Container (110)
                 └─ Controllable (111)
                      └─ Defensible
                           └─ Module (114)
                                ├─ Production (78)
                                └─ Processingmodule (77)
```

---

## 3. Module Enumeration

```c
// Get all modules on a station
uint32_t count = GetNumStationModules(station_id, false, false);
UniverseID modules[count];
GetStationModules(modules, count, station_id, false, false);

// Filter to production/processing
for (int i = 0; i < count; i++) {
    if (IsRealComponentClass(modules[i], "production"))       { /* manufacturing */ }
    if (IsRealComponentClass(modules[i], "processingmodule")) { /* raw processing */ }
    if (IsComponentOperational(modules[i]))                   { /* not wrecked/building */ }
}
```

Alternative (vtable, no FFI): `x4n::entity::is_derived_from(id, GameClass::Production)`.

---

## 4. Ware-to-Module Mapping

**No direct FFI** to query "what ware does this module produce." The game's Lua UI resolves this via XML library lookups. For native code, parse the macro name:

```
prod_gen_energycells_macro     → energycells
prod_arg_foodrations_macro     → foodrations
prod_tel_sunriseflowers_macro  → sunriseflowers
prod_ter_computronicsubstrate_macro → computronicsubstrate
```

Pattern: `prod_<race>_<wareid>_macro`. Race prefixes: `gen`, `arg`, `tel`, `par`, `ter`, `bor`, `spl`.

Use `x4n::entity::get_component_macro(module_id)` to read the macro, then extract the ware_id substring between the second `_` and `_macro`.

---

## 5. Pause/Resume

### 5.1 FFI Functions

```c
void PauseProductionModule(UniverseID moduleid, bool pause);   // class 78
void PauseProcessingModule(UniverseID moduleid, bool pause);   // class 77
```

### 5.2 PauseProductionModule @ 0x14017CB90

Decompiled behavior:
1. Resolves component via `ComponentRegistry_Find(registry, id, 4)`
2. Validates class 78 (Production)
3. **Ownership check**: compares module's owner (vtable call at +5624) against the player faction global (`qword_14388AB00`). If not player-owned, logs error and aborts:
   > `"PauseProductionModule(): Given productionmodule '%s' is not player-owned! Aborting call."`
4. On pause: sets byte at `module+0x3F4` to 1, calls production shutdown routine
5. On resume: sets byte at `module+0x3F4` to 0, calls production restart routine

### 5.3 Internal State (offsets from module base)

| Offset | Type | Field | Notes |
|--------|------|-------|-------|
| +0x3A8 (936) | int32 | recipe_index | -1 = no recipe assigned |
| +0x3B0 (944) | double | rate_multiplier | production speed factor |
| +0x3B8 (952) | double | consumption_multiplier | resource consumption factor |
| +0x3D8 (984) | ptr | recipe_begin | start of recipe array (96 bytes per entry) |
| +0x3E0 (992) | ptr | recipe_end | end of recipe array |
| +0x3F0 (1008) | int32 | production_state | 1=uninitialized, 8=active |
| +0x3F4 (1012) | uint8 | **paused** | 0=running, 1=manually paused |

### 5.4 Player-Only Limitation

The FFI `PauseProductionModule` explicitly checks player ownership and refuses NPC modules. For NPC faction control, alternatives:
- **MD bridge**: `set_production_paused` MD action works on any faction (used by `factionlogic_economy.xml`)
- **Direct write**: set byte at `module+0x3F4` (bypasses ownership check, but skips shutdown/restart routines)

---

## 6. GetContainerWareProduction @ 0x1401AA460

Station-level query: "how much of ware X does this station produce per hour?"

```c
double GetContainerWareProduction(UniverseID station, const char* wareid, bool ignorestate);
```

### 6.1 ignorestate Behavior (IDA confirmed)

The module iteration loop checks:
```c
state = *(int32*)(module + 1008);       // production_state
if (state != 1                          // skip uninitialized
    && *(int32*)(module + 936) != -1    // must have recipe
    && (ignorestate || state == 8))     // ignorestate=true bypasses active check
```

**When `ignorestate=true`**: includes ALL modules with valid recipes, regardless of production state (paused, starved, etc.). Reports theoretical capacity.

**When `ignorestate=false`**: only counts modules in state 8 (actively producing). Reports actual current output.

### 6.2 Implication for Pause/Resume

Using `GetContainerWareProduction(station, ware, true)` as a pre-check before pause/resume is **safe**: it returns > 0 even when all modules for that ware are paused, allowing the subsequent module iteration to find and unpause them.

---

## 7. SDK API

Two headers, separate concerns:

- `x4n_module.h` — `x4n::module::Module` (standalone module handle)
- `x4n_station.h` — `x4n::station::Station` (station handle, returns modules)

Constructors validate class via vtable — passing a wrong entity type results in `valid() == false`.

```cpp
// Module-level access
x4n::module::Module mod(module_id);   // validates GameClass::Module
if (mod.is_production()) {
    mod.ware_id();       // "energycells" (parsed from macro name)
    mod.set_paused(true); // calls PauseProductionModule FFI
}

// Station-level access
x4n::station::Station station(station_id);   // validates GameClass::Station
station.produces("energycells");              // fast station-level check (ignorestate=true)
station.modules();                            // returns vector<x4n::module::Module>

// All-in-one: find matching modules, pause them, return count
uint32_t affected = station.set_ware_paused("energycells", true);
```

### Ware-to-module matching

`Module::ware_id()` parses the macro name (`prod_<race>_<wareid>_macro`). No FFI exists for this query — the game's own Lua UI uses XML library lookups instead. Macro parsing is the only native-code path.
