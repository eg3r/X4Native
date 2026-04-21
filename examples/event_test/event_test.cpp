// ---------------------------------------------------------------------------
// x4native_event_test — Tests the bidirectional event system
//
// Demonstrates:
//   - Subscribing to built-in lifecycle events
//   - Subscribing to on_frame_update (Lua-bridged per-frame)
//   - Raising C++→Lua events via raise_lua_event
//   - Raising C++→C++ events via raise_event
//   - Direct MD event subscriptions (typed, no bridge needed)
//   - Unsubscribing from events
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4_md_events.h>
#include <x4n_log.h>

// Subscription handles — lifecycle & custom events
static int g_sub_loaded   = 0;
static int g_sub_save     = 0;
static int g_sub_frame    = 0;
static int g_sub_reload   = 0;
static int g_sub_custom   = 0;
static int g_sub_pong     = 0;

// Subscription handles — direct MD events
static int g_sub_md_killed       = 0;
static int g_sub_md_relation     = 0;
static int g_sub_md_sector_owner = 0;
static int g_sub_md_assignment   = 0;
static int g_sub_md_build        = 0;
static int g_sub_md_sub_added    = 0;
static int g_sub_md_sub_removed  = 0;

// Frame counter — only log periodically to avoid spam
static int g_frame_count  = 0;

// MD event counters — log periodically instead of per-event
static int g_md_killed_count       = 0;
static int g_md_relation_count     = 0;
static int g_md_sector_owner_count = 0;
static int g_md_assignment_count   = 0;
static int g_md_build_count        = 0;
static int g_md_sub_added_count    = 0;
static int g_md_sub_removed_count  = 0;

// ---------------------------------------------------------------------------
// Event callbacks
// ---------------------------------------------------------------------------

static void on_game_loaded() {
    x4n::log::info("event_test: [on_game_loaded] game world is ready");

    // Reset counters on each load
    g_frame_count = 0;
    g_md_killed_count = 0;
    g_md_relation_count = 0;
    g_md_sector_owner_count = 0;
    g_md_assignment_count = 0;
    g_md_build_count = 0;
    g_md_sub_added_count = 0;
    g_md_sub_removed_count = 0;

    // --- Direct MD event subscriptions (typed, no bridge needed) ---

    g_sub_md_killed = x4n::md::on_killed_after(
        [](const x4n::md::KilledData& e) {
            g_md_killed_count++;
            if (g_md_killed_count <= 3)
                x4n::log::info("event_test: [md:killed] entity {} killer={} method={}",
                    e.source_id, e.killer, e.kill_method);
        });

    g_sub_md_relation = x4n::md::on_faction_relation_changed_after(
        [](const x4n::md::FactionRelationChangedData& e) {
            g_md_relation_count++;
            if (g_md_relation_count <= 3)
                x4n::log::info("event_test: [md:faction_relation] changed at t={:.1f}",
                    e.timestamp);
        });

    g_sub_md_sector_owner = x4n::md::on_sector_changed_owner_after(
        [](const x4n::md::SectorChangedOwnerData& e) {
            g_md_sector_owner_count++;
            x4n::log::info("event_test: [md:sector_owner] sector {} new_owner {} (prev {})",
                e.sector_changing_ownership, e.new_owner_faction, e.previous_owner_faction);
        });

    g_sub_md_assignment = x4n::md::on_changed_assignment_after(
        [](const x4n::md::ChangedAssignmentData& e) {
            g_md_assignment_count++;
            if (g_md_assignment_count <= 5)
                x4n::log::info("event_test: [md:assignment68] subordinate={} src={}",
                    e.subordinate, e.source_id);
        });

    g_sub_md_build = x4n::md::on_build_finished_after(
        [](const x4n::md::BuildFinishedData& e) {
            g_md_build_count++;
            if (g_md_build_count <= 20)
                x4n::log::info("event_test: [md:build_finished] #{} source={} buildprocessor={}",
                    g_md_build_count, e.source_id, e.buildprocessor);
        });

    g_sub_md_sub_added = x4n::md::on_subordinate_added_after(
        [](const x4n::md::SubordinateAddedData& e) {
            g_md_sub_added_count++;
            if (g_md_sub_added_count <= 10)
                x4n::log::info("event_test: [md:sub_added] #{} sub={} commander={}",
                    g_md_sub_added_count, e.subordinate, e.source_id);
        });

    g_sub_md_sub_removed = x4n::md::on_subordinate_removed_after(
        [](const x4n::md::SubordinateRemovedData& e) {
            g_md_sub_removed_count++;
            if (g_md_sub_removed_count <= 10)
                x4n::log::info("event_test: [md:sub_removed] #{} sub={} old_cmdr={}",
                    g_md_sub_removed_count, e.subordinate, e.source_id);
        });

    x4n::log::info("event_test: [on_game_loaded] subscribed to 7 MD events");

    // --- C++→Lua round-trip test ---

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
            "event_test: [on_frame_update] {} frames | killed={} relation={} sector={} assign68={} build={} sub_add={} sub_rm={}",
            g_frame_count, g_md_killed_count, g_md_relation_count,
            g_md_sector_owner_count, g_md_assignment_count, g_md_build_count,
            g_md_sub_added_count, g_md_sub_removed_count);
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
        "event_test: subscribed to 6 lifecycle/custom events (ids: {}, {}, {}, {}, {}, {})",
        g_sub_loaded, g_sub_save, g_sub_frame, g_sub_reload, g_sub_custom, g_sub_pong);
    x4n::log::info(
        "event_test: MD event subscriptions registered in on_game_loaded (7 types)");
}

X4N_SHUTDOWN {
    x4n::log::info("event_test: shutting down — unsubscribing all events");

    // Lifecycle & custom events
    x4n::off(g_sub_loaded);
    x4n::off(g_sub_save);
    x4n::off(g_sub_frame);
    x4n::off(g_sub_reload);
    x4n::off(g_sub_custom);
    x4n::off(g_sub_pong);

    // MD events
    x4n::off(g_sub_md_killed);
    x4n::off(g_sub_md_relation);
    x4n::off(g_sub_md_sector_owner);
    x4n::off(g_sub_md_assignment);
    x4n::off(g_sub_md_build);
    x4n::off(g_sub_md_sub_added);
    x4n::off(g_sub_md_sub_removed);

    x4n::log::info("event_test: total frames: {} | killed={} relation={} sector={} assign68={} build={} sub_add={} sub_rm={}",
        g_frame_count, g_md_killed_count, g_md_relation_count,
        g_md_sector_owner_count, g_md_assignment_count, g_md_build_count,
        g_md_sub_added_count, g_md_sub_removed_count);
}
