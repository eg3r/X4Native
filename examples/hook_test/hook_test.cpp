// ---------------------------------------------------------------------------
// x4native_hook_test — Sample extension demonstrating game function hooks
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>
#include <x4n_hooks.h>

static int g_before_hook = 0;
static int g_after_hook  = 0;

// Before-hook: runs before the original SetMapFilterString.
// Args are passed by reference — modifications affect the original call.
static void before_set_filter(x4n::hook::HookControl& ctl,
                              UniverseID& holomapid, uint32_t& numtexts,
                              const char**& textarray) {
    x4n::log::debug("hook_test: SetMapFilterString(holomap=%llu, n=%u) called",
                     holomapid, numtexts);
    for (uint32_t i = 0; i < numtexts; ++i)
        x4n::log::debug("hook_test:   text[%u] = %s", i,
                         textarray[i] ? textarray[i] : "(null)");

    // Example: skip the original call entirely
    // ctl.skip_original = 1;
}

// After-hook on a void-return function: receives args by value, no result param.
static void after_set_filter(UniverseID holomapid, uint32_t numtexts,
                             const char** textarray) {
    x4n::log::debug("hook_test: SetMapFilterString(holomap=%llu, n=%u) done",
                     holomapid, numtexts);
}

static void on_game_loaded() {
    x4n::log::info("hook_test: game loaded — installing hooks");

    g_before_hook =
        x4n::hook::before<&X4GameFunctions::SetMapFilterString>(before_set_filter);
    g_after_hook =
        x4n::hook::after<&X4GameFunctions::SetMapFilterString>(after_set_filter);

    if (g_before_hook > 0 && g_after_hook > 0)
        x4n::log::info("hook_test: hooks installed (before=#%d, after=#%d)",
                        g_before_hook, g_after_hook);
    else
        x4n::log::error("hook_test: hook installation failed");
}

X4N_EXTENSION {
    x4n::log::info("hook_test: init");
    x4n::on("on_game_loaded", on_game_loaded);
}

X4N_SHUTDOWN {
    x4n::log::info("hook_test: shutting down");
    if (g_before_hook > 0) x4n::hook::remove(g_before_hook);
    if (g_after_hook  > 0) x4n::hook::remove(g_after_hook);
}
