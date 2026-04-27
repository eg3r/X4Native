// ---------------------------------------------------------------------------
// x4n_faction.h — Faction Registry & Relation Reads
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::faction::find_class_by_id()  — resolve faction id string to FactionClass*
//   x4n::faction::get_relation()      — read NPC-NPC relation float by id strings
//   x4n::faction::get_relation_ptr()  — same, with cached FactionClass pointers
//
// Backed by the FactionRelation_GetFloat internal FFI (see x4_internal_func_list.inc)
// and the FactionRegistry data global at X4_RVA_FACTION_REGISTRY (manual_types.h).
//
// Matches the engine's inlined FNV-1 + RBTree lookup pattern that appears in
// GetFactionRelationStatus2, SetFactionRelationToPlayerFaction, GetFactionDetails.
// See docs/rev/FACTION_RELATIONS.md §2.2 + §3.1 for the reverse-engineering notes.
//
// Use case: galaxy-wide NPC-NPC relation matrix snapshots (X4Strategos §4.1).
// Cost: ~21 factions × 21 = 441 reads per matrix burst — sub-millisecond.
//
// **Side-effect — cleanup-on-read.** Every relation read may mutate the
// boost map: if a per-pair boost has decayed past its `1e-6` epsilon since
// last access, FactionRelation_GetFloat tail-calls FactionRelation_RemoveBoost
// to garbage-collect the entry. Safe for UI-thread polling (the boost map is
// game-loop-thread-private), but means matrix sweeps continuously prune
// expired boosts as a side-effect of reading. Don't call from a non-UI
// thread; don't expect read-only semantics. Documented at the FFI
// declaration in x4_internal_func_list.inc.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

#include <cstdint>

