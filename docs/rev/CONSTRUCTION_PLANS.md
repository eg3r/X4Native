# Construction Plan Subsystem -- Reverse Engineering Notes

## Overview

Construction plans define station module layouts. `SpawnStationAtPos` reads a plan from the registry and creates all modules in one call. Plans can be imported from disk (`ImportMapConstructionPlan`) or created in memory (`ConstructionDB_CreatePlanDirect`).

## Plan Registry

- Global: `g_ConstructionPlanRegistry` at `0x146C7A3B8` (pointer to registry object)
- Data structure: Red-black tree. Sentinel/head at `registry+16`, root at `registry+24`
- Tree node layout: `+0=parent, +8=left, +16=right, +24=color(DWORD), +32=hash(uint64 FNV-1a), +40=plan_ptr`
- ~273 library plans loaded at startup (base game + DLCs)
- FNV-1a: seed=2166136261, prime=16777619, signed char XOR, 64-bit arithmetic
- **Plan name hashes are case-SENSITIVE** (no tolower). Macro lookups ARE lowercased.

## EditableConstructionPlan (296 bytes)

Three-level class hierarchy:
```
XLib::DBDataItem<3> (184 bytes base)
  +0   vtable
  +8   id_hash (uint64, FNV-1a of id string)
  +16  id (std::string, 32 bytes MSVC Release ABI)
  +48  name (std::string)
  +80  resolved_name (std::string)
  +112 desc (std::string)
  +144 resolved_desc (std::string)
  +176 xml_int_field (int32)

U::ConstructionPlan (extends to 296 bytes)
  +184 entries_begin (PlanEntry**)    <- std::vector<PlanEntry*>
  +192 entries_end (PlanEntry**)
  +200 entries_capacity (PlanEntry**)
  +208 bookmarks_begin
  +216 bookmarks_end
  +224 bookmarks_capacity
  +232 bookmark_flags (int32, 0)
  +236 bookmark_byte (uint8, 0)
  +240 source_type (int32: 0=library, 2=local, 3=external)
  +248 required_extension (std::string)
  +264 ext1 (=4)
  +272 ext2 (=0)
  +280 has_requirements (uint8, =1)
  +288 requirements_obj (ptr, nullptr)

U::EditableConstructionPlan (same size, different vtable)
  Only local (source_type=2) plans can be added/removed at runtime.
```

## PlanEntry (528 bytes)

```
+0   id (int64)              -- auto-assigned from global counter if 0
+8   macro_ptr (void*)       -- from MacroRegistry_Lookup
+16  connection_ptr (void*)  -- ConnectionEntry* from macro's connection array
+24  predecessor (PlanEntry*) -- nullptr = root module
+32  pred_connection_ptr     -- ConnectionEntry* on the predecessor's macro
+40  padding (8 bytes)
+48  transform (64 bytes)    -- 4x __m128: [position, rot_row0, rot_row1, rot_row2]
+112 loadout (408 bytes)     -- default-initialized by PlanEntry_Construct
+520 is_fixed (bool)
+521 is_modified (bool)
+522 is_bookmark (bool)
+523 padding (5 bytes)
```

- Allocate via `x4n::memory::game_alloc<X4PlanEntry>()` (or raw 528 bytes, 16-byte aligned)
- Initialize via `PlanEntry_Construct(ptr, macro, conn, pred, pred_conn, transform, loadout, fixed, modified, bookmark, id)` at `0x140D10960`
- ID counter global: `g_PlanEntryIDCounter` at `0x14387E8E0` (under `g_PlanEntryIDCritSec` at `0x143CF41B0`)

### Transform Layout (64 bytes at +48)

```
+48  position  = {x, y, z, 0.0f}        (16 bytes, __m128)
+64  rot_row0  = {r00, r01, r02, 0.0f}  (16 bytes, __m128)
+80  rot_row1  = {r10, r11, r12, 0.0f}  (16 bytes, __m128)
+96  rot_row2  = {r20, r21, r22, 0.0f}  (16 bytes, __m128)
```

