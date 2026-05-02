# X4Native — Internal Subsystems

## Event System

The event system is a **general-purpose bidirectional event bus**. Any event name can flow in any direction — Lua→C++, C++→Lua, or C++→C++. The bus is implemented as a pub/sub system in the core DLL (`EventSystem`), with two bridges in the proxy that connect it to X4's Lua/MD layer.

```mermaid
---
config:
    layout: 'elk'
    theme: 'neutral'
    themeVariables:
        lineColor: '#ff9100'
---
graph LR
    subgraph "MD / Lua"
        RLE["raise_lua_event"]
        RE["RegisterEvent\nCallEventScripts"]
    end

    subgraph "Core DLL"
        EB["EventSystem::fire\n(name, data)"]
        IN["Lua Bridge\ninbound"]
        OUT["Lua Bridge\noutbound"]
    end

    subgraph "Extension DLLs"
        CB["callbacks"]
        RAI["raise_event()"]
    end

    RLE -->|"Lua→C++"| IN --> EB
    EB --> OUT -->|"C++→Lua"| RE
    EB -->|"dispatch"| CB
    RAI -->|"fire"| EB
```

**Key principle:** The event bus doesn't know or care which events exist. Extensions subscribe to any name. The Lua bridge is a general transport.

### Built-in Events

| Event | Source | Direction |
|-------|--------|-----------|
| `on_game_loaded` | MD cue (`player.age ≥ 3s`) → Lua → DLL | Lua→C++ |
| `on_game_started` | MD `event_game_started` → Lua → DLL | Lua→C++ |
| `on_game_save` | MD `event_game_saved` → Lua → DLL | Lua→C++ |
| `on_ui_reload` | Lua bootstrap on re-execution | Lua→C++ |
| `on_before_reload` | Core `impl_prepare_reload()` | C++ internal |
| `on_frame_update` | Lua `SetScript("onUpdate")` → DLL | Lua→C++ |

### C++→Lua Bridge

The proxy calls X4's `CallEventScripts` directly — the same dispatch path the game engine uses for `<raise_lua_event>` from MD. Lua listeners can't tell whether an event came from MD or C++.

```
api->raise_lua_event("mymod.alert", "data")
    → proxy_raise_lua_event():
        getfield(GLOBALSINDEX, "CallEventScripts")
        pushstring(name), pushstring(param)
        pcall(2, 0, 0)
    → all RegisterEvent("mymod.alert", fn) listeners fire
```

### Lua→C++ Bridge

Table-driven forwarding in `x4native.lua`. Extensions can dynamically register bridges at init time via `x4n::bridge_lua_event(lua_event)` — the C++ event name defaults to the Lua event name. Pass an explicit second arg only when the two should differ.

### Event Naming Conventions

| Scope | Pattern | Example |
|-------|---------|---------|
| C++ bus | `on_<name>` | `on_game_loaded`, `on_game_started` |
| Framework Lua | `x4native.<name>` | `x4native.game_loaded`, `x4native.game_started` |
| Extension Lua | `<modname>.<name>` | `tradealert.price_changed` |

### Thread Safety

The C++ event bus uses `std::mutex` with snapshot-copy-before-dispatch. Lua is single-threaded (UI thread). `raise_lua_event` must only be called from the UI thread — all current events satisfy this.

### MD Event Dispatch (Direct Interception)

In addition to the string-named event bus, X4Native hooks the game's internal MD event dispatch function (`EventQueue_InsertOrDispatch`) to provide **direct, typed subscriptions** to all 551 MD event types — no MD cue XML or Lua bridge needed.

#### Architecture

```mermaid
---
config:
    layout: 'elk'
    theme: 'neutral'
    themeVariables:
        lineColor: '#ff9100'
---
graph TB
    subgraph "Game Engine"
        GE["Game logic raises event"]
        EQ["EventQueue_InsertOrDispatch"]
        MD["MD cue listeners"]
    end

    subgraph "X4Native Hook"
        BEF["md_fire_before(type_id)"]
        AFT["md_fire_after(type_id)"]
        TR["trampoline<T>\n(extract typed struct)"]
    end

    subgraph "Extension"
        CB["on_killed_after(callback)"]
    end

    GE --> EQ
    EQ --> BEF --> MD --> AFT
    BEF --> TR --> CB
    AFT --> TR --> CB
```

#### Dispatch Flow

