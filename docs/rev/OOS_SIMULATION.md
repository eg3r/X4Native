# X4 Attention Level System (OOS Simulation) -- Reverse Engineering Notes

> **Binary:** X4.exe v9.00 (build 900) -- **Date:** 2026-03
>
> All addresses are absolute (imagebase `0x140000000`). Subtract imagebase to get RVA.

---

## 1. Summary

X4 does NOT use an "out-of-sector" (OOS) flag. Instead, it uses a hierarchical **attention level** system that assigns each entity a numeric attention level based on its spatial relationship to the player. This level determines simulation fidelity, AI behavior complexity, physics simulation, and entity lifecycle.

The attention level is a single `int32` stored at **offset +492 (0x1EC)** in each entity's component structure. It is computed by the engine based on the player's current position in the universe hierarchy, NOT by distance measurement.

---

## 2. Attention Level Enum

Defined in `common.xsd` (`attentionexprlookup`), ordered from lowest to highest fidelity:

| Value | Name | Meaning |
|-------|------|---------|
| 0 | `unknown` | Entity exists but has no spatial relationship to player |
| 1 | `ingalaxy` | Same galaxy as player (always true in single-galaxy X4) |
| 2 | `incluster` | Same cluster as player |
| 3 | `sectorvisible` | Sector is visible from player's sector (adjacent) |
| 4 | `insector` | Same sector as player |
| 5 | `adjacentzone` | In an adjacent zone to the player's zone |
| 6 | `visible` | Potentially visible to the player (within render range) |
| 7 | `inzone` | Same zone as the player |
| 8 | `nearby` | Close to the player within the zone |
| 9 | `adjacentroom` | In a room adjacent to the player's room (on-foot) |
| 10 | `inroom` | Same room as the player (on-foot) |

String table for these values starts at `0x142999520` in the binary (sequential: unknown, ingalaxy, incluster, sectorvisible, insector, adjacentzone, visible/inzone/nearby/adjacentroom/inroom).

### Key Thresholds

- **Level 5 (`adjacentzone`)** -- boundary between "simplified" and "zone-loaded" simulation
- **Level 6 (`visible`)** -- minimum for most AI actions (game enforces: "minimum attention level is 'visible'")
- **Level 7 (`inzone`)** -- full 3D simulation, physics active
- **Level 8 (`nearby`)** -- highest non-room attention, triggers prop/detail generation
- **Level 9 (`adjacentroom`)** -- room system activation (on-foot)

---

## 3. Core Functions

### SetEntityAttentionLevel @ `0x140396460`

**Signature:** `char __fastcall SetEntityAttentionLevel(Entity* entity, uint32_t new_level)`

The central function that changes an entity's attention level. 24+ callers.

**Behavior:**
1. If `entity->attention_level` (offset +123*4 = +492) already equals `new_level`, checks if visual state changed; if not, returns early
2. Updates `entity->attention_level` to `new_level`
3. Sets dirty flags at offsets +71*4, +103*4, +121*4 (OR with 0x40)
4. Calls vtable methods to update visual/simulation state
5. Allocates and fires `U::ChangedAttentionEvent` (vtable at `0x142b36a90`) with old and new levels
6. If crossing the **level 6 boundary** (old < 6 vs new >= 6 or vice versa), additionally fires `U::ChangedAdjacentZoneAttentionEvent`

### PropagateAttentionToChildren @ `0x140396680`

**Signature:** `void __fastcall PropagateAttentionToChildren(Entity* entity, int old_level, char arg3, uint32_t new_level, char arg5)`

Iterates child entities in two containers (offsets +168 and +160, likely "owns" relationships) and sets each child's attention to `new_level`. Children that return true from vtable+5568 (special flag, likely "has own attention") are set to level 1 (ingalaxy) instead of inheriting the parent's level.

### SetEntityVisibilityFlags @ `0x140395F80`

**Signature:** `void __fastcall SetEntityVisibilityFlags(Entity* entity, int attention_type, uint32_t* flags, int class_id, double timeout)`

Manages a per-entity visibility flag table at offset +216 (144-byte structure, allocated on demand). When `attention_type == 7` (inzone), writes visibility flags at offset +480. Tracks pending visibility state changes with timestamps.

### Transition Handlers (Virtual Functions)

These are called via vtable during attention transitions and handle the cascade effects:

