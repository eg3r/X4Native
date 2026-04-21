# Extension Developer Guide

Build native C++ extensions for X4: Foundations using the X4Native framework.

## Prerequisites

- Visual Studio 2022 Build Tools (MSVC, C++23)
- CMake 3.20+
- X4Native installed in the game's `extensions/x4native/` folder

## Project Structure

An extension is a standard X4 extension folder with a native DLL:

```
extensions/x4native_mymod/
  content.xml         ← X4 extension manifest
  x4native.json       ← X4Native config (name, DLL path, priority)
  native/
    x4native_mymod.dll
```

### content.xml

Standard X4 extension descriptor. Must declare `x4native` as a dependency. The `id` attribute is the **canonical unique identifier** — X4Native keys all routing (logs, stash, scoped events, user settings) on it.

```xml
<?xml version="1.0" encoding="utf-8"?>
<content version="900" id="x4native_mymod" name="My Mod"
         description="My native extension." author="you"
         date="2026-01-01" save="0" enabled="1">
  <dependency id="x4native" optional="false" />
  <text language="44" name="My Mod" description="My native extension." />
</content>
```

- `save="0"` — extension doesn't affect save compatibility (use `1` only if your mod adds persistent data that breaks saves without it)
- `<text language="44" ...>` — optional localization override (language 44 = English); omit if not needed
- `version` in the root `<content>` element reflects the **game version** this was built for (900 = 9.00)

### x4native.json

Tells the framework where to find your DLL and how to load it:

```json
{
    "library": "native\\x4native_mymod.dll",
    "priority": 100,
    "min_api_version": 1
}
```

| Field | Required | Description |
|-------|----------|-------------|
| `library` | Yes | Relative path to your DLL |
| `priority` | No | Load order (lower = earlier, default 0) |
| `min_api_version` | No | Minimum framework API version required |
| `autoreload` | No | `true` → watch DLL for changes and hot-reload in-place (default `false`) |
| `logfile` | No | Log filename inside `<profile>\x4native\` (default `<extension_id>.log`). Must be a relative name. |
| `settings` | No | Array of user-configurable settings surfaced in **Settings → Extensions → `…`** and persisted per-user. See [Settings](#settings). |

> `name` was required in earlier versions and is now ignored — extension identity and the log prefix are read from `content.xml`. Leaving it in place produces a one-line deprecation warning.

## Minimal Extension

```cpp
#include <x4native.h>

X4N_EXTENSION {
    x4n::log::info("Hello from my extension!");

    x4n::on("on_game_loaded", [] {
        x4n::log::info("Game world is ready!");
    });
}

X4N_SHUTDOWN {
    x4n::log::info("Goodbye!");
}
```

`X4N_EXTENSION` runs when the framework loads your DLL. `X4N_SHUTDOWN` runs on unload. The framework auto-cleans event subscriptions and hooks on shutdown, but explicit cleanup is good practice.

### Selective Includes

`x4native.h` is a convenience umbrella that pulls in the entire SDK. For faster compiles or minimal extensions, include only what you need:

```cpp
#include <x4n_core.h>        // x4n::game(), exe_base(), X4N_EXTENSION/X4N_SHUTDOWN macros
#include <x4n_events.h>      // x4n::on/off/raise/bridge_lua_event
#include <x4_md_events.h>   // x4n::md::on_*_before/after (typed MD event subscriptions)
#include <x4n_log.h>         // x4n::log::info/warn/error/debug
#include <x4n_stash.h>       // x4n::stash::set/get (survives /reloadui)
#include <x4n_hooks.h>       // x4n::hook::before/after/remove
#include <x4n_settings.h>    // x4n::settings::get_*/set_*, on_setting_changed

// Game domain helpers:
#include <x4n_entity.h>      // x4n::entity::find_component
#include <x4n_math.h>        // x4n::math::RAD_TO_DEG, advance_seed, fnv1a_lower
#include <x4n_memory.h>      // x4n::memory::game_alloc<T>, game_alloc_array<T> (SMem pool)
#include <x4n_rooms.h>       // x4n::rooms::roomtype_name
#include <x4n_plans.h>       // x4n::plans::resolve_macro, plan_registry, plan_set_entries
#include <x4n_visibility.h>  // x4n::visibility::get_radar_visible, is_map_visible, ...
```

Every sub-header is self-contained (includes `x4n_core.h` automatically). An events-only extension:

```cpp
#include <x4n_core.h>
#include <x4n_events.h>

