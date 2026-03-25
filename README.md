# X4Native

![C++23](https://img.shields.io/badge/C%2B%2B-23-blue?style=flat-square&logo=cplusplus)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey?style=flat-square&logo=windows)
![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)
![CI](https://github.com/eg3r/X4Native/actions/workflows/ci.yml/badge.svg)
![Release](https://github.com/eg3r/X4Native/actions/workflows/release.yml/badge.svg)

A native C++ extension framework for **X4: Foundations**.

X4Native lets mod authors go beyond what Lua and Mission Director XML can do alone. It loads into the game process and enables mods written in C++ — with direct access to game functions, event handling, and more. If a mod you use requires X4Native, you just install it as a dependency.

---

## For Players

### Installation

1. Download the latest release
2. Extract into your X4 `extensions/` folder (e.g. `X4 Foundations/extensions/x4native/`)
3. Disable **Protected UI mode** in game settings
4. Launch the game — check `extensions/x4native/x4native.log` to verify the framework loaded; each extension writes its own log to its extension folder

Mods that depend on X4Native will list it as a requirement. Install them into `extensions/` alongside `x4native/` — the framework discovers and loads them automatically.

### Requirements

- X4: Foundations (v9.x, Windows x64)
- Protected UI mode **OFF** in game settings

---

## For Extension Developers

X4Native provides a C++ SDK for building native extensions that run inside the game process.

- **Full game API access** — call any of X4's exported game functions, fully typed
- **Function hooking** — intercept game functions with before/after hooks
- **Event system** — lifecycle events, custom events, Lua-to-C++ bridges
- **Hot-reloadable** — `/reloadui` picks up new builds without restarting
- **Stash** — in-memory state that survives `/reloadui` and extension reloads

### Getting the SDK

Download the **X4Native SDK** from the [latest release](https://github.com/eg3r/X4Native/releases). The SDK package contains everything you need:

- `sdk/` — header files (`x4native.h` umbrella, core headers `x4n_core.h`/`x4n_events.h`/`x4n_log.h`/`x4n_stash.h`/`x4n_hooks.h`, domain helpers `x4n_entity.h`/`x4n_math.h`/`x4n_memory.h`/`x4n_rooms.h`/`x4n_plans.h`/`x4n_visibility.h`, game type headers)
- `CMakeLists.txt` — ready-to-use CMake template for your extension

Point your CMake project at the SDK `include` path and you're ready to build. See [docs/EXTENSION_GUIDE.md](docs/EXTENSION_GUIDE.md) for the full developer guide — project setup, API reference, and all available features.

### Quick Start

```cpp
#include <x4native.h>

X4N_EXTENSION {
    x4n::log::info("Hello X4!");

    x4n::on("on_game_loaded", [] {
        auto* game = x4n::game();
        UniverseID player = game->GetPlayerID();
        x4n::log::info("Player ID: %llu", player);
    });
}

X4N_SHUTDOWN {
    x4n::log::info("Goodbye!");
}
```

### Examples

| Example | What it shows |
|---------|---------------|
| [hello](examples/hello) | Lifecycle events, game function calls |
| [event_test](examples/event_test) | Event round-trips (C++→Lua→C++) |
| [hook_test](examples/hook_test) | Before/after hooks on game functions |
| [lua_bridge](examples/lua_bridge) | Dynamic Lua→C++ event forwarding |

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for build instructions, architecture overview, deploy workflow, and development setup.


## License

MIT — see [LICENSE](LICENSE).
