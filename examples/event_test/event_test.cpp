// ---------------------------------------------------------------------------
// x4native_event_test — Tests the bidirectional event system
//
// Demonstrates:
//   - Subscribing to built-in lifecycle events
//   - Subscribing to on_frame_update (Lua-bridged per-frame)
//   - Raising C++→Lua events via raise_lua_event
//   - Raising C++→C++ events via raise_event
//   - Unsubscribing from events
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>

// Subscription handles
static int g_sub_loaded   = 0;
static int g_sub_save     = 0;
static int g_sub_frame    = 0;
static int g_sub_reload   = 0;
static int g_sub_custom   = 0;
static int g_sub_pong     = 0;

// Frame counter — only log periodically to avoid spam
static int g_frame_count  = 0;

// ---------------------------------------------------------------------------
// Event callbacks
// ---------------------------------------------------------------------------

static void on_game_loaded() {
    x4n::log::info("event_test: [on_game_loaded] game world is ready");

    // Reset frame counter on each load
    g_frame_count = 0;

    // Demonstrate C++→Lua: send a ping to Lua, expect it to echo back
    // via Lua RegisterEvent → raise_event("on_event_test_pong")
    int rc = x4n::raise_lua("x4native_event_test.ping", "round_trip_test");
    if (rc == 0)
        x4n::log::info(
            "event_test: [on_game_loaded] C++->Lua ping sent ('x4native_event_test.ping')");
    else
        x4n::log::warn(
            "event_test: [on_game_loaded] raise_lua_event failed");

    // Demonstrate C++→C++ custom event
    x4n::raise("event_test.internal_ping");
}

static void on_game_save() {
    x4n::log::info("event_test: [on_game_save] game is saving");
}

static void on_frame_update() {
    g_frame_count++;

    // Log every 600 frames (~10 seconds at 60fps) to show it's working
    if (g_frame_count % 600 == 0) {
        x4n::log::debug(
            "event_test: [on_frame_update] %d frames processed", g_frame_count);
    }
}

static void on_ui_reload() {
    x4n::log::info(
        "event_test: [on_ui_reload] Lua state refreshed, re-registering bindings");
    g_frame_count = 0;
}

static void on_custom_ping() {
    x4n::log::info(
        "event_test: [event_test.internal_ping] received C++ inter-extension event");
}

static void on_pong() {
    // This fires when Lua echoes our ping back via raise_event
    x4n::log::info(
        "event_test: [on_event_test_pong] round trip complete! C++->Lua->C++");
}

// ---------------------------------------------------------------------------
// Extension lifecycle
// ---------------------------------------------------------------------------

X4N_EXTENSION {
    x4n::log::info("event_test: initializing — subscribing to events");

    // Subscribe to all built-in lifecycle events
    g_sub_loaded = x4n::on("on_game_loaded",  on_game_loaded);
    g_sub_save   = x4n::on("on_game_save",    on_game_save);
    g_sub_frame  = x4n::on("on_frame_update", on_frame_update);
    g_sub_reload = x4n::on("on_ui_reload",    on_ui_reload);

    // Subscribe to our own custom C++ event
    g_sub_custom = x4n::on("event_test.internal_ping", on_custom_ping);

    // Subscribe for the Lua echo (round-trip test)
    g_sub_pong = x4n::on("on_event_test_pong", on_pong);

    x4n::log::info(
        "event_test: subscribed to 6 events (ids: %d, %d, %d, %d, %d, %d)",
        g_sub_loaded, g_sub_save, g_sub_frame, g_sub_reload, g_sub_custom, g_sub_pong);
}

X4N_SHUTDOWN {
    x4n::log::info("event_test: shutting down — unsubscribing all events");

    x4n::off(g_sub_loaded);
    x4n::off(g_sub_save);
    x4n::off(g_sub_frame);
    x4n::off(g_sub_reload);
    x4n::off(g_sub_custom);
    x4n::off(g_sub_pong);

    x4n::log::info("event_test: total frames seen: %d", g_frame_count);
}
