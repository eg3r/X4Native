// ---------------------------------------------------------------------------
// x4n_save.h — Values stored in the savegame
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// A key-value store per extension that is saved with the game and comes back
// when that save is loaded. Shaped like x4n::stash, but the values belong to
// the game the player is in, not to the session.
//
// Usage:
//   x4n::save::on_ready([] {                        // this game's values are in
//       int64_t visits = x4n::save::get_int("visits", 0);
//       x4n::save::set_int("visits", visits + 1);
//   });
//   x4n::save::set_string("fleet_name", "Red Wing");
//   x4n::save::set_bool("intro_seen", true);
//   bool seen = x4n::save::get_bool("intro_seen", false);
//
// Lifecycle:
//   - Read and write from the `on_save_data_ready` event on (on_ready below).
//     It fires after `on_game_loaded`: the game hands a loaded save's values
//     back a moment after the world exists. For a new game it fires with
//     nothing stored. It fires again after /reloadui and after a hot-reload
//     of your extension, when it is initialised again, with the values
//     unchanged.
//   - Before that, the getters return their fallback and the setters false.
//   - Values are kept apart per extension (content.xml id) and per game: they
//     are dropped when another game starts or loads.
//   - A value you set is part of every save made from the next UI frame on.
//   - A game saved without your extension keeps your values for when the
//     extension is back.
//
// Limits: a key has 1 to 128 bytes, a value at most 256 KiB, and all
// extensions together 2 MiB, counted as stored: a byte other than a letter,
// digit, '_', '-' or '.' takes three. This is for the state of a game, not
// for bulk data. A setter returns false when a limit is hit.
//
// Values are bytes. The typed helpers store text ("1", "42", "0.5"), which
// keeps a save readable and works on every platform; use set/get for binary
// data.
//
// Call from the game's main thread, as every event handler does.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"
#include "x4n_events.h"

#include <charconv>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace x4n { namespace save {

/// Whether the running X4Native supports save data. False on an older core.
inline bool available() {
    auto* api = detail::g_api;
    return api && api->save_set && api->save_get && api->save_remove && api->save_clear;
}

/// Whether the values of the current game can be read and written.
inline bool ready() {
    auto* api = detail::g_api;
    return api && api->save_ready && api->save_ready() != 0;
}

/// Runs `callback` each time a game's values become readable: after a load,
/// a new game start and a /reloadui. Returns the subscription id for x4n::off.
inline int on_ready(void (*callback)()) {
    return x4n::on("on_save_data_ready", callback);
}

// --- Raw bytes -------------------------------------------------------------

/// Stores a copy of `size` bytes under `key`.
inline bool set(const char* key, const void* data, uint32_t size) {
    auto* api = detail::g_api;
    return api && api->save_set && api->save_set(key, data, size, api) != 0;
}

/// The stored bytes, or nullptr. *out_size receives the byte count. The bytes
/// are followed by a NUL. The pointer is valid until you change the key or
/// another game loads; copy what you keep.
inline const void* get(const char* key, uint32_t* out_size = nullptr) {
    auto* api = detail::g_api;
    if (out_size) *out_size = 0;
    return (api && api->save_get) ? api->save_get(key, out_size, api) : nullptr;
}

/// Whether a value is stored under `key`.
inline bool has(const char* key) { return get(key) != nullptr; }

/// Removes one key. Returns true if it existed.
inline bool remove(const char* key) {
    auto* api = detail::g_api;
    return api && api->save_remove && api->save_remove(key, api) != 0;
}

/// Removes every key of this extension.
inline void clear() {
    auto* api = detail::g_api;
    if (api && api->save_clear) api->save_clear(api);
}

// --- Text ------------------------------------------------------------------

/// Stores text (without a terminator; get_string returns it terminated).
inline bool set_string(const char* key, std::string_view value) {
    return set(key, value.data(), static_cast<uint32_t>(value.size()));
}

/// The stored text, or `fallback`. Valid until you change the key or another
/// game loads; copy it to keep it.
inline const char* get_string(const char* key, const char* fallback = nullptr) {
    const void* p = get(key);
    return p ? static_cast<const char*>(p) : fallback;
}

// --- Booleans and numbers (stored as text) ---------------------------------

inline bool set_bool(const char* key, bool value) {
    return set(key, value ? "1" : "0", 1);
}

inline bool get_bool(const char* key, bool fallback = false) {
    uint32_t size = 0;
    const char* p = static_cast<const char*>(get(key, &size));
    if (!p || size != 1 || (p[0] != '0' && p[0] != '1')) return fallback;
    return p[0] == '1';
}

inline bool set_int(const char* key, int64_t value) {
    char text[24];
    const auto end = std::to_chars(text, text + sizeof(text), value).ptr;
    return set(key, text, static_cast<uint32_t>(end - text));
}

inline int64_t get_int(const char* key, int64_t fallback = 0) {
    uint32_t size = 0;
    const char* p = static_cast<const char*>(get(key, &size));
    int64_t value = 0;
    if (!p || size == 0) return fallback;
    const auto parsed = std::from_chars(p, p + size, value);
    return (parsed.ec == std::errc() && parsed.ptr == p + size) ? value : fallback;
}

/// Stores a double with every digit it needs to read back exactly.
inline bool set_number(const char* key, double value) {
    char text[32];
    const auto end = std::to_chars(text, text + sizeof(text), value).ptr;
    return set(key, text, static_cast<uint32_t>(end - text));
}

inline double get_number(const char* key, double fallback = 0.0) {
    uint32_t size = 0;
    const char* p = static_cast<const char*>(get(key, &size));
    double value = 0.0;
    if (!p || size == 0) return fallback;
    const auto parsed = std::from_chars(p, p + size, value);
    return (parsed.ec == std::errc() && parsed.ptr == p + size) ? value : fallback;
}

}} // namespace x4n::save
