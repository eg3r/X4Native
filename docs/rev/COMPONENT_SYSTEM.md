# X4 Component System — Entity Base Layout, Registry, Class Hierarchy

> **Binary:** X4.exe v9.00 | **Date:** 2026-03-27 | **Updated:** 2026-03-31 (build 603098)
>
> All addresses are absolute (imagebase `0x140000000`).
>
> SDK types: `x4_manual_types.h` (`X4Component`), `x4n_entity.h`, `x4n_galaxy.h`.

---

## 1. Overview

Every game entity — sectors, clusters, stations, ships, NPCs, zones, rooms — is an `X4Component`. All share a common base struct layout. Subclass-specific fields (visibility bytes, hull/shield, faction) begin at higher offsets.

The game engine stores all components in a global registry indexed by `UniverseID`. Lookup is O(1) via page-based indexing (no hash map, no tree traversal). See §5.

The component hierarchy forms a scene graph: Galaxy → Cluster → Sector → Zone → Station/Ship → Module → Room → NPC.

---

## 2. X4Component Base Struct

Confirmed by decompiling functions listed in the Source column.

```
Offset  Size  Type              Field              Source Functions
------  ----  ----              -----              ----------------
+0x00   8     void*             vtable             All (~800+ slots, see §4)
+0x08   8     uint64_t          id                 GetClusters_Lua, GetParentComponent,
                                (UniverseID)       RemoveComponent, ChildComponent_Enumerate
                                                   NOTE: same field as raw generation seed
+0x10   32    ?                 (unresolved)       +0x10..+0x2F: internal engine bookkeeping
+0x30   8     void*             definition         GetComponentName (vtable[3] = GetName)
                                                   GetComponentData "macro" (vtable[4] = GetMacroName)
                                                   Embedded sub-object — this ptr = component+0x30
+0x38   8     void*             ctrl_vtable        AddSector: shared_ptr control block vtable
+0x40   4     int32_t (atomic)  ref_count          AddSector: lock xadd [rdi+40h]
+0x44   4     int32_t (atomic)  weak_count         AddSector: lock cmpxchg [rdi+44h] (states 1/2/3)
+0x48   32    ?                 (unresolved)       +0x48..+0x67
+0x68   4     int32_t           class_id           ChildComponent_Enumerate: 1 << *(DWORD*)(child+0x68)
                                                   Values: see §7 class table
                                                   NOT a validity flag (was previously misidentified)
+0x6C   4     ?                 (padding)
+0x70   8     X4Component*      parent             GetParentComponent, GetContextByClass,
                                                   Component_ComputeWorldTransform (parent chain walk)
+0x78   48    ?                 (unresolved)       +0x78..+0xA7
+0xA8   ?     ChildContainer    children           GetClusters_Lua, GetSectors_Lua, GetStationModules
                                                   Bucketed hash map (see §3)
+0xD1   1     uint8_t           exists             GetSectors_Lua: cmp byte ptr [rax+0D1h], 0
                                                   0=destroyed, nonzero=alive
+0x3C0  8     int64_t           combined_seed      raw_seed + session_seed (= MD $Station.seed)
```

### 2.1 Definition Interface (+0x30)

The field at +0x30 is an **embedded sub-object** (not a pointer to an external object). Its vtable pointer lives at +0x30, and `this` for virtual calls is `component + 0x30`.

| Vtable Slot | Byte Offset | Function | Returns |
|-------------|-------------|----------|---------|
| 3 | +0x18 | `GetName()` | `std::string*` (display name) |
| 4 | +0x20 | `GetMacroName()` | `std::string*` (macro identifier) |

```asm
; GetComponentName (0x140151B60):
mov     rax, [rbx+30h]       ; load definition vtable
lea     rcx, [rbx+30h]       ; this = component+0x30
call    qword ptr [rax+18h]  ; vtable[3] = GetName()
```

The returned `std::string*` follows MSVC x64 SSO layout: if `capacity` (at str+24) < 16, string data is inline at str+0; otherwise str+0 is a heap pointer.

SDK helper: `x4n::entity::get_component_macro(component)`.

### 2.2 Shared_ptr Control Block (+0x38)

Standard MSVC `shared_ptr` control block:

```
+0x38: void*    ctrl_vtable    — control block vtable ([0] = release/destroy)
+0x40: int32_t  ref_count      — atomic reference count (lock xadd)
+0x44: int32_t  weak_count     — atomic weak ref / lifecycle state
                                 1=alive, 2=ref'd, 3=releasing (lock cmpxchg)
```