1. Game calls `EventQueue_InsertOrDispatch(source, event_object, timestamp, immediate)`
2. X4Native hook extracts `type_id` from the event object's vtable (`vtable[1]` = `mov eax, IMM; ret`)
3. Constructs `X4MdEvent` payload: `{type_id, source_id, timestamp, raw_event}`
4. Fires all `before` subscribers for that `type_id` via `EventSystem::md_fire_before()`
5. Calls the original function (game's MD cue dispatch happens here)
6. Fires all `after` subscribers via `EventSystem::md_fire_after()`

#### Storage

Subscriptions use O(1) array lookup — two fixed arrays of 600 slots indexed by `type_id`:

```cpp
static std::array<std::vector<Subscription>, 600> s_md_before;
static std::array<std::vector<Subscription>, 600> s_md_after;
```

#### Type Safety via Trampoline

The `detail::trampoline<T>` template bridges the raw `X4MdEvent*` to a typed struct:

```
raw callback(name, data, userdata)
    → cast data to X4MdEvent*
    → call T::from(ev)  (extracts fields at known offsets)
    → call user callback with typed T&
```

This is zero-copy — fields are extracted on-the-fly into a stack-allocated struct.

#### Code Generation

The 261 typed event structs and `on_*_before/after()` functions in `sdk/x4_md_events.h` are auto-generated by `scripts/generate_event_type_ids.ps1` from three sources:

| Source | Location | What it provides |
|--------|----------|-----------------|
| `event_type_ids.csv` | `reference/` | Type ID, RTTI class name, vtable RVA |
| `event_layouts.csv` | `reference/` | Field offsets and sizes per event type |
| `common.xsd` | `reference/game/libraries/` | Field names and documentation from game schema |

The `event_probe` example extension discovers `event_layouts.csv` at runtime by scanning builder function machine code.

### Subscriptions Across Save Loads

Subscription IDs are **not stable** across save loads. Extensions re-subscribe during `x4native_init()` which runs on each save load cycle.

---

## Function Hooking

### Design

Inspired by Harmony's patching model (.NET), adapted for native x64:

| Concept | X4Native | Notes |
|---------|----------|-------|
| Prefix | `hook_before` | Runs before original. Can modify args, skip original. |
| Postfix | `hook_after` | Runs after original. Can read/modify return value. |
| Priority | Extension load order | `priority` in `x4native.json` (lower = first) |
| Multiple patches | Callback chain | One MinHook detour per target, dispatches to all callbacks |

### Execution Flow

```
GetComponentName() called by game code
    ↓
[MinHook trampoline → x4native dispatcher]
    ↓
result_buf = {0}    (zero-initialized)
skip = false
    ↓
BEFORE callbacks (all run, load-order):
    → ExtA::my_before(ctx)     // priority 50 → first
    → ExtB::my_before(ctx)     // priority 100 → second
    ↓
if (!skip):
    result_buf = original(args)
    ↓
AFTER callbacks (all run, load-order):
    → ExtA::my_after(ctx)
    → ExtB::my_after(ctx)
    ↓
return result_buf
```

**Chain rules:**

- All before-hooks always run — `skip_original` does not short-circuit the chain
- `skip_original` is an OR-gate — any before-hook can set it
- Result is last-writer-wins
- Original overwrites result (if it runs)
- After-hooks always run regardless of whether original was skipped
- Args are modifiable — changes propagate to later hooks and the original

### Why Exported Functions Are Stable

Hook targets use `GetProcAddress` — the same lookup as the Game API resolver. As long as the export name exists, **hooks survive game patches** without offset databases or signature scanning.

### SEH Wrapping

Each callback is wrapped in `__try/__except`. A crashing callback is auto-disabled for the session. Other callbacks and the original continue normally. SEH has zero overhead on x64 on the happy path (table-based exception model).

### MinHook

[MinHook](https://github.com/TsudaKageyu/minhook) (v1.3.4, BSD-2-Clause): pure C, ~15 KB, zero dependencies, integrated via CMake FetchContent. Suspends all threads during hook install/remove for thread safety.

### Hook Lifecycle

- **Hot-reload:** All hooks removed before core unload (`MH_Uninitialize`), re-registered by extensions on re-init
- **Save load:** Extensions shut down (hooks cleared) → re-discovered → re-init → hooks re-registered
- **Auto-cleanup:** All hooks for an extension are removed when that extension unloads

---

## Extension Discovery & Loading

### Why x4native.json (Not content.xml)

X4's `content.xml` has a fixed schema enforced by the game engine. Adding unknown elements risks rejection or breakage on game updates. `x4native.json` is invisible to the game engine and fully under our control.

### Discovery Flow

```mermaid
---
config:
    layout: 'elk'
    theme: 'neutral'
    themeVariables:
        lineColor: '#ff9100'
---
graph TB
    SCAN["Scan extensions/ folder"] --> CHECK["x4native.json exists?"]
    CHECK -->|Yes| PARSE["Parse config"]
    CHECK -->|No| SCAN
    PARSE --> ENABLED["IsExtensionEnabled?"]
    ENABLED -->|Yes| LOAD["LoadLibrary(dll)"]
    ENABLED -->|No| SKIP["Skip (info log)"]
    LOAD -->|Success| INIT["x4native_init(api)"]
    LOAD -->|Failure| FAIL["Log error, skip"]
    INIT -->|Success| DONE["Extension Active"]
    INIT -->|Exception| FAIL
```

The core scans every subdirectory of `extensions/` for `x4native.json`, checks if the extension is enabled in X4's built-in extension manager, validates the config, loads the DLL, and calls `x4native_init(api)` with the API struct.

### Extension Lifecycle

| Phase | When | What happens |
|-------|------|--------------|
| Discovery | Game start | Core scans `extensions/*/x4native.json` |
| Init | Game start | `x4native_init(api)` — register events, hooks. Can read stash. No game calls yet. |
| Game Loaded | Savegame loaded | `on_game_loaded` fires. Game functions safe. Gamestart MD cues may not have run yet. |
| Game Started | After gamestart cues | `on_game_started` fires. All gamestart MD cues complete (known flags, factions, etc.). |
| Runtime | Every frame | `on_frame_update`, `on_game_save`, custom events. Stash read/write anytime. |
| Shutdown | Game exit / save load | `x4native_shutdown()` — cleanup. Write state to stash here. |
| Unload | After shutdown | `FreeLibrary`, all hooks and subscriptions auto-cleared. Stash persists in proxy. |

---

## Error Handling & Crash Protection

### Extension Isolation

Every extension callback is wrapped in SEH:

```cpp
__try {
    ext.callback(event_name, data);
} __except(EXCEPTION_EXECUTE_HANDLER) {
    Logger::error("Extension '{}' crashed during '{}' — disabling.", ...);
    ext.enabled = false;
}
```

### What Can Be Caught

| Failure Type | Catchable? | Recovery? |
|---|---|---|
| Access violation (null deref) | SEH | Disable extension |
| Stack overflow | SEH | Risky |
| Division by zero | SEH | Disable extension |
| C++ exception (throw) | try/catch | Disable extension |
| Heap corruption | No | Eventual crash |
| Bad hook (wrong signature) | No | Eventual crash |
| Deadlock | No | Game freezes |

### Logger

Two sinks, no external dependencies:

- **File sink:** two files — framework log at `<mod_root>/x4native.log`, and one per-extension log at `<ext_folder>/<name>.log` (or `"logfile"` field from `x4native.json`). Thread-safe via `std::mutex`, flush on `Info+` (survives crashes).
- **Debug sink:** `OutputDebugStringA` (visible in debugger / DebugView)

Format uses `std::format` (C++23) with timestamps.

---

## Stash (In-Memory Key-Value Store)

The stash is a namespace-scoped key-value store that lives in the proxy DLL, making it the only extension-accessible state that survives `/reloadui` and extension hot-reload.

### Why the Proxy Owns It

The proxy is the only DLL guaranteed to stay loaded for the entire process lifetime (pinned via `GET_MODULE_HANDLE_EX_FLAG_PIN`). The core DLL is hot-reloaded, and extension DLLs are `FreeLibrary`'d on every reload cycle. Any state in those DLLs is destroyed. The proxy is the natural — and only safe — home.

### Data Flow

```
Extension             Core DLL            Proxy DLL
  api->stash_set() →   (pass-through) →   g_stash[ns][key] = blob
  api->stash_get() →   (pass-through) →   return g_stash[ns][key].data()
```

Function pointers flow: proxy → `CoreInitContext` → core saves them → `ExtensionManager::fill_api()` writes them into each extension's `X4NativeAPI` struct. Extensions call them directly — no dispatch table indirection.

### Implementation

```cpp
// In proxy.cpp (static, process-lifetime)
std::unordered_map<std::string,
    std::unordered_map<std::string, std::vector<uint8_t>>> g_stash;
std::mutex g_stash_mutex;
```

- **Namespace**: typically the extension name (auto-set by `x4n::stash::` helpers via `_reserved[0]`)
- **Key**: arbitrary string
- **Value**: opaque byte blob (`vector<uint8_t>`), copied on `set`, pointer returned on `get`
- **Thread safety**: all operations hold `g_stash_mutex`
- **Pointer stability**: `get()` returns a pointer into the vector's buffer, valid until the next `set` or `remove` on the same key

### Lifecycle

| Event | Stash behavior |
|-------|---------------|
| Game start | Empty |
| Extension init | Read previous state (if any) |
| Runtime | Read/write freely |
| Extension shutdown | Write state to preserve it |
| `/reloadui` | **Persists** — proxy stays loaded |
| Extension hot-reload | **Persists** — same mechanism |
| Save-load | **Persists** — proxy is pinned |
| Game exit | **Lost** — process terminates |
