# Construction Plan Subsystem -- Reverse Engineering Notes

> Verified: v9.00 build 900 (R-Station4 + connection resolution + conversion trace research, 2026-03-23)

## Overview

Construction plans define station module layouts. `SpawnStationAtPos` reads a plan from the registry and creates all modules in one call. Plans can be imported from disk (`ImportMapConstructionPlan`) or created in memory (`ConstructionDB_CreatePlanDirect`).

## Plan Registry

- Global: `g_ConstructionPlanRegistry` at RVA `0x06C73FA0` (pointer to registry object)
- Data structure: Red-black tree at `registry+16`
- Tree node layout: `+0=parent, +8=left, +16=right, +24=color, +32=hash(uint64 FNV-1a), +40=plan_ptr`
- ~273 library plans loaded at startup (base game + DLCs)
- FNV-1a: seed=2166136261, prime=16777619, signed char XOR, 64-bit arithmetic

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

- Allocate via `x4n::memory::game_alloc<X4PlanEntry>()` (or raw: `GameAlloc(528, 0, 0, 0, 16)`)
- Initialize via `PlanEntry_Construct(ptr, macro, conn, pred, pred_conn, transform, loadout, fixed, modified, bookmark, id)`
- ID counter global: `0x1438778A0` (under critical section at `0x143CECC60`)

### Transform Layout (64 bytes at +48)

```
+48  position  = {x, y, z, 0.0f}        (16 bytes, __m128)
+64  rot_row0  = {r00, r01, r02, 0.0f}  (16 bytes, __m128)
+80  rot_row1  = {r10, r11, r12, 0.0f}  (16 bytes, __m128)
+96  rot_row2  = {r20, r21, r22, 0.0f}  (16 bytes, __m128)
```

Identity rotation: `{1,0,0,0}, {0,1,0,0}, {0,0,1,0}` (constants at 0x142266DE0/DF0/E00).

### Validation Rule (ConstructionPlan_Append at 0x140D0AEC0)

Connection, predecessor, and pred_connection must be **all set or all null**:
- `connection_ptr->template_ptr` must equal `macro_ptr->template_ptr` (+0x48)
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
ImportMapConstructionPlan(filename, plan_id)            @ 0x14019ED60
  -> ConstructionDB_ImportLocal(registry, id, filename) @ 0x140D0D640
    1. Resolve file path (data dir + extension, .xml.gz / .xml)
    2. Parse XML -> internal DOM tree
    3. XMLPlan_FindPlanByID -> locate <plan> element
    4. Allocate 296 bytes (EditableConstructionPlan)
    5. ConstructionPlan_Construct_FromXML(plan, 2, xml) @ 0x140D0BD40
       -> ConstructionPlan_PopulateEntries_FromXML      @ 0x140E0FD50
    6. Set EditableConstructionPlan vtable
    7. ConstructionDB_AddPlan -> insert into RB-tree
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

| Function | RVA | Purpose |
|----------|-----|---------|
| `GetNumPlannedStationModules` | `0x0019CE00` | Read live plan -> cache UIConstructionPlanEntry[] |
| `GetPlannedStationModules` | `0x0019D910` | Copy cached entries to caller buffer |
| `GetBuildMapConstructionPlan2` | `0x0018F4E0` | Same but UIConstructionPlanEntry2 (+bookmarknum) |
| `ConstructionDB_CreatePlanDirect` | `0x00D0E680` | Alloc + construct + register EditableConstructionPlan |
| `ConstructionDB_AddPlan` | `0x00D0E4E0` | Insert plan into RB-tree registry |
| `PlanEntry_Construct` | `0x00D09C90` | Initialize 528-byte PlanEntry in-place |
| `ConstructionPlan_CopyEntries` | `0x00D09E60` | Copy entries between plans |
| `ConstructionPlan_Append` | `0x00D0AEC0` | Validate + append entry (connection validation) |
| `ConstructionPlan_Construct_FromXML` | `0x00D0BD40` | XML data -> ConstructionPlan object |
| `ConstructionPlan_PopulateEntries_FromXML` | `0x00E0FD50` | XML -> PlanEntry[] (1400 insns) |
| `XML_ParseOffset_ToTransform` | `0x00E1EDC0` | XML <offset> -> position + rotation matrix |
| `Quaternion_ToRotationMatrix` | `0x000D18C0` | [x,y,z,w] -> 3x3 row-major matrix |
| `ConstructionDB_ImportLocal` | `0x00D0D640` | Full XML import: parse + construct + register |
| `ImportMapConstructionPlan` | `0x0019ED60` | FFI wrapper for ConstructionDB_ImportLocal |
| `MacroRegistry_Lookup` | `0x009E72B0` | Resolve macro name string to MacroData* |
| `Station_InitFromPlan` | `0x00488120` | Spawn consumes plan entries for module creation |
| `ComponentFactory_Create` | `0x0089A400` | Universal entity factory (station=case 96) |
| `GameAlloc` | `0x0145CB90` | SMem pool allocator |
| `RemoveConstructionPlan` | `0x0019FC30` | Deregister + destroy plan |
| `SpawnStationAtPos` | `0x001B7660` | Create station from registered plan |

## Macro Registry

- Global: `g_MacroRegistry` at RVA `0x06C73E30`
- BST at `registry+64`
- `MacroRegistry_Lookup(registry, string_view*, silent)` -- lowercased FNV-1a lookup
- Falls back to loading XML asset files if not cached
- string_view = `{const char* data, size_t length}` (16 bytes)

## ABI Constraint

Internal functions taking `std::string*` expect MSVC Release layout (32 bytes). Debug builds add an 8-byte `_Container_proxy*` prefix (40 bytes total), causing field misalignment and crashes. **Build with RelWithDebInfo or Release.**

## Station Finalization

NPC stations are created from library plans but then modified by:
1. `finalisestations.xml` -- adds habitation, docks, defense, connection modules
2. `factionlogic_economy.xml` -- dynamically adds production modules over time

The live module graph does NOT match the origin library plan. Full module serialization via `GetPlannedStationModules` is required for accurate replication.
