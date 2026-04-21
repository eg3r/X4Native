// ---------------------------------------------------------------------------
// x4native_lua_bridge — Sample extension demonstrating dynamic Lua→C++ bridges
//
// Uses x4n::bridge_lua_event() to forward game Lua events into the C++ event
// system, then subscribes to those C++ events with x4n::on().
// No Lua code needed — bridges are registered entirely from C++.
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>

static void on_undocked(const char* param) {
    x4n::log::info("lua_bridge: player undocked (param: {})", param ? param : "none");
}

static void on_conversation_finished(const char* param) {
    x4n::log::info("lua_bridge: conversation finished (param: {})", param ? param : "none");
}

static void on_saved(const char* param) {
    x4n::log::info("lua_bridge: game was saved (param: {})", param ? param : "none");
}

static void on_game_loaded() {
    x4n::log::info("lua_bridge: game loaded — registering Lua bridges");

    // Bridge Lua events → C++ events (name mapping is arbitrary)
    x4n::bridge_lua_event("playerUndocked",        "on_player_undocked");
    x4n::bridge_lua_event("conversationFinished",   "on_conversation_finished");
    x4n::bridge_lua_event("gameSaved",              "on_game_saved_lua");

    // Subscribe to the C++ side of bridges
    x4n::on("on_player_undocked",        on_undocked);
    x4n::on("on_conversation_finished",  on_conversation_finished);
    x4n::on("on_game_saved_lua",         on_saved);

    x4n::log::info("lua_bridge: 3 bridges active");
}

X4N_EXTENSION {
    x4n::log::info("lua_bridge: init");
    x4n::on("on_game_loaded", on_game_loaded);
}

X4N_SHUTDOWN {
    x4n::log::info("lua_bridge: shutdown");
}