namespace x4n { namespace faction {

namespace detail {

/// FNV-1 hash of a NUL-terminated faction id, no case normalization.
/// Matches the engine's inlined FNV-1 loop in GetFactionRelationStatus2:
///   v7 = 2166136261; for (each byte b) v7 = b ^ (16777619 * v7);
/// (FNV-1: multiply-then-XOR, NOT FNV-1a's XOR-then-multiply.)
/// Result is a 32-bit hash extended into uint64 with high bits zero —
/// matches the engine's `LL`-typed hash for uint64 RBTree key compares.
inline uint64_t fnv1_id(const char* id) noexcept {
    uint64_t hash = 2166136261ULL;
    for (const char* p = id; *p; ++p) {
        hash = static_cast<uint64_t>(static_cast<uint8_t>(*p)) ^ (16777619ULL * hash);
    }
    return hash;
}

} // namespace detail

/// Resolve a faction id string ("argon", "teladi", "xenon", etc.) to its
/// engine `X4FactionClass*` via the FNV-1 + RBTree lookup matching the
/// engine's inlined pattern. The returned pointer is permanent for the
/// session — safe to cache across calls within one game-load, but re-resolve
/// on save reload since the registry rebuilds.
///
/// Returns nullptr if the id is unknown, the faction registry is not yet
/// initialized, or the offset macros do not match this build.
///
/// @stability MODERATE — depends on X4_RVA_FACTION_REGISTRY + node layout.
/// @verified v9.00 build 606138 (per FACTION_RELATIONS.md §2.2)
inline X4FactionClass* find_class_by_id(const char* faction_id) noexcept {
    if (!faction_id || !faction_id[0]) return nullptr;

    auto* off = ::x4n::detail::offsets();
    if (!off || !off->faction_registry) return nullptr;

    // The data global at X4_RVA_FACTION_REGISTRY is a POINTER SLOT (verified
    // 2026-04-27 via IDA at SetFactionRelationToPlayerFaction RVA 0x0017F8D0:
    //   `mov rdx, cs:qword_146C80848`  — explicit MEM load, not LEA
    //   `add rdx, 10h`                 — registry+16 = sentinel
    //   `mov rax, [rdx+8]`             — registry+24 = root
    // So we deref the slot first (matches X4Native's ComponentRegistry
    // pattern in find_component), then walk the struct fields.
    void* reg_struct = *reinterpret_cast<void**>(off->faction_registry);
    if (!reg_struct) return nullptr;

    auto* reg_bytes = static_cast<uint8_t*>(reg_struct);
    auto* sentinel  = reg_bytes + X4_FACTION_REGISTRY_TREE_BASE;
    void* root      = *reinterpret_cast<void**>(reg_bytes + X4_FACTION_REGISTRY_TREE_ROOT);
    if (!root) return nullptr;

    const uint64_t target = detail::fnv1_id(faction_id);

    // RBTree lower_bound: traverse from root, save candidate when going left
    // (key >= target). Matches engine's inlined search exactly. Key is read
    // as a full uint64 to match the engine's qword compare (verified at
    // SetFactionRelationToPlayerFaction RVA 0x0017F8D0+0x60: `cmp [rax+20h], r11`
    // with r11 holding the zero-extended FNV-1 hash).
    uint8_t* candidate = sentinel;
    auto*    node      = static_cast<uint8_t*>(root);
    while (node) {
        const uint64_t key = *reinterpret_cast<const uint64_t*>(node + X4_FACTION_REGISTRY_NODE_KEY);
        if (key < target) {
            node = *reinterpret_cast<uint8_t**>(node + X4_FACTION_REGISTRY_NODE_RIGHT);
        } else {
            candidate = node;
            node      = *reinterpret_cast<uint8_t**>(node + X4_FACTION_REGISTRY_NODE_LEFT);
        }
    }

    if (candidate == sentinel) return nullptr;

    // Sanity check: lower_bound returns NOT-LESS-THAN — verify exact match.
    // The engine skips this check (relies on hash uniqueness across vanilla
    // faction ids), but we add it cheaply to defend against future collisions.
    const uint64_t cand_key = *reinterpret_cast<const uint64_t*>(candidate + X4_FACTION_REGISTRY_NODE_KEY);
    if (cand_key != target) return nullptr;

    return *reinterpret_cast<X4FactionClass**>(candidate + X4_FACTION_REGISTRY_NODE_VALUE);
}

/// Read the [-1.0, +1.0] internal relation float from faction `a_id` toward
/// `b_id`. Returns 0.0f if either id is unknown, the FFI is unresolved, or
/// no boost/base entry exists (true neutral). Returns ~+1.0f when a_id == b_id
/// (engine's same-faction self-reference constant).
///
/// Uses FactionRelation_GetFloat under the hood. Side-effect: cleanup-on-read
/// of expired boosts (see x4_internal_func_list.inc). Safe for matrix sweeps.
///
/// @stability MODERATE — depends on FactionRelation_GetFloat resolved RVA.
/// @verified v9.00 build 606138 (per FACTION_RELATIONS.md §2.2)
inline float get_relation(const char* a_id, const char* b_id) noexcept {
    auto* g = game();
    if (!g || !g->FactionRelation_GetFloat) return 0.0f;
    X4FactionClass* a = find_class_by_id(a_id);
    X4FactionClass* b = find_class_by_id(b_id);
    if (!a || !b) return 0.0f;
    // FFI is declared with void* (see x4_internal_func_list.inc); cast at the
    // type-safe wrapper boundary.
    return g->FactionRelation_GetFloat(static_cast<void*>(a), static_cast<void*>(b));
}

/// Variant for callers that already hold FactionClass pointers (e.g.,
/// galaxy-wide matrix sweeps cache the pointer per faction once and reuse it
/// for the inner loop, avoiding N×N redundant find_class_by_id calls).
inline float get_relation_ptr(X4FactionClass* a, X4FactionClass* b) noexcept {
    auto* g = game();
    if (!g || !g->FactionRelation_GetFloat || !a || !b) return 0.0f;
    return g->FactionRelation_GetFloat(static_cast<void*>(a), static_cast<void*>(b));
}

}} // namespace x4n::faction
