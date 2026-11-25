// ---------------------------------------------------------------------------
// x4native_save_data — Sample extension for values stored in the savegame
//
// x4n::save is a key-value store per extension that is saved with the game:
// what you set is in the next save, and comes back when that save is loaded.
//
// Shows:
//   x4n::save::on_ready      restore your state once a game's values are in
//   typed values             set_bool / set_int / set_number / set_string
//   raw bytes                a small struct through set / get
//   has / remove / clear
//   changing values at runtime: when the game is saved, and on a Lua event
//
// What it keeps for each game:
//   loads        how often the extension has seen this game come up
//   saves        how often this game was saved
//   first_seen   game time when the extension first saw this game
//   intro_seen   a flag
//   note         a text you set from Lua or MD
//   position     a struct of three doubles, stored as bytes
//
// Commands. Raise the Lua event "x4native_save_data.command" with one of
// these as its argument, from a Lua file or from MD (<raise_lua_event>):
//   set <key> <text>     store text under any key
//   get <key>            log one value
//   remove <key>         remove one value
//   clear                remove everything this extension stored
//   dump                 log the values listed above
//   selftest write       store values that are awkward to save: text that
//                        looks like a number, control and NUL bytes, UTF-8,
//                        an empty value, 100 KiB of bytes
//   selftest check       read them back and log PASS or FAIL
//
// Everything is logged to this extension's log file.
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>
#include <x4n_save.h>

#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

static X4GameFunctions* game = nullptr;

struct Position {
    double x, y, z;
};

static void dump() {
    Position position = {};
    uint32_t size = 0;
    const void* bytes = x4n::save::get("position", &size);
    if (bytes && size == sizeof(position)) std::memcpy(&position, bytes, sizeof(position));

    x4n::log::info("save_data: loads={} saves={} first_seen={:.1f} intro_seen={} note='{}' position=({}, {}, {})",
                   x4n::save::get_int("loads", 0),
                   x4n::save::get_int("saves", 0),
                   x4n::save::get_number("first_seen", -1.0),
                   x4n::save::get_bool("intro_seen", false),
                   x4n::save::get_string("note", "(none)"),
                   position.x, position.y, position.z);
}

// A game's values are in: after a load, a new game start, and a /reloadui.
// This is the place to restore state, not on_game_loaded: the game hands a
// loaded save's values back a moment after the world exists.
static void on_save_data_ready() {
    if (!x4n::save::has("first_seen")) {
        // First time in this game: a new game, or the extension was added to an existing one.
        const double now = (game && game->GetCurrentGameTime) ? game->GetCurrentGameTime() : 0.0;
        x4n::save::set_number("first_seen", now);
        x4n::save::set_bool("intro_seen", false);
        const Position start = {1.5, -2.25, 1e9};
        x4n::save::set("position", &start, sizeof(start));
        x4n::log::info("save_data: first time in this game, initial values stored");
    }
    x4n::save::set_int("loads", x4n::save::get_int("loads", 0) + 1);
    x4n::log::info("save_data: values ready");
    dump();
}

// The game was saved. The save just written holds the values as they were;
// what is set here is part of the next one.
static void on_game_save() {
    if (!x4n::save::ready()) return;
    x4n::save::set_int("saves", x4n::save::get_int("saves", 0) + 1);
    x4n::log::info("save_data: game saved, saves={}", x4n::save::get_int("saves", 0));
}

// --- selftest: values that are awkward to save ------------------------------