### 2.3 Corrected: +0x68 is Class ID, NOT Validity Flag

**Critical correction** from earlier documentation. The field at +0x68 was described as "Internal state flags" but is actually the **runtime class ID** (same values as `X4_CLASS_*` defines). Evidence:

- `ChildComponent_Enumerate` (`0x1402F9B80`): `1 << *(_DWORD *)(child + 104)` where 104 = 0x68
- Used as class bitmask filter in all child enumerators (61 callers)
- The **actual** existence flag is the byte at +0xD1 (209)

---

## 3. Child Container (+0xA8)

> **Corrected 2026-03-28.** Previously described as "bucketed hash map" — this was **wrong**. No hash function exists. The structure is a group-indexed partition array with direct arithmetic indexing.

The field at `component+0xA8` is a **pointer** to a child container object. The container is a **group-indexed partition array** — a fixed-size array of vectors (buckets), one per entity group. Children belonging to similar entity types share the same group bucket. Lookup is O(1) direct array indexing, not hashing.

### 3.1 Container Object Layout

`component+0xA8` stores a pointer. The pointed-to object:

```
container + 0x00: ???                  (8 bytes — not accessed by enumerator)
container + 0x08: bucket_array_begin   (pointer to array of 32-byte buckets)
container + 0x10: bucket_array_end     (pointer past last bucket)
container + 0x18: ???                  (not accessed)
container + 0x20: total_child_count    (DWORD — sum across all buckets)
```

### 3.2 Bucket Layout (32 bytes each)

```
bucket + 0x00: child_ptr_begin   (pointer to X4Component*[])
bucket + 0x08: child_ptr_end     (pointer past last child)
bucket + 0x10: capacity_end      (pointer — std::vector-style capacity)
bucket + 0x18: count             (DWORD — cached child count)
bucket + 0x1C: padding           (4 bytes)
```

### 3.3 Group Index (NOT class ID)

`ChildComponent_Enumerate`'s 3rd parameter is a **group index**, NOT a class ID:

```asm
; At 0x1402F9C21 — direct array indexing, no hash:
lea     ecx, [rsi-1]        ; group_index - 1
shl     rcx, 5              ; * 32 bytes per bucket
add     rdi, rcx            ; bucket_ptr = base + (group_index - 1) * 32
```

- `group_index = 0` → scan ALL buckets (no single-bucket optimization)
- `group_index = N` → jump directly to `bucket[N-1]`

**Multiple class IDs share the same group.** Evidence: both `GetClusters_Lua` (filters for class 15) and `GetSectors_Lua` (filters for class 87) pass `group_index = 2`. The group is a coarse partition; callers apply vtable-based post-filters (`IsClassID`) to select the exact class.

Observed group indices across 61 callers: 0 (all), 2 (spatial), 3, 5, 6 (ships/docked), 7-16. At least 16 groups exist.

### 3.4 Secondary Bitmask Filter

A byte-wide bitmask at `byte_146C7D590` provides an 8-bit class filter **within** each bucket:

```asm
mov     ecx, [rdx+68h]    ; child.class_id
shl     al, cl             ; 1 << (class_id % 8) — wraps at 8 bits
test    bl, al             ; test against bitmask
```

This is a secondary optimization, NOT the primary indexing. Since it operates on a single byte (`al`), class IDs >= 8 alias (e.g., class 8 produces the same bit as class 0). In practice, the global appears to be `0xFF` (all-pass), meaning the real filtering is done by callback filter objects (4th parameter) and post-iteration vtable checks.

### 3.5 Iterator State Structure

```
ChildIteratorState (40 bytes):
  +0x00: current_bucket_ptr    (QWORD)
  +0x08: last_bucket_ptr       (QWORD — end of scan range)
  +0x10: child_index            (DWORD — index within current bucket)
  +0x14: single_bucket_flag     (BYTE  — don't advance to next bucket)
  +0x15: class_bitmask          (BYTE  — 1 << class_id filter)
  +0x18: filter_callback_1      (QWORD — filter object pointer)
  +0x20: filter_callback_2      (QWORD — filter object pointer)
```

**Do NOT walk manually.** Group assignment is opaque (determined at child insertion time). Use `ChildComponent_Enumerate` or the safe FFI approach.

