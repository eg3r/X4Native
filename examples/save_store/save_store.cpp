// ---------------------------------------------------------------------------
// x4native_save_store — Sample extension using the core's save store
//
// On every game load the Mission Director script asks for `loads` through a
// Lua event; this extension reads it from the save, adds one and writes it
// back, then raises the answer to the UI Lua file, which hands it to MD.
//
// Exercises x4n::save:
//   x4n::save::get("loads", int64_t{0})   read with a fallback
//   x4n::save::set("loads", loads)        store the new value
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>
#include <x4n_save.h>

#include <cstdint>
#include <string>

static int g_subscription = 0;

static void on_load_requested(const char* /*param*/) {
    const int64_t loads = x4n::save::get("loads", int64_t{0});
    x4n::log::info("save_store: game load: loads={}", loads);

    const int64_t next = loads + 1;
    if (!x4n::save::set("loads", next)) {
        x4n::log::error("save_store: the count could not be stored");
        return;
    }

    const std::string answer = std::to_string(next);
    x4n::raise_lua("x4native_save_store.loaded", answer.c_str());
    x4n::log::info("save_store: stored loads={}", next);
}

X4N_EXTENSION {
    x4n::log::info("save_store: init");

    // MD raises the game Lua event; the bridge forwards it to the C++ side.
    x4n::bridge_lua_event("x4native_save_store.load", "on_save_store_load");
    g_subscription = x4n::on("on_save_store_load", on_load_requested);
}

X4N_SHUTDOWN {
    x4n::log::info("save_store: shutdown");
    x4n::off(g_subscription);
}