Identity rotation: `{1,0,0,0}, {0,1,0,0}, {0,0,1,0}` (constants at `0x14226AE60/AE70/AE80`).

### Validation Rule (ConstructionPlan_Append at `0x140D0D050`)

Connection, predecessor, and pred_connection must be **all set or all null**:
- `connection_ptr->template_ptr` (at conn+0x30) must equal `macro_ptr->template_ptr` (+0x48)
- `pred_connection_ptr->template_ptr` must equal predecessor's `macro_ptr->template_ptr`
- If any of the three is null, all must be null (auto-placement mode)

## UIConstructionPlanEntry (80 bytes, FFI struct)

```
+0   idx (size_t)                  -- 0-based index in plan array (NOT PlanEntry.id)
+8   macroid (const char*)         -- macro name string
+16  componentid (UniverseID)      -- runtime module ID (from component registry)
+24  offset.x (float)              -- position x
+28  offset.y (float)              -- position y
+32  offset.z (float)              -- position z
+36  offset.yaw (float)            -- euler yaw in degrees
+40  offset.pitch (float)          -- euler pitch in degrees
+44  offset.roll (float)           -- euler roll in degrees
+48  connectionid (const char*)    -- connection name on this module
+56  predecessoridx (size_t)       -- index of predecessor (array_size = no predecessor)
+64  predecessorconnectionid (const char*) -- connection name on predecessor
+72  isfixed (bool)                -- build UI lock flag
+73  padding (7 bytes)
```

UIConstructionPlanEntry2 adds `bookmarknum (uint32)` at +76, same 80-byte stride.

### Field Mapping: X4PlanEntry -> UIConstructionPlanEntry

| PlanEntry | UIEntry | Conversion |
|-----------|---------|------------|
| array index | idx (+0) | 0-based position in vector, NOT PlanEntry.id |
| macro_ptr->name(+32) | macroid (+8) | SSO string pointer |
| RB-tree lookup | componentid (+16) | Entry.id mapped to spawned module's UniverseID |
| +48,+52,+56 | offset.x/y/z (+24..+32) | Direct float copy |
| rot_rows +64/+80/+96 | offset.yaw (+36) | `atan2f(-row2[0], row2[2]) * (-180/pi)` |
| rot_rows +64/+80/+96 | offset.pitch (+40) | `asinf(clamp(row2[1], -1, 1)) * (180/pi)` |
| rot_rows +64/+80/+96 | offset.roll (+44) | `atan2f(-row0[1], row1[1]) * (180/pi)` |
| connection_ptr->name(+16) | connectionid (+48) | SSO string, nullptr -> "" |
| predecessor index | predecessoridx (+56) | Linear scan for pointer in array; nullptr -> array_size sentinel |
| pred_conn->name(+16) | predecessorconnectionid (+64) | SSO string, nullptr -> "" |
| +520 | isfixed (+72) | Direct bool copy |

### Euler Extraction (Matrix -> Degrees)

The rotation matrix decomposition uses YXZ Tait-Bryan convention:
```c
yaw   = atan2f(-row2[0], row2[2]) * (-180.0f / PI);  // double negation
pitch = asinf(clamp(row2[1], -1.0f, 1.0f)) * (180.0f / PI);
roll  = atan2f(-row0[1], row1[1]) * (180.0f / PI);
```

Addresses: `0x14019D189..0x14019D1FC` (GetNumPlannedStationModules), `0x14018F83C..0x14018F8AD` (GetBuildMapConstructionPlan2).

RAD2DEG constant: 57.295776f (0x42652EE1).

### Euler Reconstruction (Degrees -> Matrix)