X4N_EXTENSION {
    x4n::on("on_game_loaded", [] { /* ... */ });
}
```

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(x4native_mymod)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(x4native_mymod SHARED mymod.cpp)

target_include_directories(x4native_mymod PRIVATE
    path/to/x4native/sdk
)

set_target_properties(x4native_mymod PROPERTIES
    PREFIX ""
    OUTPUT_NAME "x4native_mymod"
)
```

Build: `cmake -B build -G "Visual Studio 17 2022" -A x64 && cmake --build build --config Debug`

## API Reference

All API functions live in the `x4n::` namespace. Include `<x4native.h>` for everything, or individual `<x4n_*.h>` headers for selective access.

### Lifecycle Events

| Event | When |
|-------|------|
| `on_game_loaded` | Game world is initialized and safe to call game functions. Gamestart MD cues may not have run yet. |
| `on_game_started` | World fully initialized, all gamestart MD cues complete. Use when you need gamestart effects (e.g., known sectors, faction setup). |
| `on_game_save` | Game is being saved |
| `on_ui_reload` | Lua state rebuilt (after `/reloadui`) |
| `on_frame_update` | Every UI frame tick |

### Events

```cpp
// Subscribe (returns subscription ID)
int id = x4n::on("on_game_loaded", [] { /* no params */ });
int id = x4n::on("event_name", [](const char* p) { /* string param from Lua */ });
int id = x4n::on("event_name", [](void* p) { /* raw pointer from C++ raise */ });

// Unsubscribe
x4n::off(id);

// Raise a C++ event (other extensions can subscribe to it)
x4n::raise("my_custom_event");
x4n::raise("my_custom_event", my_data_ptr);  // subscribers get void*

// Raise a Lua event (C++ → Lua)
x4n::raise_lua("my_lua_event_name", "optional_param");
```

### Lua Event Bridges

Forward game Lua events into C++ without writing any Lua:

```cpp
// Register bridge (call during on_game_loaded or on_game_started)
x4n::bridge_lua_event("playerUndocked", "on_player_undocked");

// Subscribe — string param from Lua is forwarded automatically
x4n::on("on_player_undocked", [](const char* param) {
    x4n::log::info("Player undocked (param: {})", param ? param : "none");
});

// Or subscribe without params if you don't need them
x4n::on("on_player_undocked", [] { x4n::log::info("A ship undocked!"); });
```

### Custom MD Event Forwarding

Some game events (e.g. `event_object_destroyed`, `event_player_money_updated`) only exist in the Mission Director system and have no Lua equivalent. To use them in C++, your extension needs two pieces:

**1. MD cue** — add an XML file in your extension's `md/` folder (the game auto-discovers all `*.xml` files there; no registration in `content.xml` needed):

```xml
<!-- md/my_events.xml -->
<mdscript name="MyModEvents"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xsi:noNamespaceSchemaLocation="md.xsd">
  <cues>
    <cue name="mymod_ObjectDestroyed" instantiate="true">
      <conditions>
        <event_object_destroyed />
      </conditions>
      <actions>
        <raise_lua_event name="'mymod.object_destroyed'" />
      </actions>
    </cue>
  </cues>
</mdscript>
```

**2. Bridge to C++** in your extension code:

```cpp
X4N_EXTENSION {
    x4n::on("on_game_loaded", [] {
        x4n::bridge_lua_event("mymod.object_destroyed", "on_object_destroyed");
    });

    x4n::on("on_object_destroyed", [](const char* param) {
        x4n::log::info("Destroyed: {}", param ? param : "unknown");
    });
}
```

The full flow is: **Game Engine → MD cue → `raise_lua_event` → Lua bridge → C++ event**. This is the same pattern X4Native itself uses for `on_game_loaded`, `on_game_started`, and `on_game_saved`.

### Direct MD Event Subscriptions

X4Native hooks the game's internal event dispatch (`EventQueue_InsertOrDispatch`) and provides **typed, zero-bridge subscriptions** to all 551 MD event types. No MD cue XML, no Lua bridge, no string matching — just a direct C++ callback with a typed struct.

Include `<x4_md_events.h>` (auto-generated, included by `<x4native.h>`):