| Function | Address | Purpose |
|----------|---------|---------|
| `ChildComponent_Enumerate` | `0x1402F9B80` | Iterate children by group index with optional filters (61 callers) |
| `ChildComponent_Iterator_Init` | `0x1402FF740` | Initialize bucket iterator from container+8 |
| `ChildComponent_Iterator_Next` | `0x1402F9AA0` | Advance to next matching child |
| `ChildComponent_GetBucketCount` | `0x1402E5120` | Read child count (total or per-bucket) |

---

## 4. Main Vtable Slots

The main vtable at +0x00 has ~800+ entries. Key slots (byte offsets):

| Slot | Byte Offset | Function | Confirmed By |
|------|-------------|----------|-------------|
| 17 | +136 | `GetClassType() -> uint` | Multiple functions |
| 566 | +4528 | `GetClassID() -> uint` (120=sentinel) | `GetComponentClass` |
| 567 | +4536 | `IsClassID(classid) -> bool` | `GetStationModules`, `ChildComponent_Enumerate` |
| 568 | +4544 | `IsOrDerivedFromClassID(classid) -> bool` | `GetContextByClass`, `IsComponentClass` |
| 597 | +4776 | `GetIDCode() -> std::string*` (build 606138) | `GetObjectIDCode` |
| 643 | +5144 | `SetWorldTransform(...)` | `Component_ComputeWorldTransform` |
| 648 | +5184 | `SetPosition(transform*)` | `SetObjectSectorPos` |
| 676 | +5408 | `Destroy(reason, flags)` | `RemoveComponent` |
| 701 | +5608 | `GetFactionID() -> int` | `GetAllFactionStations` |
| 749 | +5992 | `IsKnownTo(faction_ctx) -> bool` | `GetClusters_Lua` known filter |

---

## 5. Component Registry

**Global:** `g_ComponentRegistry` at RVA `0x06C866C0`.

```c
// ComponentRegistry_Find @ 0x1400CE890
X4Component* lookup_component(uint64_t id) {
    return ComponentRegistry_Find(g_ComponentRegistry, id, 4);
}
```

### 5.1 UniverseID Decomposition

- Bits 0-24: slot index (1-based)
- Bits 25-40: generation counter

The registry has up to 32 pages, ~1M entries per page, 3 entries packed per 32-byte block. The third parameter to `ComponentRegistry_Find` is a class mask (4 = general component lookup).

SDK helper: `x4n::entity::find_component(id)`.

---

## 6. Entity Hierarchy (Scene Graph)

Every entity has a parent pointer at +0x70. Position is relative to parent.

```
Galaxy
  +-- Cluster (class 15)
        +-- Sector (class 87)
              +-- Zone (class 108) — parent for ships/stations
                    +-- Station (class 97)
                          +-- WalkableModule (class 119)
                                +-- Room (class 83)
                                      +-- Actor/NPC (class 71/75)
                    +-- Ship (class 116 abstract, concrete: 89-94)
```

Navigation: `GetContextByClass(entity, "sector", false)` walks the parent chain via +0x70.

**`Entity_AttachToParent`** at `0x140397C50` — core hierarchy reparenting (26 callers, NOT exported). Steps: check attachability (vtable+4960) → set positional offset (vtable+5184) → execute reparent (vtable+4944) → update visibility + attention level.

---

## 7. Class ID Table

Source: `GetComponentClassMatrix()` runtime dump. 120 entries.

> **Build 603098 change:** `navcontext` (ID 70) was inserted into the class table, shifting all IDs that were ≥ 70 up by 1. `player` (ID 75, previously unnamed in this table) is now confirmed at that slot after the shift. See full list below.

**`ClassName_StringToID`** at `0x1402D4130` — maps class name strings to numeric IDs. BST at `0x1438D2568`. Returns 120 (sentinel) for unknown names.

IDs 0-108: concrete/leaf classes. IDs 109-119: abstract hierarchy classes.