The inverse operation for plan injection:
```c
y = -yaw_deg * DEG2RAD;   // sign flip matches extraction convention
p = pitch_deg * DEG2RAD;
r = roll_deg * DEG2RAD;
// Matrix = Ry * Rx * Rz (YXZ order)
row0 = { cy*cr + sy*sp*sr,  -cy*sr + sy*sp*cr,  sy*cp,  0 }
row1 = { cp*sr,              cp*cr,              -sp,    0 }
row2 = { -sy*cr + cy*sp*sr,  sy*sr + cy*sp*cr,   cy*cp, 0 }
```

## XML Import Pipeline

```
ImportMapConstructionPlan(filename, plan_id)            @ 0x14019FC30
  -> ConstructionDB_ImportLocal(registry, id, filename) @ 0x140D10060
    1. Resolve file path (data dir + extension, "xml.gz xml")
    2. Parse XML via sub_140E0B160 -> internal DOM tree
    3. Validate XML element type == 36
    4. XMLPlan_FindPlanByID (sub_140E0C180) -> locate <plan> element
    5. Allocate 0x128 (296) bytes for EditableConstructionPlan
    6. ConstructionPlan_Construct_FromXML(plan, 2, xml) @ 0x140D0E760
    7. Set U::EditableConstructionPlan vtable
    8. ConstructionDB_AddPlan @ 0x140D151B0 -> insert into RB-tree
```

### XML <offset> -> Transform Conversion (@ 0x140E1EDC0)

The `<offset>` element stores position + rotation. Position is always x/y/z floats. Rotation has two representations:

1. **Quaternion** (most common in XML plans):
   - `<quaternion qx="..." qy="..." qz="..." qw="..."/>`
   - Converted to 3x3 rotation matrix via `Quaternion_ToRotationMatrix` @ 0x1400D18C0
   - Input: `[x, y, z, w]` quaternion

2. **Direct rotation** (rare, alternate format):
   - `<rotation>` element parsed by `sub_140E258D0`

The game's internal format is always **rotation matrix**, never euler angles or quaternion. Euler angles only exist in the FFI output (UIConstructionPlanEntry).

### Quaternion -> Matrix (@ 0x1400D18C0)

Standard conversion:
```c
void quat_to_matrix(float out[12], float q[4]) {  // q=[x,y,z,w]
    float xx2=2*x*x, yy2=2*y*y, zz2=2*z*z;
    float xy2=2*x*y, xz2=2*x*z, yz2=2*y*z;
    float wx2=2*w*x, wy2=2*w*y, wz2=2*w*z;
    row0 = {1-yy2-zz2, xy2+wz2,   xz2-wy2,   0};
    row1 = {xy2-wz2,   1-xx2-zz2, yz2+wx2,    0};
    row2 = {xz2+wy2,   yz2-wx2,   1-xx2-yy2,  0};
}
```

## MacroData Layout

Returned by `MacroRegistry_Lookup`. Contains the macro's connection points.

```
+0x00  vtable
+0x08  FNV-1a hash of macro name (uint64)
+0x20  name (std::string, SSO)
+0x44  class_id (int32)
+0x48  template_ptr (void*) -- used for connection validation
+0x170 connection_array_start (void*)
+0x178 connection_array_end (void*)
```

Connection count = `(end - start) / 352`.

Valid macro class IDs for station modules: 0x71-0x76 (checked via classification table at `off_14250F390`).

## ConnectionEntry (352 bytes, stride 0x160)

Stored in a **sorted array** (by FNV-1a hash) within each MacroData object.

```
+0x00  type/flags (uint64)
+0x08  FNV-1a hash of lowercased name (uint64, used for binary search)
+0x10  name (std::string, SSO)
+0x30  template_ptr (void*) -- must match MacroData+0x48 for validation
+0x38  connection geometry, snap point data, tags (288 bytes)
```

### Connection Resolution Algorithm

There is **no standalone function** for this -- every call site inlines the same pattern.
Implemented as `x4n::plans::resolve_connection()` in the SDK (`x4n_plans.h`).

```
1. Lowercase the connection name string
2. Compute FNV-1a hash (seed=0x811C9DC5, prime=0x1000193, signed char, 64-bit)
3. Binary search in sorted array at MacroData+0x170, stride 352, compare hash at entry+8
4. Return ConnectionEntry* or nullptr
```

