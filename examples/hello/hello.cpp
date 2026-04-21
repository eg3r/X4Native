// ---------------------------------------------------------------------------
// x4native_hello — Sample extension that logs lifecycle events
//
// Exercises the logging API:
//   x4n::log::info("text")              → hello.log (this extension's log)
//   x4n::log::info("count: {}", n)      → same, with std::format-style args
//   x4n::log::global.info("note")       → x4native.log (framework log)
//   x4n::log::to_file("events.log").info(...)  → named file in ext folder
//   x4n::log::set_log_file("f.log")     → redirect default to a different file
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>
#include <x4n_settings.h>

static X4GameFunctions* game = nullptr;
static int g_sub_loaded  = 0;
static int g_sub_saved   = 0;
static int g_sub_setting = 0;

// Called only for this extension's own keys — see x4n::on_setting_changed.
// (on_any_setting_changed is available if you want cross-extension events.)
static void on_hello_setting_changed(const X4NativeSettingChanged& info) {
    switch (info.type) {
        case X4N_SETTING_TOGGLE:
            x4n::log::info("hello: setting changed: {} = {}",
                           info.key, info.b ? "true" : "false");
            break;
        case X4N_SETTING_SLIDER:
            x4n::log::info("hello: setting changed: {} = {:.2f}",
                           info.key, info.d);
            break;
        case X4N_SETTING_DROPDOWN:
            x4n::log::info("hello: setting changed: {} = {}",
                           info.key, info.s ? info.s : "(null)");
            break;
    }
}

static void on_game_loaded() {
    x4n::log::info("hello: game loaded!");

    if (game && game->GetPlayerID) {
        UniverseID player = game->GetPlayerID();
        x4n::log::info("hello: player entity id {}", player);
    }

    x4n::log::to_file("events.log").info("game loaded event fired");
}

static void on_game_save() {
    x4n::log::info("hello: game saved!");
    x4n::log::to_file("events.log").info("save event fired");
}

X4N_EXTENSION {
    game = x4n::game();

    // Default per-extension log (<profile>\x4native\x4native_hello.log) —
    // framework opens it before this runs.
    x4n::log::info("hello: init called");
    x4n::log::info("hello: game version: {}", x4n::game_version());
    x4n::log::info("hello: ext path: {}",     x4n::path());

    // Route one message to the shared framework log.
    x4n::log::global.info("hello extension loaded");

    if (game)
        x4n::log::info("hello: game function table available");
    else
        x4n::log::warn("hello: game function table NOT available");

    // One-shot write to a named file in the extension folder.
    x4n::log::to_file("hello_startup.log").info("hello extension initialised");

    // Read current settings (declared in x4native.json "settings" array).
    // First run seeds defaults; subsequent runs read persisted user values.
    bool        verbose  = x4n::settings::get_bool  ("verbose", false);
    double      poll_s   = x4n::settings::get_number("poll_interval_s", 5.0);
    const char* greeting = x4n::settings::get_string("greeting", "hello");
    x4n::log::info("hello: settings: verbose={} poll_interval_s={:.1f} greeting={}",
                   verbose, poll_s, greeting);

    g_sub_loaded  = x4n::on("on_game_loaded", on_game_loaded);
    g_sub_saved   = x4n::on("on_game_save",   on_game_save);
    g_sub_setting = x4n::on_setting_changed(on_hello_setting_changed);
}

X4N_SHUTDOWN {
    x4n::log::info("hello: shutting down");
    x4n::log::global.info("hello extension unloaded");
    x4n::off(g_sub_loaded);
    x4n::off(g_sub_saved);
    x4n::off(g_sub_setting);
}
