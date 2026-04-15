# Extension API Design — Future Rework Plan

## Current Architecture (v1.0)

Extensions interact with X4Native through two layers:

1. **`game->Function()`** — direct calls to X4.exe's 2000+ functions via a pointer table
2. **`x4n::domain::function()`** — SDK inline functions that hide plumbing

Under the hood, the `x4n::` functions use three different mechanisms:
- **Inline memory reads** via `X4GameOffsets` — hot-path data (vtable slots, struct offsets, pre-resolved RVAs)
- **Game function calls** via `X4GameFunctions*` — X4.exe exported/internal functions
- **Core DLL calls** via `X4NativeAPI` function pointers — framework services (subscribe, log, hook)

The `X4NativeAPI` struct uses `_reserved` slots for ABI stability. Extensions compiled against older headers continue working because field offsets don't shift.

## Problem

Three mechanisms behind one namespace is a premature optimization. The inline path saves ~5ns per call vs a function pointer call, but the game functions they wrap cost ~50ns+. The savings are <10% of an already-cheap operation.

The mixed design also means:
- Three failure modes, three debugging paths
- Some functions survive hot-reload cleanly (function pointers), others have stale static caches (inline reads)
- `_reserved` slots are a finite resource — eventually run out

## Proposed Rework: Minimal Frozen Struct + `get_interface()`

Freeze `X4NativeAPI` to two fields:

```c
typedef struct X4NativeAPI {
    int api_version;                        // offset 0 — frozen forever
    const void* (*get_interface)(int id);   // offset 8 — frozen forever
} X4NativeAPI;
```

All capabilities become side-tables retrieved by enum ID:

```c
typedef enum X4InterfaceID {
    X4_INTERFACE_CORE    = 0,   // subscribe, log, hook, etc.
    X4_INTERFACE_GAME    = 1,   // X4GameFunctions* (2000+ game functions)
    X4_INTERFACE_OFFSETS = 2,   // X4GameOffsets* (hot-path data)
    X4_INTERFACE_SDK     = 3,   // X4NativeSdk* (SDK functions as DLL calls)
} X4InterfaceID;
```

Core implementation is an array lookup:

```cpp
static const void* s_interfaces[16] = {};
const void* get_interface(int id) {
    return (id >= 0 && id < 16) ? s_interfaces[id] : nullptr;
}
```

### Extension init caches side-table pointers

The `X4N_EXTENSION` macro caches everything at init — zero per-call overhead:

```cpp
namespace x4n::detail {
    inline X4NativeAPI*        g_api     = nullptr;
    inline const X4CoreApi*    g_core    = nullptr;  // subscribe, log, hook
    inline X4GameFunctions*    g_game    = nullptr;  // game functions
    inline const X4GameOffsets* g_offsets = nullptr;  // hot-path offsets
    inline const X4NativeSdk*  g_sdk     = nullptr;  // SDK functions
}

// In X4N_EXTENSION macro:
g_api     = _x4n_api;
g_core    = static_cast<...>(_x4n_api->get_interface(X4_INTERFACE_CORE));
g_game    = static_cast<...>(_x4n_api->get_interface(X4_INTERFACE_GAME));
g_offsets = static_cast<...>(_x4n_api->get_interface(X4_INTERFACE_OFFSETS));
g_sdk     = static_cast<...>(_x4n_api->get_interface(X4_INTERFACE_SDK));
```

### SDK functions become thin wrappers

```cpp
// x4n_entity.h — extension developer calls this (unchanged)
inline X4Component* find_component(uint64_t id) {
    return static_cast<X4Component*>(detail::g_sdk->find_component(id));
}
```

### Hot-path functions stay inline where it matters

```cpp
// x4n_entity.h — vtable call, inlined for tight loops
inline bool is_a(const X4Component* comp, GameClass cls) {
    if (!comp || !comp->vtable) return false;
    using Fn = bool(__fastcall*)(const void*, uint32_t);
    auto fn = reinterpret_cast<Fn*>(comp->vtable)[detail::g_offsets->vtable_is_derived_class];
    return fn ? fn(comp, static_cast<uint32_t>(cls)) : false;
}
```

## Benefits

- **`X4NativeAPI` frozen forever** — no `_reserved`, no ABI versioning
- **Extensions never recompile** for any reason (new game version, new X4Native features)
- **One extensibility mechanism** — new side-tables via new enum values
- **Side-tables grow independently** — append-only, no size constraints
- **Clean separation**: hot-path inlines (`offsets`) vs normal DLL calls (`sdk`) is explicit, not mixed

## Migration Path

1. Add `get_interface()` to current `X4NativeAPI` (consumes one `_reserved` slot)
2. Create `X4NativeSdk` side-table with SDK functions as DLL calls
3. Move `x4n::` inline functions from direct memory access to `g_sdk->` calls (except hot-path)
4. Move core function pointers (subscribe, log, hook) to `X4CoreApi` side-table
5. Once all access goes through `get_interface()`, freeze `X4NativeAPI` to `{api_version, get_interface}`
6. Remove `_reserved`

Steps 1-2 can be done incrementally alongside other work. Steps 3-6 are the breaking change (requires all extensions to recompile once, then never again).