Confirmed at three independent call sites:
- `0x140E10335` -- connection resolution in XML import
- `0x140E107AF` -- predecessor connection resolution in XML import
- Inline in `x4n::plans::resolve_connection()` (SDK reimplementation, `x4n_plans.h`)

## Key Functions

| Function | Address | Size | Purpose |
|----------|---------|------|---------|
| `GetNumPlannedStationModules` | `0x14019DCD0` | 0x614 | Read live plan -> cache UIConstructionPlanEntry[] |
| `GetPlannedStationModules` | `0x14019E7E0` | 0xA7 | Copy cached entries to caller buffer |
| `GetBuildMapConstructionPlan2` | `0x1401903B0` | 0x5C7 | Same but UIConstructionPlanEntry2 (+bookmarknum) |
| `ImportMapConstructionPlan` | `0x14019FC30` | 0x174 | FFI wrapper for ConstructionDB_ImportLocal |
| `RemoveConstructionPlan` | `0x1401A0B00` | 0x481 | Deregister + destroy plan |
| `SpawnStationAtPos` | `0x1401B8530` | 0x88C | Create station from registered plan |
| `ConstructionDB_ImportLocal` | `0x140D10060` | 0x7E3 | Full XML import: parse + construct + register |
| `ConstructionDB_AddPlan` | `0x140D151B0` | 0x199 | Insert plan into RB-tree registry |
| `ConstructionDB_CreatePlanDirect` | `0x140D15350` | 0xB4 | Alloc + construct + register EditableConstructionPlan |
| `ConstructionDB_ImportFromXML_Plans` | `0x140D10BB0` | 0x16B | Bulk import plans from XML DOM |
| `PlanEntry_Construct` | `0x140D10960` | 0xD8 | Initialize 528-byte PlanEntry in-place |
| `ConstructionPlan_Append` | `0x140D0D050` | 0x778 | Validate + append entry (connection validation) |
| `ConstructionPlan_Construct_FromXML` | `0x140D0E760` | 0x844 | XML data -> ConstructionPlan object |
| `Loadout_Init` | `0x1400EF140` | 0x138 | Default-initialize 408-byte loadout block |
| `Quaternion_ToRotationMatrix` | `0x1400D18C0` | -- | [x,y,z,w] -> 3x3 row-major matrix |
| `MacroRegistry_Lookup_Inner` | `0x1409EA820` | 0xB40 | Resolve macro name string to MacroData* |
| `ComponentFactory_Create` | `0x14089BD70` | 0x292C | Universal entity factory (70 callers, station=case 96) |
| `GetStationDefaults` | `0x140818EB0` | 0x97 | Resolve station defaults (child conn at +0x7A8) |
| `GetZoneDefaults` | `0x14081A310` | 0x9A | Resolve zone defaults (parent conn at +0x558) |
| `Component_GetComponentDefaults` | `0x140453FE0` | 0x8F | Resolve component defaults (root module check at +0x191) |
| `CreateStationAction_Construct` | `0x140B5B910` | 0x930 | MD create_station handler constructor (Scripts::CreateStationAction) |
| `CreateStationAction_Execute` | `0x140B5C4E0` | 0xFE9 | MD create_station execution (the actual creation logic) |
| `CreateFactoryAction_Construct` | `0x140B56EF0` | 0x68A | MD create_factory handler constructor (Scripts::CreateFactoryAction) |
| `CreateStation_Inner` | `0x1407F8D70` | 0xD0 | Wrapper: GetDefaults + CreateStation_SpawnInZone |
| `CreateStation_SpawnInZone` | `0x14081E910` | 0x204 | Core: validates connections + ComponentFactory_Create |
| `BuildTask_Construct` | `0x14036D360` | 0x618 | Creates U::BuildTask (0x390 bytes) for staged construction |
| `AddBuildToExpandStationAction_Construct` | `0x140B4E860` | 0x1CB | MD add_build_to_expand_station handler constructor |
| `MD_ActionDispatch` | `0x140B1AB00` | 0x13254 | Main MD action dispatch (giant switch on action ID) |