static std::vector<std::pair<std::string, std::string>> selftest_values() {
    std::string every_byte;
    for (int i = 0; i < 256; ++i) every_byte.push_back(static_cast<char>(i));
    std::string large(100 * 1024, '\0');
    for (size_t i = 0; i < large.size(); ++i) large[i] = static_cast<char>((i * 31 + 7) & 0xFF);

    return {
        {"t.number",   "123"},
        {"t.float",    "1.5e10"},
        {"t.hex",      "0x10"},
        {"t.inf",      "inf"},
        {"t.nan",      "nan"},
        {"t.empty",    ""},
        {"t.spaces",   "  two  spaces  "},
        {"t.quotes",   "say \"hi\" \\ 'there' <tag> & more"},
        {"t.lua",      "]] .. os.exit() -- [["},
        {"t.newlines", "line one\nline two\r\nline three\ttab"},
        {"t.utf8",     "\xC3\xA4\xC3\xB6\xC3\xBC \xE2\x82\xAC \xF0\x9F\x9A\x80"},
        {"t.bytes",    every_byte},
        {"t.large",    large},
        {"123",        "a key that looks like a number"},
        {"inf",        "a key that looks like infinity"},
        {"key with spaces & \"quotes\"", "an awkward key"},
    };
}

static void selftest_write() {
    int stored = 0;
    const auto values = selftest_values();
    for (const auto& [key, value] : values)
        if (x4n::save::set(key.c_str(), value.data(), static_cast<uint32_t>(value.size()))) ++stored;
    x4n::log::info("save_data: selftest write: {} of {} values stored", stored, values.size());
}

static void selftest_check() {
    int good = 0;
    const auto values = selftest_values();
    for (const auto& [key, value] : values) {
        uint32_t size = 0;
        const char* stored = static_cast<const char*>(x4n::save::get(key.c_str(), &size));
        if (stored && std::string_view(stored, size) == value) {
            ++good;
        } else {
            x4n::log::error("save_data: selftest check: '{}' differs ({} of {} bytes)",
                            key, stored ? size : 0, value.size());
        }
    }
    x4n::log::info("save_data: selftest check: {} ({} of {} values intact)",
                   good == static_cast<int>(values.size()) ? "PASS" : "FAIL", good, values.size());
}

// --- commands from Lua or MD ------------------------------------------------

static void on_command(const char* param) {
    const std::string_view command = param ? param : "";
    const auto space = command.find(' ');
    const std::string_view verb = command.substr(0, space);
    const std::string_view rest = space == std::string_view::npos ? "" : command.substr(space + 1);

    if (!x4n::save::ready()) {
        x4n::log::warn("save_data: '{}' ignored, no game's values are loaded", command);
        return;
    }

    if (verb == "set") {
        const auto split = rest.find(' ');
        const std::string key(rest.substr(0, split));
        const std::string_view text = split == std::string_view::npos ? "" : rest.substr(split + 1);
        const bool ok = x4n::save::set_string(key.c_str(), text);
        x4n::log::info("save_data: set '{}' = '{}' -> {}", key, text, ok ? "stored" : "rejected");
    } else if (verb == "get") {
        const std::string key(rest);
        x4n::log::info("save_data: get '{}' = '{}'", key, x4n::save::get_string(key.c_str(), "(not set)"));
    } else if (verb == "remove") {
        const std::string key(rest);
        x4n::log::info("save_data: remove '{}' -> {}", key, x4n::save::remove(key.c_str()) ? "removed" : "not set");
    } else if (verb == "clear") {
        x4n::save::clear();
        x4n::log::info("save_data: cleared");
    } else if (verb == "dump") {
        dump();
    } else if (verb == "selftest" && rest == "write") {
        selftest_write();
    } else if (verb == "selftest" && rest == "check") {
        selftest_check();
    } else {
        x4n::log::warn("save_data: unknown command '{}'", command);
    }
}

X4N_EXTENSION {
    game = x4n::game();

    if (!x4n::save::available()) {
        x4n::log::warn("save_data: this X4Native has no save data support");
        return;
    }

    x4n::save::on_ready(on_save_data_ready);
    x4n::on("on_game_save", on_game_save);

    // Lua event -> C++ event, so Lua files and MD scripts can send commands.
    x4n::bridge_lua_event("x4native_save_data.command", "x4native_save_data_command");
    x4n::on("x4native_save_data_command", on_command);

    x4n::log::info("save_data: init");
}

X4N_SHUTDOWN {
    x4n::log::info("save_data: shutdown");
}
