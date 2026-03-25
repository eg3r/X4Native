// ---------------------------------------------------------------------------
// x4n_plans.h — Construction Plan Helpers
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::plans::resolve_macro()       — macro name to MacroData* lookup
//   x4n::plans::resolve_connection()  — connection name to ConnectionEntry* lookup
//   x4n::plans::plan_registry()       — construction plan DB pointer
//   x4n::plans::game_alloc<T>()       — forwarded from x4n::memory (see x4n_memory.h)
//   x4n::plans::game_alloc_array<T>() — forwarded from x4n::memory
//   x4n::plans::plan_set_entries()    — wire entries into an editable plan
//
// All functions require on_game_loaded to have fired.
// See docs/rev/CONSTRUCTION_PLANS.md for full system documentation.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_math.h"
#include "x4n_memory.h"

namespace x4n { namespace plans {

/// Compute FNV-1a hash of a lowercased string (engine convention).
/// Forwarded from x4n::math:: for internal use.
using math::fnv1a_lower;

/// Resolve a connection name string to its ConnectionEntry pointer within a macro.
/// macro_ptr must come from resolve_macro(). Returns nullptr if not found.
/// Uses FNV-1a hash + binary search on the macro's sorted connection array.
/// See docs/rev/CONSTRUCTION_PLANS.md for ConnectionEntry layout.
/// @stability HIGH — depends on MacroData offsets (X4_MACRODATA_OFFSET_CONNECTIONS_*).
/// @verified v9.00 build 600626
inline void* resolve_connection(void* macro_ptr, const char* connection_name) {
    if (!macro_ptr || !connection_name || !connection_name[0]) return nullptr;

    uint64_t hash = fnv1a_lower(connection_name);
    if (hash == 2166136261ULL) return nullptr;  // empty string hash = seed -> invalid

    auto addr = reinterpret_cast<uintptr_t>(macro_ptr);
    auto begin = *reinterpret_cast<uintptr_t*>(addr + X4_MACRODATA_OFFSET_CONNECTIONS_BEGIN);
    auto end   = *reinterpret_cast<uintptr_t*>(addr + X4_MACRODATA_OFFSET_CONNECTIONS_END);
    if (!begin || begin >= end) return nullptr;

    // Binary search: entries sorted by hash at entry+8, stride 352 bytes
    // Note: engine stores hash as uint64 in the comparison despite FNV-1a producing 32-bit;
    // the upper 32 bits are the XOR overflow from 64-bit arithmetic.
    size_t count = (end - begin) / X4_CONNECTION_ENTRY_SIZE;
    size_t lo = 0, hi = count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        auto entry_addr = begin + mid * X4_CONNECTION_ENTRY_SIZE;
        auto entry_hash = *reinterpret_cast<uint64_t*>(entry_addr + X4_CONNECTION_OFFSET_HASH);
        if (entry_hash < hash)
            lo = mid + 1;
        else if (entry_hash > hash)
            hi = mid;
        else
            return reinterpret_cast<void*>(entry_addr);
    }
    return nullptr;
}

/// Resolve a macro name string to its internal MacroData pointer.
/// Returns nullptr if macro not found. silent=true suppresses game error logs.
/// Lowercases the name before lookup (engine convention).
/// Caches the registry pointer on first call. Only call after on_game_loaded.
/// @stability MODERATE — depends on X4_RVA_MACRO_REGISTRY + MacroRegistry_Lookup.
/// @verified v9.00 build 600626
inline void* resolve_macro(const char* macro_name, bool silent = true) {
    auto* g = game();
    if (!g || !g->MacroRegistry_Lookup) return nullptr;
    static uintptr_t s_macro_reg = 0;
    if (!s_macro_reg)
        s_macro_reg = *reinterpret_cast<uintptr_t*>(exe_base() + X4_RVA_MACRO_REGISTRY);
    if (!s_macro_reg) return nullptr;

    // Lowercase (engine uses lowercased FNV-1a keys)
    char lower[256];
    size_t len = 0;
    for (const char* p = macro_name; *p && len < sizeof(lower) - 1; p++, len++)
        lower[len] = (*p >= 'A' && *p <= 'Z') ? (*p + 32) : *p;
    lower[len] = 0;

    struct { const char* data; size_t length; } sv{ lower, len };
    return g->MacroRegistry_Lookup(reinterpret_cast<void*>(s_macro_reg), &sv, silent ? 1 : 0);
}

/// Get the construction plan registry pointer.
/// Caches on first call. Only call after on_game_loaded.
/// @stability MODERATE — depends on X4_RVA_CONSTRUCTION_PLAN_DB.
/// @verified v9.00 build 600626
inline void* plan_registry() {
    static void* s_plan_reg = nullptr;
    if (!s_plan_reg)
        s_plan_reg = *reinterpret_cast<void**>(exe_base() + X4_RVA_CONSTRUCTION_PLAN_DB);
    return s_plan_reg;
}

// game_alloc / game_alloc_array forwarded from x4n::memory (see x4n_memory.h).
using memory::game_alloc;
using memory::game_alloc_array;

/// Set the entry vector on an EditableConstructionPlan.
/// Copies entry pointers into a game-allocated array and wires up the
/// plan's internal vector (plan+184/192/200). See docs/rev/CONSTRUCTION_PLANS.md.
/// @stability HIGH — depends on plan struct offsets (+184/192/200).
/// @verified v9.00 build 600626
inline bool plan_set_entries(void* plan, X4PlanEntry** entries, size_t count) {
    if (!plan || !count) return false;

    auto* arr = game_alloc_array<X4PlanEntry*>(count);
    if (!arr) return false;

    for (size_t i = 0; i < count; i++)
        arr[i] = entries[i];

    auto addr = reinterpret_cast<uintptr_t>(plan);
    *reinterpret_cast<X4PlanEntry***>(addr + 184) = arr;
    *reinterpret_cast<X4PlanEntry***>(addr + 192) = arr + count;
    *reinterpret_cast<X4PlanEntry***>(addr + 200) = arr + count;
    return true;
}

}} // namespace x4n::plans