| Function | Address | Size | Purpose |
|----------|---------|------|---------|
| `ZoneAttentionTransitionHandler` | `0x140712AE0` | 0x527 | Handles zone-level transitions (levels 7-8). Manages physics shape loading, prop generation, interior creation. |
| `SectorAttentionTransitionHandler` | `0x1405BFD40` | 0xB11 | Handles sector-level transitions (levels 4-8). Manages zone loading/unloading, adjacent zone scanning. |
| `ClusterAttentionTransitionHandler` | `0x140800620` | 0x5B9 | Handles cluster-level transitions. Manages cross-sector entity demotion to `ingalaxy`. |

---

## 4. How Attention Is Determined

Attention level is NOT computed by distance. It is determined by the entity's position in the **universe hierarchy** relative to the player:

```
Player is in: Galaxy > Cluster > Sector > Zone > [Room]
                                                    |
Entity in same room .......... attention = 10 (inroom)
Entity in adjacent room ....... attention = 9 (adjacentroom)
Entity nearby in zone ......... attention = 8 (nearby)
Entity in same zone ........... attention = 7 (inzone)
Entity visible from zone ...... attention = 6 (visible)
Entity in adjacent zone ....... attention = 5 (adjacentzone)
Entity in same sector ......... attention = 4 (insector)
Entity in visible sector ...... attention = 3 (sectorvisible)
Entity in same cluster ........ attention = 2 (incluster)
Entity in same galaxy ......... attention = 1 (ingalaxy)
Entity unknown ................ attention = 0 (unknown)
```

The attention level cascades through the hierarchy:
- When the player **enters a sector**, all entities in that sector get promoted to at least level 4
- When the player **enters a zone**, entities in that zone get promoted to level 7+
- When the player **leaves a zone/sector**, entities get demoted back (often to level 1, `ingalaxy`)
- Child entities inherit parent's attention level via `PropagateAttentionToChildren`

---

## 5. What Changes at Each Attention Level

### Below `visible` (< 6) -- "Low Attention" / OOS