## Macro Registry

- Global: `g_MacroRegistry` at `0x146C7A248`
- BST at `registry+64`
- `MacroRegistry_Lookup_Inner(registry, string_view*, silent)` -- lowercased FNV-1a lookup
- Falls back to loading XML asset files if not cached
- string_view = `{const char* data, size_t length}` (16 bytes)

## ABI Constraint

Internal functions taking `std::string*` expect MSVC Release layout (32 bytes). Debug builds add an 8-byte `_Container_proxy*` prefix (40 bytes total), causing field misalignment and crashes. **Build with RelWithDebInfo or Release.**

## Station Creation Pipeline (MD `create_station`)

The full pipeline from MD script to spawned station entity:

### Phase 1: Station Spawning (MD `create_station`)

```
MD action dispatch (MD_ActionDispatch @ 0x140B1AB00)
  -> case 0x707 (1799): allocate 0x430 bytes
  -> CreateStationAction_Construct @ 0x140B5B910
     Sets vtable: Scripts::CreateStationAction
     Parses MD params: zone(+40), sector(+56), macro(+72), owner(+88),
       constructionplan(+104), position/safepos(+320/+584), rotation(+528),
       buildmethod(+968/+976), rawname(+288), stage(+120)

  -> CreateStationAction_Execute @ 0x140B5C4E0
     1. Resolve zone or sector from params
     2. Resolve station macro via MacroRegistry (lowercased FNV-1a)
     3. Resolve faction owner via g_FactionManager FNV-1a lookup
     4. Resolve construction plan: either from constructionsequence value
        (direct pointer at plan+184) or from plan ID string via
        g_ConstructionPlanRegistry FNV-1a RB-tree lookup
     5. Compute safe position (via sub_140C2C1A0 or sub_140C295B0)
     6. Call CreateStation_Inner @ 0x1407F8D70
        -> GetStationDefaults(macro) -> child connection at +0x7A8
        -> GetZoneDefaults(zone+0x60) -> parent connection at +0x558
        -> CreateStation_SpawnInZone @ 0x14081E910
           -> Validates connections (parent+child must both be non-null)
           -> ComponentFactory_Create(case 96 = station class)
           -> Returns station component if class check passes
     7. Set faction owner via vtable+5720 call
     8. Create U::ScriptSpawnSource or U::DropSpawnSource
     9. If construction plan has stages, create BuildTask via
        BuildTask_Construct @ 0x14036D360
```

### Phase 2: Build Task Queuing (MD `add_build_to_expand_station`)

```
MD action dispatch
  -> case 0x661 (1633): allocate 0xF0 bytes
  -> AddBuildToExpandStationAction_Construct @ 0x140B4E860
     Sets vtable: Scripts::AddBuildToExpandStationAction
     Params: object(+40 = buildstorage), buildobject(+56 = station),
       constructionplan(+72), loadout(+88/+120), result(+192/+208)
```

The `add_build_to_expand_station` creates build tasks in the station's build
storage. Each task represents one module to be constructed. The task includes:
- Construction plan reference
- Module macro and loadout
- Resource requirements (derived from module ware costs)

### Phase 3: Builder Ship Assignment

Builder ships (construction vessels, CVs) are **not assigned by the station creation
code**. Instead, the builder assignment is entirely AI-driven:

1. Builder ships run `order.build.find.task` AI script
2. Stations signal `'request construction vessel'` on their build storage
3. Builder ships respond via interrupt handler in `order.build.find.task.xml`:
   - Check if builder is available and in range
   - Check faction/player ownership relations
   - `assign_construction_vessel object="this.ship" buildmodule="event.param2"`
4. Builder flies to station, starts construction
5. Events: `event_build_started`, `event_build_finished` on the build storage

