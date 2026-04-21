// ---------------------------------------------------------------------------
// x4native_native_tick — Native Frame Tick Example
//
// Demonstrates:
//   - Subscribing to on_native_frame_update (core-owned engine hook)
//   - Using the typed X4NativeFrameUpdate struct
//   - Calling safe read-only game functions from the tick callback
//   - Periodic logging with frame timing data
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>

static int g_sub_tick   = 0;
static int g_sub_loaded = 0;

// Accumulate time to log periodically (every ~5 seconds)
static double g_accum = 0.0;

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

static void on_native_tick(const X4NativeFrameUpdate* frame) {
    g_accum += frame->delta;

    // Log a status line every ~5 seconds of real frame time
    if (g_accum >= 5.0) {
        g_accum = 0.0;

        // Query some safe read-only game functions
        auto* gf = x4n::game();

        const char* player_name = "N/A";
        const char* ship_name   = "N/A";
        const char* faction     = "N/A";

        if (gf) {
            if (gf->GetPlayerName)
                player_name = gf->GetPlayerName();

            if (gf->GetPlayerFactionName)
                faction = gf->GetPlayerFactionName(false);

            if (gf->GetPlayerControlledShipID && gf->GetComponentName) {
                UniverseID ship_id = gf->GetPlayerControlledShipID();
                if (ship_id)
                    ship_name = gf->GetComponentName(ship_id);
            }
        }

        x4n::log::info(
            "native_tick: player='{}' faction='{}' ship='{}' | "
            "fps={:.1f} delta={:.4f}s game_time={:.1f} speed={:.0f}x paused={} suspended={}",
            player_name, faction, ship_name,
            frame->fps, frame->delta, frame->game_time,
            frame->speed_multiplier,
            frame->game_paused, frame->is_suspended);
    }
}

static void on_game_loaded() {
    g_accum = 4.5;  // trigger first log quickly (after ~0.5s)
    x4n::log::info("native_tick: game loaded, native frame updates active");
}

// ---------------------------------------------------------------------------
// Extension lifecycle
// ---------------------------------------------------------------------------

X4N_EXTENSION {
    x4n::log::info("native_tick: initializing");

    g_sub_tick   = x4n::on("on_native_frame_update", on_native_tick);
    g_sub_loaded = x4n::on("on_game_loaded",         on_game_loaded);

    x4n::log::info("native_tick: subscribed (tick={}, loaded={})",
                   g_sub_tick, g_sub_loaded);
}

X4N_SHUTDOWN {
    x4n::log::info("native_tick: shutting down");
    x4n::off(g_sub_tick);
    x4n::off(g_sub_loaded);
}