- **AI:** Simplified behavior. AI scripts have `<attention min="unknown">` blocks that define reduced-fidelity behavior. Most attack/combat scripts switch from full 3D maneuvers to stat-based resolution.
- **Physics:** No 3D physics simulation. Entities have logical positions only.
- **Rendering:** No 3D models loaded or rendered.
- **Entity lifecycle:** "Temporary" entities (NPCs, mass traffic) may be despawned. Game comments: "Set actor to not-temporary so that it will not automatically be destroyed in low attention."
- **Combat:** Uses `lowattentioncombat` and `lowattentionattack` systems (string refs at `0x142999478`, `0x1429c2388`) -- stat-based damage resolution.
- **Economy:** Runs via background simulation (not the "god system" -- X4 doesn't use that term internally).

### `visible` (6) -- Render Range

- AI scripts with `<attention min="visible">` activate (most combat, movement scripts)
- 3D models loaded and potentially rendered
- Entities become valid targets for player interaction

### `inzone` (7) -- Full 3D Simulation

- Full physics active (Jolt Physics `JPH::`)
- All AI behaviors active
- `ZoneAttentionTransitionHandler` fires `ReplacePhysicsShapesEvent` to load detailed collision
- Prop generation triggered via `GeneratePropsEvent`

### `nearby` (8) -- Close Proximity

- Detail generation for rooms/interiors
- `SectorAttentionTransitionHandler` calls zone loading functions when entities enter this level
- Adjacent zone scanning activated

### `adjacentroom`/`inroom` (9-10) -- On-Foot

- Room interiors fully loaded
- NPC body/animation active
- Interior detail objects spawned

---

## 6. Events

| Event RTTI | Condition |
|------------|-----------|
| `U::ChangedAttentionEvent` (vtable `0x142b36a90`) | Any attention level change. Payload: old_level (offset +24), new_level (offset +28) |
| `U::ChangedAdjacentZoneAttentionEvent` (vtable at related address) | Crossing the level-6 boundary (adjacent zone visibility change) |

MD scripts listen for these via `event_object_changed_attention`. The event `param` is the new level, `param2` is the old level.

---

## 7. Implications for X4Online

### The Problem

When the host player is in Sector A, all entities in Sector B have attention level <= 3 (`sectorvisible` or lower). If Client B is in Sector B, they see:
- No 3D physics on ships (stat-based movement only)
- Simplified AI behavior (no combat maneuvers, basic pathfinding)
- No rendered models on the host side (client has its own rendering, but host simulation drives state)
- Temporary entities may be despawned

### Can We Fake Player Presence?

**No built-in API exists.** There is no `SetAttentionLevel`, `ForceAttention`, or "virtual player" API exposed to scripts or Lua. The attention system is driven entirely by the player's position in the hierarchy.

**Three potential approaches:**

#### Approach A: Hook `SetEntityAttentionLevel` (Recommended)

Hook `SetEntityAttentionLevel` at `0x140396460` to prevent demotion of entities in sectors where clients are present:

```cpp
// Before hook on SetEntityAttentionLevel
bool before_SetAttention(Entity* entity, uint32_t* new_level) {
    if (*new_level < ATTENTION_INSECTOR) {
        UniverseID sector = GetContextByClass(entity_id, "sector", true);
        if (sector_has_connected_client(sector)) {
            *new_level = ATTENTION_INSECTOR;  // Keep at sector level minimum
            // Or: *new_level = ATTENTION_INZONE for full sim
        }
    }
    return true; // continue to original
}
```

**Pros:** Clean, interceptable, doesn't break the engine's expectations.
**Cons:** Entity at `insector` (4) still lacks full physics. Would need `inzone` (7) for full simulation, which requires zone loading.

**Risk:** Moderate. Keeping entities at `insector` is safe -- the engine handles this naturally for the player's sector. Forcing `inzone` without the player's zone being loaded could cause issues.

#### Approach B: Teleport an Invisible Entity

Create or move an invisible entity (e.g., a tiny probe) to each client's sector. The engine would naturally promote nearby entities.

**Pros:** Works with the engine's natural attention system.
**Cons:** An entity doesn't promote a whole sector -- only entities in the same zone. Would need zone-level presence.

#### Approach C: Accept OOS Degradation

Accept that entities in non-player sectors have lower simulation fidelity. The client renders based on replicated position data anyway. The host sends position updates, and the client uses those positions regardless of the host's simulation fidelity.

**Pros:** Zero risk, simplest approach.
**Cons:** Ships in client's sector may move unrealistically (e.g., no collision avoidance, simplified flight paths). Combat resolution would be stat-based rather than projectile-based.

### Recommended Strategy

**Start with Approach C** (accept degradation), then **add Approach A** (hook-based attention floor) if players report visible quality issues.

The key insight is that **replication already decouples host simulation from client rendering**. The client places entities at replicated positions regardless of the host's attention level. The main impact is on AI behavior quality and combat accuracy, which may be acceptable for initial multiplayer.

If Approach A is implemented, the recommended attention floor is **level 4 (`insector`)** for any sector with a connected client. This keeps AI scripts that require `insector` running without the overhead of full physics simulation. Level 7 (`inzone`) should only be forced if combat fidelity is critical.

---

## 8. Key Addresses

| Name | Address | Purpose |
|------|---------|---------|
| `SetEntityAttentionLevel` | `0x140396460` | Core attention setter (24+ callers) |
| `PropagateAttentionToChildren` | `0x140396680` | Cascades attention to owned entities |
| `SetEntityVisibilityFlags` | `0x140395F80` | Per-entity visibility flag management |
| `ZoneAttentionTransitionHandler` | `0x140712AE0` | Zone-level attention transitions |
| `SectorAttentionTransitionHandler` | `0x1405BFD40` | Sector-level attention transitions |
| `ClusterAttentionTransitionHandler` | `0x140800620` | Cluster-level attention transitions |
| `ChangedAttentionEvent vtable` | `0x142b36a90` | RTTI: `U::ChangedAttentionEvent` |
| `Attention string table` | `0x142999520+` | "unknown", "ingalaxy", ..., "inroom" |
| Attention field offset | Entity + 492 (0x1EC) | `int32` attention level |

---

## 9. AI Script Attention Behavior

AI scripts define different behavior at different attention levels via `<attention min="...">` blocks:

```xml
<!-- Example from fight.attack.object.fighter.xml -->
<attention min="visible">
  <!-- Full 3D combat: target tracking, weapon firing, evasive maneuvers -->
</attention>
<attention min="unknown">
  <!-- Stat-based: simplified damage, no projectiles, basic positioning -->
</attention>
```

Common patterns:
- **Combat scripts** have `visible` + `unknown` blocks (full vs stat-based)
- **Movement scripts** have `visible` + `unknown` blocks (3D flight vs simplified)
- **Economy/logistics** scripts often only have `unknown` (run at any attention)
- **Interior/NPC** scripts require `visible` or higher
- **Mass traffic** scripts have `visible` + `unknown` blocks

---

## 10. Related Documents

| Document | Contents |
|----------|----------|
| [GAME_LOOP.md](GAME_LOOP.md) | Frame tick, subsystem updates |
| [SUBSYSTEMS.md](SUBSYSTEMS.md) | Universe hierarchy, zone system |
| [THREADING.md](THREADING.md) | Main-thread-only constraint |
| [STATE_MUTATION.md](STATE_MUTATION.md) | Safe mutation patterns |