| ID | Name | Notes |
|----|------|-------|
| **15** | **`cluster`** | Galaxy subdivision — parent of sectors |
| 34 | `defensible` | Has hull/shields. Hull: `sub_14011BBF0`. Shield: `sub_1404E0990`. Read by `GetComponentDetails` @ `0x140AB1E80`. |
| **70** | **`navcontext`** | New in build 603098 — navigation context class |
| **71** | **`npc`** | On-foot NPC character (was 70 pre-603098) |
| **72** | **`object`** | Base for all placed 3D entities — required by `GetObjectPositionInSector` (was 71 pre-603098) |
| **75** | **`player`** | Player entity class |
| **76** | **`positional`** | Required by `Get/SetPositionalOffset` (was 75 pre-603098) |
| **83** | **`room`** | Walkable interior room (was 82 pre-603098) |
| 84 | `satellite` | (was 83 pre-603098) |
| **87** | **`sector`** | Target of `GetContextByClass("sector")` (was 86 pre-603098) |
| 89-94 | `ship_xs/s/m/l/xl` | Concrete ship subclasses (were 88-92 pre-603098; xl is now 94) |
| **97** | **`station`** | Station entity (was 96 pre-603098) |
| **108** | **`zone`** | Physics zone — walked by `SetObjectSectorPos` (was 107 pre-603098) |
| **110** | **`container`** | Abstract: stations and ships containing entities (was 109 pre-603098) |
| **111** | **`controllable`** | Abstract: entities accepting orders (was 110 pre-603098) |
| **116** | **`ship`** | Abstract ship class (was 115 pre-603098) |
| **119** | **`walkablemodule`** | Abstract: modules with walkable interiors (was 118 pre-603098) |
| _(120)_ | _(sentinel)_ | Not a class — BST resolver returns this for unknown names (was 119 pre-603098) |

Key class IDs used in SDK code are defined in `x4_manual_types.h` (`X4_CLASS_*` constants). Full table (all 120 entries) is in SUBSYSTEMS.md §13.2.

---

## 8. Player Slot Layout

Per-player data at `qword_143C9FA58 + 560`:

| Offset | Contents | Access |
|--------|----------|--------|
| +0 | Player slot pointer | — |
| +8 | Player actor game_id (uint64) | `GetPlayerID()` |
| +112 | Ptr to current physical entity | `GetPlayerObjectID()`, `GetPlayerContainerID()` |
| +27316 | Ship activity enum | `GetPlayerActivity()` Lua wrapper |
| +29496 | Cached room entity pointer | `GetEnvironmentObject()` (unreliable — often returns 0) |

### 8.1 Key Player Functions

| Function | Address | Method |
|----------|---------|--------|
| `GetPlayerID` | `0x14016b040` | `player_slot[+8]` |
| `GetPlayerObjectID` | `0x14016b400` | Walks `player_slot[+112]` for class 72 |
| `GetPlayerContainerID` | `0x14016ae60` | Class 110 (container) |
| `GetPlayerZoneID` | `0x14016bb40` | Class 108 (zone) |
| `GetPlayerOccupiedShipID` | `0x140abb7b0` | Class 116 (ship) |
| `GetObjectPositionInSector` | `0x1401691c0` | Requires class 72; walks +112 for sector |
| `SetObjectSectorPos` | `0x14017f630` | Requires class 72; walks +112 for zone |
| `GetContextByClass` | `0x1401519e0` | Generic parent-chain walk |

### 8.2 On-Foot Detection

```cpp
bool is_on_foot = (g->GetPlayerOccupiedShipID() == 0) &&
                  (g->GetPlayerContainerID() != 0);
UniverseID avatar = g->GetPlayerObjectID();  // class 72, safe for position read
```

---

## 9. Galaxy Enumeration

`GetClusters` (`0x140264060`) and `GetSectors` (`0x1402642C0`) are **Lua globals only** — NOT C FFI.

### FFI-Safe Approach (SDK)

`x4n::galaxy::rebuild_cache()` enumerates sectors via `GetSectorsByOwner` per faction + `get_component_macro()` (offset-based). ~152 sectors, <1ms.

### Direct Child Walk (Alternative)

1. Galaxy: `GetPlayerGalaxyID()` (C FFI) → `find_component()` → object pointer
2. Clusters: walk `galaxy + 0xA8` children, filter class 15
3. Sectors: walk `cluster + 0xA8` children, filter class 87
4. `GetClusters`/`GetSectors` also apply `isKnownTo` filtering (vtable +5992). Direct walk bypasses this.

**`g_GameUniverse`** at RVA `0x03CAEE68` (build 603098). Safer entry: use `GetPlayerGalaxyID()` C FFI.

---

## 10. Proxy NPC Spawning

`CreateNPCFromPerson` (`0x1401b99e0`) CANNOT be used for arbitrary proxy spawning — requires pre-existing `NPCSeed` in the person list.

**Correct approach:** `SpawnObjectAtPos2(macro, sector, pos, owner_faction)`:
- Works with any character macro from `character_macros.xml`
- Created entity has class 72 — compatible with `SetObjectSectorPos`
- Registered in component system — visible to all entity queries
