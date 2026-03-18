# X4Native — Project Guidelines

## Overview

X4Native is a native C++ extension framework for **X4: Foundations** (Windows x64). It loads into the game process via LuaJIT's `package.loadlib()` and provides an SDK for other modders to write native extension DLLs.

## Architecture

Two-DLL design: a thin **proxy** (`x4native_64.dll`) that is LoadLibrary-locked by LuaJIT, and a **core** (`x4native_core.dll`) that is copy-on-load hot-reloadable. The proxy owns the stable Lua-facing API table; the core owns all subsystems (logger, event bus, extension manager, hook manager).

The game bridge is: **MD XML cues → Lua events → DLL entry points**. Game lifecycle events (game_loaded, game_saved) flow from Mission Director scripts through `raise_lua_event` into the DLL layer.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for full design and sequence diagrams.

## Code Style

- C++23, MSVC only (Visual Studio 2022 BuildTools)
- No exceptions in exported functions — use return codes and SEH where needed
- C-compatible ABI at all DLL boundaries (`extern "C"`, no STL in signatures)
- Shared types between proxy and core live in `src/common/`
- Use Logger class (`src/core/logger.h`) for all logging, never raw printf/OutputDebugString
- Keep the proxy DLL minimal (~200 lines) — complexity belongs in core

## Build and Test

```powershell
# CMake is bundled with VS2022 BuildTools (not on PATH by default)
$env:PATH = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;" + $env:PATH

# Configure + build
cmake --preset default
cmake --build build --config Debug

# Output: native/x4native_64.dll + native/x4native_core.dll
```

CMake auto-detects the X4 game directory via the Steam registry. Override with `-DX4_GAME_DIR=<path>`.

**Deploy to game** (symlink or copy):
```powershell
cmake --build build --target deploy
```

## Project Structure

```
src/
  common/       — Shared headers (x4native_defs.h, lua_api.h/cpp)
  proxy/        — Proxy DLL (proxy.cpp) — thin, rarely changes
  core/         — Core DLL (core.cpp, logger, event_system, extension_manager,
                  hook_manager, game_api, version)
sdk/            — Public SDK headers
  x4native.h            — Umbrella header (C++ helpers, hook wrappers)
  x4native_extension.h  — C ABI contract (init, shutdown, hooks)
  x4_game_types.h       — Auto-generated game structs
  x4_game_func_table.h  — Function pointer table struct
  x4_game_func_list.inc — X-macro function list
  x4_manual_types.h     — Hand-authored types (RE / community)
native/         — Built DLL output (gitignored except version_db/)
extension/      — X4 extension payload (deployed to extensions/x4native/)
  content.xml   — X4 extension manifest
  ui.xml        — Lua file registration
  md/           — Mission Director XML cues
  ui/           — Lua bootstrap (x4native.lua)
  t/            — Translations
examples/       — Example extensions (hello, event_test, hook_test, lua_bridge)
scripts/        — Tooling (update_references.ps1 + extract/generate scripts)
docs/           — Consolidated docs (ARCHITECTURE, INTERNALS, GAME_API,
                  SDK_CONTRACT, RELEASE, EXTENSION_GUIDE)
reference/      — Extracted game data for research
  game/         — Lua/XML/XSD from cat/dat archives
  x4_exports.txt      — PE export table from X4.exe
  x4_ffi_raw.txt      — Raw FFI cdef declarations
  x4_struct_names.txt  — Unique struct type names
  x4_ffi_summary.txt   — Cross-reference statistics
```

## Dependencies

- **nlohmann/json v3.11.3** — Config parsing (FetchContent)
- **MinHook v1.3.3** — Function hooking (FetchContent, static lib)
- **lua51_64.dll** — LuaJIT 2.1.0-beta3, resolved at runtime via GetProcAddress (ships with game, NOT linked at build time)
- **Windows version.lib** — PE version info fallback

## Game Reference Files

`reference/game/` contains text files extracted from X4's cat/dat archives. This is committed to git so we can diff between game versions.

### What's in there

| Directory | Contents |
|-----------|----------|
| `md/` | Mission Director cue scripts — game logic, event triggers, AI orders |
| `aiscripts/` | AI behavior trees (XML) |
| `libraries/` | Game data definitions — wares, ships, weapons, factions, jobs |
| `ui/` | Lua UI scripts and XSD schemas — the entire UI API surface |
| `index/` | File index manifests |
| `extensions/` | DLC-specific overrides (ego_dlc_boron, ego_dlc_split, etc.) |
| `VERSION` | Plain-text game build number (e.g. `900`) |

### Updating after a game patch

```powershell
.\scripts\update_references.ps1        # Runs full pipeline (game files + exports + FFI)
git diff reference/                     # Review what changed
git add reference/ ; git commit -m "reference: update to v<version>"
```

Skip flags: `-SkipGameFiles`, `-SkipExports`, `-SkipFFI` for partial updates.

### Using reference files for research

When investigating game APIs, events, or behavior:

- **MD cue patterns**: Search `reference/game/md/` for `<cue>`, `<event_...>`, `raise_lua_event` to understand game event flow
- **Lua UI API**: Search `reference/game/ui/` for function signatures, `RegisterEvent`, FFI `cdef` blocks, and `C.*` calls
- **Library data**: Search `reference/game/libraries/` for game object definitions (wares, ships, weapons, factions)
- **AI scripts**: Search `reference/game/aiscripts/` for AI order implementations and available actions
- **DLC additions**: Each `reference/game/extensions/<dlc>/` mirrors the base structure with DLC-specific overrides

## Key Technical Facts

- X4's Lua runtime is **LuaJIT 2.1.0-beta3** shipping as `lua51_64.dll` in the game root. `X4.exe` exports zero Lua symbols.
- Game version lives in `<game_dir>/version.dat` as plain text (e.g. `900` = v9.00).
- `sn_mod_support_apis` (by bvbohnen) is the reference implementation for DLL loading in X4 — study its patterns when in doubt.
- Protected UI mode must be **OFF** for `package.loadlib()` to work.
- Lua files run before the game world initializes — never call game functions until `on_game_loaded` fires.

## Conventions

- Extension SDK uses a C ABI with `X4NATIVE_EXPORT` macro — see `sdk/x4native_extension.h`
- Event names use `on_` prefix: `on_game_loaded`, `on_game_saved`, `on_frame_update`
- All DLL exports use `core_init`/`core_shutdown` (core) or `luaopen_x4native` (proxy) naming
- Config files use JSON (`x4native.json` per extension) with flat `"library"` key for the DLL path
- Framework log: `<ext_root>/x4native.log` (truncated each run, keeps .1–.4 backups). Each extension also gets its own log at `<ext_folder>/<name>.log` (or `"logfile"` field from `x4native.json`)