### Phase 4: Resource Delivery and Construction

Resources are delivered to the build storage by normal economy trading:
- Build storage has buy offers for required construction resources
- Freighters/traders fulfill these buy offers
- Builder ship processes resources into module construction progress

### NPC Faction Economy

**Critical finding: NPC factions have NO budget system for station construction.**
There is no money/credit deduction when a faction builds a station. The pipeline is:
1. `create_station` creates an invisible station entity
2. `add_build_to_expand_station` queues build tasks
3. Resources are delivered by the economy (no cost to the faction)
4. Builder ships construct modules (also no direct cost)
5. Only the PLAYER pays for station construction (via build plot costs, resource purchases)

This means injecting builds for NPC factions is economically safe -- there is no
budget to break.

## SpawnStationAtPos vs create_station

`SpawnStationAtPos` (FFI export @ `0x1401B8530`, size 0x88C) is a **god-mode
instant station creation** function. Comparing with the MD pipeline:

| Aspect | SpawnStationAtPos | MD create_station |
|--------|-------------------|-------------------|
| Entry point | FFI/Lua callable | MD action dispatch |
| Macro resolution | Same (MacroRegistry_Lookup_Inner) | Same |
| Plan lookup | Same (g_ConstructionPlanRegistry FNV-1a) | Same, plus constructionsequence support |
| Position | Direct UIPosRot | Safe position calculation with plot box |
| Zone handling | Finds existing or creates new zone | Same |
| Core call | ComponentFactory_Create directly | CreateStation_SpawnInZone -> ComponentFactory_Create |
| Build tasks | None -- all modules instant | Optional staged via BuildTask_Construct |
| Builder ships | Not needed | Needed for staged construction |
| Source tracking | U::DropSpawnSource | U::ScriptSpawnSource |
| Build method | None | Configurable (station builder, etc.) |

Both ultimately call `ComponentFactory_Create` with `a9=1` (station flag) and
pass `plan+184` (the PlanEntry** vector) as the module list.

## U::BuildTask Layout (0x390 bytes)

```
+0    vtable (U::BuildTask::`vftable')
+16   state (DWORD, 0=pending, 2=active)
+24   ptr1
+32   station_ID (uint64)
+40   is_assigned (bool)
+48   ptr3
+56   faction_owner_ptr
+64   game_thread_ptr (from TLS+768)
+72   plan_entries_begin (PlanEntry**)     -- copied from construction plan
+80   plan_entries_end
+88   plan_entries_capacity
+96   bookmarks_begin
+104  bookmarks_end
+112  bookmarks_capacity
+120  current_bookmark (DWORD)             -- stage index
+124  bookmark_flag (bool)
+128  resource_tree (RB-tree root for required resources)
+288  loadout (408 bytes, via Loadout_Init)
+696  resource_state (DWORD)
+720  additional_info_ptr
+728  reserved
+736  reserved
+752  position (__m128: x, y, z, 0)
+768  rot_row0 (__m128, identity default)
+784  rot_row1 (__m128, identity default)
+800  rot_row2 (__m128, identity default)
+816  flags (WORD)
+824-896 timing/progress fields
```

## MD Action IDs (Station-Related)

| Action | ID (hex) | ID (dec) | Handler Constructor | Handler Size |
|--------|----------|----------|---------------------|-------------|
| `create_station` | 0x707 | 1799 | `0x140B5B910` | 0x430 |
| `create_factory` | 0x6E8 | 1768 | `0x140B56EF0` | 0x638 |
| `add_build_to_expand_station` | 0x661 | 1633 | `0x140B4E860` | 0xF0 |

## Station Finalization

NPC stations are created from library plans but then modified by:
1. `finalisestations.xml` -- adds habitation, docks, defense, connection modules
2. `factionlogic_economy.xml` -- dynamically adds production modules over time

The live module graph does NOT match the origin library plan. Full module serialization via `GetPlannedStationModules` is required for accurate replication.
