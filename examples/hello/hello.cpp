// ---------------------------------------------------------------------------
// x4native_hello — Sample extension that logs lifecycle events
//
// Also exercises the new per-extension logging API:
//   x4n::log::info("text")           → hello.log  (default, auto-opened)
//   x4n::log::info("text", false)    → x4native.log (global)
//   x4n::log::info("text", "f.log")  → named file in extension folder
//   x4n::log::set_log_file("f.log")  → redirect default to a different file
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>

static X4GameFunctions* game = nullptr;
static int g_sub_loaded = 0;
static int g_sub_saved  = 0;

static void on_game_loaded() {
    // Default: goes to hello.log
    x4n::log::info("hello: game loaded!");

    if (game && game->GetPlayerID) {
        UniverseID player = game->GetPlayerID();
        x4n::log::info("hello: player entity ID retrieved");
    }

    // Named file: one-shot write to a separate file in the extension folder
    x4n::log::info("game loaded event fired", "events.log");
}

static void on_game_save() {
    x4n::log::info("hello: game saved!");
    x4n::log::info("save event fired", "events.log");
}

X4N_EXTENSION {
    game = x4n::game();

    // Default per-extension log (hello.log) — framework opened it before this runs
    x4n::log::info("hello: init called");
    x4n::log::info("hello: game version: %s", x4n::game_version());
    x4n::log::info("hello: ext path: %s", x4n::path());

    // Route one message to the shared x4native.log
    x4n::log::info("hello extension loaded", false);

    if (game)
        x4n::log::info("hello: game function table available");
    else
        x4n::log::warn("hello: game function table NOT available");

    // Named file: write a startup note to a separate log in the extension folder
    x4n::log::info("hello extension initialised", "hello_startup.log");

    // Redirect the default log to a new file for the rest of the session
    // (uncomment to test set_log_file — subsequent info/warn/error go to hello_v2.log)
    // x4n::log::set_log_file("hello_v2.log");
    // x4n::log::info("hello: this goes to hello_v2.log");

    g_sub_loaded = x4n::on("on_game_loaded", on_game_loaded);
    g_sub_saved  = x4n::on("on_game_save",   on_game_save);
}

X4N_SHUTDOWN {
    x4n::log::info("hello: shutting down");
    x4n::log::info("hello extension unloaded", false);  // also note in x4native.log
    x4n::off(g_sub_loaded);
    x4n::off(g_sub_saved);
}