```cpp
#include <x4_md_events.h>
```

Every MD event type has a data struct and a pair of subscription functions:

```cpp
// Subscribe to events — returns subscription ID
int id = x4n::md::on_killed_after([](const x4n::md::KilledData& e) {
    x4n::log::info("Entity {} destroyed by {}", e.source_id, e.killer);
});

int id2 = x4n::md::on_faction_relation_changed_after(
    [](const x4n::md::FactionRelationChangedData& e) {
        x4n::log::info("Faction relation changed at t={:.1f}", e.timestamp);
    });

// Unsubscribe (same as named events)
x4n::off(id);
```

#### Before vs After

Each event has two subscription points:

| Variant | When it fires | Use case |
|---------|---------------|----------|
| `on_{event}_before` | Before the game processes the event (MD cues haven't fired yet) | Observe or log pre-dispatch state |
| `on_{event}_after` | After the game has dispatched to all MD cue listeners | React to fully-processed events |

Most extensions should use the `_after` variant.

#### Data Structs

Every struct contains at minimum:

| Field | Type | Description |
|-------|------|-------------|
| `source_id` | `uint64_t` | Universe ID of the entity that raised the event |
| `timestamp` | `double` | Game time when the event occurred |

Additional fields are event-specific. Examples:

```cpp
// KilledData (type_id 233) — entity destroyed
struct KilledData {
    uint64_t source_id;          // destroyed entity
    double   timestamp;
    uint64_t killer;             // killer entity ID
    uint64_t kill_method;        // kill method enum
    uint64_t was_parent_killed;  // parent also destroyed?
};

// FactionRelationChangedData (type_id 184) — faction relations shifted
struct FactionRelationChangedData {
    uint64_t source_id;
    double   timestamp;
    uint64_t faction1;           // faction descriptor pointer
    uint64_t faction2;           // other faction descriptor pointer
    uint64_t new_relation;       // new relation value (reinterpret as float)
    uint32_t old_relation;       // old relation value (reinterpret as float)
    uint32_t reason;             // relation change reason
};

// SectorChangedOwnerData (type_id 409) — sector ownership changed
struct SectorChangedOwnerData {
    uint64_t source_id;
    double   timestamp;
    uint64_t sector_changing_ownership;
    uint64_t new_owner_faction;
    uint64_t previous_owner_faction;
};
```

Browse `sdk/x4_md_events.h` for all 261 typed event definitions.

#### Practical Example

```cpp
#include <x4_md_events.h>

static int g_sub_killed = 0;
static int g_sub_owner  = 0;

X4N_EXTENSION {
    x4n::on("on_game_loaded", [] {
        g_sub_killed = x4n::md::on_killed_after(
            [](const x4n::md::KilledData& e) {
                x4n::log::info("Destroyed: entity {} by {}", e.source_id, e.killer);
            });

        g_sub_owner = x4n::md::on_sector_changed_owner_after(
            [](const x4n::md::SectorChangedOwnerData& e) {
                x4n::log::info("Sector {} changed owner", e.sector_changing_ownership);
            });
    });
}

X4N_SHUTDOWN {
    x4n::off(g_sub_killed);
    x4n::off(g_sub_owner);
}
```

#### Direct MD Events vs MD Bridge — When to Use Which

| | Direct MD (`x4n::md::on_*`) | MD Bridge (`raise_lua_event`) |
|---|---|---|
| **Setup** | One C++ call, no XML/Lua needed | Requires MD cue XML + Lua bridge |
| **Type safety** | Typed struct with named fields | String parameter only |
| **Performance** | Minimal — direct hook, no scripting layers | Multiple scripting layer crossings |
| **Version sensitivity** | Event struct layouts may shift between game patches | Stable (uses MD scripting layer) |
| **Available events** | All 551 internal MD event types | Only events with MD cue conditions |
| **Best for** | High-frequency events, structured data, performance-critical paths | Simple notifications, forward-compatible mods |

### Logging

Each extension writes to its own log file at `<X4 profile>\x4native\<extension_id>\<extension_id>.log`. Rotated on each load — keeps `.1`–`.4` backups. No setup required; the framework opens the file before your `X4N_EXTENSION` body runs. All writes by an extension stay inside its own subfolder — absolute paths and `..` segments passed to `set_log_file` / `to_file` are rejected with a warning.

The API is **sink-first, level-second**, using `std::format` (`{}` placeholders, not `%s`):

```cpp
#include <x4n_log.h>

// Default sink: your extension's own log.
x4n::log::info("loaded v1.0");                // plain string
x4n::log::info("count = {}", count);          // formatted
x4n::log::warn("odd: {:.3f}", value);
x4n::log::error("failed: {}", error_code);
x4n::log::debug("value = {}", 42);

// Sink: framework log (<profile>\x4native\x4native.log).
x4n::log::global.info("framework-level note");

// Sink: one-shot write to a named file, scoped to this extension:
//   <profile>\x4native\<extension_id>\events.log
x4n::log::to_file("events.log").info("event: {}", id);

// Redirect this extension's default log file.
x4n::log::set_log_file("mymod_v2.log");
```

`logfile` in `x4native.json` sets an alternative default filename inside your extension's subfolder (`<profile>\x4native\<extension_id>\`). Must be a relative name with no `..` segments:

```json
{
    "library": "native\\x4native_mymod.dll",
    "logfile": "mymod_v2.log"
}
```

> **Migration from the old API** (removed, not deprecated):
> | Old | New |
> |---|---|
> | `log::info("msg %s", str)` | `log::info("msg {}", str)` |
> | `log::info("msg", false)` | `log::global.info("msg")` |
> | `log::info("msg", "extra.log")` | `log::file("extra.log").info("msg")` |
> | `log::info("msg", true)` | `log::info("msg")` *(default is already the ext log)* |
>
> The old printf-style variadic had a silent-footgun: `log::info("path: %s", some_path.c_str())` resolved to the filename overload, creating a file named after the runtime value of `some_path`. The new API has no such ambiguity — filenames are always the first positional arg of a named `file()` sink. Scoped enums must be cast to an underlying integer before formatting (`std::format` requires this).

### Game Functions

2,051 typed game functions resolved from `X4.exe` exports:

```cpp
// Cache the table pointer during init
static X4GameFunctions* game = nullptr;

X4N_EXTENSION {
    game = x4n::game();

    // Register lifecycle listeners — functions are called when the event fires.
    x4n::on("on_game_loaded", on_game_loaded);
    x4n::on("on_game_started", on_game_started);
}

// Called when save data is loaded. Game functions safe, but gamestart MD cues NOT yet fired.
void on_game_loaded() {
    UniverseID player = game->GetPlayerID();
    const char* name = game->GetComponentName(player);
    bool docked = game->IsPlayerValid();
}

// Called when world is fully initialized (all gamestart MD cues complete).
// Use this when you need known sectors, faction setup, scripted entity placement, etc.
void on_game_started() {
    // All set_known flags, faction setup, etc. are now visible.
}
```

Browse `sdk/x4_game_func_table.h` for the full function list.

### Function Hooks

Intercept game function calls with before/after hooks:

```cpp
// Before-hook: args by reference, can modify or skip the original
static void before_fn(x4n::hook::HookControl& ctl,
                      UniverseID& id, uint32_t& count) {
    x4n::log::info("intercepted call with id={}", id);
    // ctl.skip_original = 1;  // skip the real function
}

// After-hook: args by value (read-only)
static void after_fn(UniverseID id, uint32_t count) {
    x4n::log::info("call completed");
}

// Install (returns hook ID, 0 on failure)
int h1 = x4n::hook::before<&X4GameFunctions::SomeFunction>(before_fn);
int h2 = x4n::hook::after<&X4GameFunctions::SomeFunction>(after_fn);

// Remove
x4n::hook::remove(h1);
x4n::hook::remove(h2);
```

Install hooks in `on_game_loaded`. The framework auto-removes hooks when your extension unloads.

### Info

```cpp
x4n::game_version();  // "9.00"
x4n::version();       // "0.9.0 (game: 9.00)"
x4n::path();          // "G:\...\extensions\x4native_mymod\"
```
### Game Domain Helpers

Higher-level helpers organized into domain namespaces. Include individual headers or `<x4native.h>` for all:

```cpp
// Entity resolution (x4n_entity.h)
void* obj = x4n::entity::find_component(entity_id);

// Game math (x4n_math.h)
uint64_t next = x4n::math::advance_seed(seed);
float deg = radians * x4n::math::RAD_TO_DEG;
uint64_t hash = x4n::math::fnv1a_lower("some_macro_name");

// Walkable interiors (x4n_rooms.h)
const char* name = x4n::rooms::roomtype_name(X4_ROOMTYPE_BAR);  // "bar"

// Game memory allocation (x4n_memory.h) — for objects the game will free
auto* entry = x4n::memory::game_alloc<X4PlanEntry>();
auto* arr = x4n::memory::game_alloc_array<X4PlanEntry*>(count);

// Construction plans (x4n_plans.h)
void* macro = x4n::plans::resolve_macro("station_gen_hab_t1_01_macro");
void* conn = x4n::plans::resolve_connection(macro, "connection_room01");
void* registry = x4n::plans::plan_registry();

// Visibility (x4n_visibility.h)
bool radar = x4n::visibility::get_radar_visible(entity_id);
bool on_map = x4n::visibility::is_map_visible(entity_id);
x4n::visibility::set_radar_visible(entity_id, false);  // direct byte write

// Radar change event — standard event system, fires on enter/leave radar range
int h = x4n::on("on_radar_changed", [](const X4RadarChangedEvent* e) {
    x4n::log::info("radar: {} -> {}", e->entity_id, e->visible);
});
x4n::off(h);
```

### Stash (Reload-Safe In-Memory Storage)

The framework provides an in-memory key-value store called **stash** that lives in the proxy DLL. It survives `/reloadui` and extension hot-reload, but is lost on game exit.

Use stash to preserve state across reloads without disk I/O:

```cpp
// Store/retrieve simple values (auto-namespaced to your extension)
x4n::stash::set("counter", 42);
int val = 0;
if (x4n::stash::get("counter", &val))
    x4n::log::info("Restored counter: {}", val);

// Strings
x4n::stash::set_string("name", "my_extension");
const char* s = x4n::stash::get_string("name");  // returns nullptr if missing

// Raw blobs
struct MyState { int hp; float x, y, z; };
MyState state = {100, 1.0f, 2.0f, 3.0f};
x4n::stash::set("state", &state, sizeof(state));

uint32_t size = 0;
auto* p = x4n::stash::get("state", &size);  // returns nullptr if missing
if (p && size == sizeof(MyState))
    memcpy(&state, p, sizeof(state));

// Remove a key or clear all your keys
x4n::stash::remove("counter");
x4n::stash::clear();  // only clears YOUR extension's keys
```

**Typed helpers** (`set<T>` / `get<T>`) require trivially-copyable types and perform a strict size check — if the stored blob size doesn't match `sizeof(T)`, `get<T>` returns `false`. This protects against reading stale data after struct layout changes during development.

| Property | Value |
|----------|-------|
| Lifetime | Game session (lost on game exit) |
| Survives | `/reloadui`, extension hot-reload, save-load |
| Namespace | Auto-scoped to extension name |
| Thread safety | Mutex-protected in proxy |
| Max size | Process memory (no artificial limit) |
## Settings

Extensions can declare **user-configurable settings** that appear in the vanilla X4 menu at **Settings → Extensions → select your extension → `…`**, directly under the built-in **Enabled** toggle. Values are persisted per-user at
`<X4 profile>/x4native/<extension_id>/settings.user.json`
(e.g. `Documents\Egosoft\X4\<account>\x4native\x4native_mymod\settings.user.json`),
not in your extension folder — they survive extension updates, Steam Workshop re-syncs, and Windows user switching.

### Declaring settings

Add a `"settings"` array to `x4native.json`. Three widget types are supported:

```json
{
    "name": "mymod",
    "library": "native\\x4native_mymod.dll",
    "settings": [
        { "id": "verbose",  "name": "Verbose logging",
          "type": "toggle", "default": false },

        { "id": "poll_s",   "name": "Poll interval (s)",
          "type": "slider", "min": 1, "max": 30, "step": 1, "default": 5 },

        { "id": "provider", "name": "LLM Provider",
          "type": "dropdown", "default": "anthropic",
          "options": [
            { "id": "anthropic", "text": "Anthropic" },
            { "id": "openai",    "text": "OpenAI"    }
          ]
        }
    ]
}
```

| Field | Required | Notes |
|-------|----------|-------|
| `id` | Yes | Stable key, used both for persistence and in the C++ API. `[a-z0-9_]` recommended. |
| `name` | No | Label shown in the UI. Defaults to `id`. |
| `type` | Yes | One of `"toggle"`, `"slider"`, `"dropdown"`. |
| `default` | No | Default value (matched to the type). Used until the player changes it. |
| `min` / `max` / `step` | For `slider` | Inclusive bounds and step size. Slider values are clamped to `[min, max]` on write. |
| `options` | For `dropdown` | Array of `{ "id", "text" }`. Writes with a value not in this list are rejected. |

### Reading & writing from C++

```cpp
#include <x4n_settings.h>

X4N_EXTENSION {
    bool        verbose  = x4n::settings::get_bool  ("verbose", false);
    double      poll_s   = x4n::settings::get_number("poll_s",  5.0);
    const char* provider = x4n::settings::get_string("provider", "anthropic");

    // Programmatic write — also updates the UI and persists to disk.
    x4n::settings::set_bool("verbose", true);

    // Subscribe to changes from UI or code.
    x4n::on_setting_changed([](const X4NativeSettingChanged& info) {
        if (std::string_view(info.extension_id) != "x4native_mymod") return;
        switch (info.type) {
            case X4N_SETTING_TOGGLE:   /* info.b */ break;
            case X4N_SETTING_SLIDER:   /* info.d */ break;
            case X4N_SETTING_DROPDOWN: /* info.s */ break;
        }
    });
}
```

Values are read lazily — the first `get_*` call for an extension loads the user JSON (if any). Each `set_*` writes atomically (`.tmp` + rename) and fires `on_setting_changed` for every subscriber.

### Constraints

- **Protected UI Mode must be disabled** (same prerequisite as X4Native itself). Without it, the `debug` library is unavailable and the Lua injector can't wire up the rows. The DLL-side read/write still works regardless.
- A setting whose `id` changes between releases is a new setting — old values in the user's JSON for the old id are ignored (not migrated).
- `toggle` / `slider` / `dropdown` are the only types currently. Text input can be added; open an issue if you need it.

## Hot-Reload Workflow

Extension DLLs use the same copy-on-load pattern as the framework core: the original DLL is never locked by the game process. This means you can **rebuild your extension while the game is running** and have it reload automatically.

### Manual reload
Trigger `/reloadui` at any time — the framework unloads all extensions, copies the new binaries, and reinitializes everything. Stash data persists across this cycle, so extensions can restore state without disk I/O.

### Automatic per-extension reload
Set `"autoreload": true` in your `x4native.json`. The framework polls your DLL's modification time every ~2 seconds (120 frames). When a change is detected, it hot-reloads **only your extension** without disturbing others or triggering a full UI reload:

1. Build your DLL (original is never file-locked, overwrite freely)
2. The framework detects the new mtime within ~2 seconds
3. Your extension is shut down, the new binary is loaded, and `X4N_EXTENSION` runs again

Each extension controls its own autoreload independently via its `x4native.json`.

> **Never ship `"autoreload": true` in a release build.** The framework always tracks DLL mtimes when the flag is set — polling every frame, comparing file timestamps, and unloading/reloading the DLL mid-session. This adds overhead and is a development-only tool. Your released `x4native.json` must have `"autoreload": false` or omit the field entirely.

## Important Notes

- **Never call game functions before `on_game_loaded`** — the game world isn't ready yet.
- **All code runs on the UI thread** — no threading required, no thread safety concerns.
- **Extensions are auto-cleaned on unload** — hooks and event subscriptions are removed automatically, but explicit cleanup in `X4N_SHUTDOWN` is recommended.
- **Static variables are lost on reload** — Use `x4n::stash` (fast, in-memory, survives reload) or disk files (survives game exit) to preserve state.
- **Protected UI mode must be OFF** in game settings for native extensions to work.
- **Game updates may break hooks** — function signatures or addresses can change between patches.

## Examples

See the `examples/` folder for working extensions:

| Example | Demonstrates |
|---------|-------------|
| `hello` | Lifecycle events, game function calls, logging |
| `event_test` | Event round-trips (C++→Lua→C++), frame updates, direct MD event subscriptions |
| `hook_test` | Before/after hooks on a game function |
| `lua_bridge` | Dynamic Lua→C++ event forwarding |
