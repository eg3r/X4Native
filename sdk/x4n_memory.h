// ---------------------------------------------------------------------------
// x4n_memory.h — Game Memory Allocation (SMem Pool)
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Provides:
//   x4n::memory::game_alloc<T>()       — allocate single game object
//   x4n::memory::game_alloc_array<T>() — allocate typed array
//
// X4 uses a custom thread-local pool allocator (SMem). Any object that the
// game engine will later free MUST be allocated through GameAlloc, not
// standard new/malloc. See docs/rev/MEMORY.md for full documentation.
//
// All functions require on_game_loaded to have fired.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

namespace x4n { namespace memory {

/// Allocate a single game object via SMem pool. Returns typed pointer.
/// Use for objects that the game will later free (plan entries, events, etc.).
/// See docs/rev/MEMORY.md.
/// @stability STABLE — uses GameAlloc C FFI function.
template<typename T>
T* game_alloc() {
    auto* g = game();
    if (!g || !g->GameAlloc) return nullptr;
    return static_cast<T*>(g->GameAlloc(sizeof(T), 0, 0, 0, 16));
}

/// Allocate a typed array via SMem pool. Returns pointer to first element.
/// @stability STABLE — uses GameAlloc C FFI function.
template<typename T>
T* game_alloc_array(size_t count) {
    auto* g = game();
    if (!g || !g->GameAlloc) return nullptr;
    return static_cast<T*>(g->GameAlloc(count * sizeof(T), 0, 0, 0, 16));
}

}} // namespace x4n::memory
