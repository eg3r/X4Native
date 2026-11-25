// ---------------------------------------------------------------------------
// x4n_save.h — Key-value data in the save
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// The core writes one `<x4native.store>` element into the save, as the last
// child of `<universe>`, and keeps each extension's values in its own
// `<x4native.ext id="...">` block there. An extension stores and reads its
// values through the calls below; it never builds a node of its own for them.
// The raw node API (x4n_savexml.h) stays for the extensions that need more.
//
// Provides:
//   x4n::save::set / set_string   — store a value under a key
//   x4n::save::get / get_string   — read it, with a fallback when absent
//   x4n::save::remove             — drop one key
//
//   - Keys are attribute values, never element names, so they never touch the
//     SaveXML name table.
//   - Types are bool, int (64-bit), number (double) and string. A get with a
//     type other than the stored one logs a warning and returns its fallback.
//   - Values follow the world they were loaded with: a save load replaces
//     them, a new game starts empty, and a UI reload keeps them.
//   - Values of an extension that is not loaded are dropped on the next save.
//   - Store at `on_game_loaded` or later; before the world is known there is
//     nothing to attach the values to.
//
// Usage:
//   x4n::save::set("intro_seen", true);
//   x4n::save::set<int64_t>("counter", 42);
//   x4n::save::set_string("fleet_name", "Red Wing");
//   bool seen = x4n::save::get("intro_seen", false);
//   const char* name = x4n::save::get_string("fleet_name", "");
//
// Responsibility:
//   - The core: the XML shape, the load and save hooks, and the stash copy
//     that crosses a UI reload.
//   - The extension: the keys and values it stores, and reading each one back
//     with the type it was written.
//   See docs/rev/SAVE_XML.md section 7.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
namespace x4n { namespace save {

namespace detail {

inline X4NativeAPI* api() { return ::x4n::detail::g_api; }

}  // namespace detail

/// Store `value` under `key`.
inline bool set(const char* key, bool value) {
    auto* api = detail::api();
    if (!api || !api->save_set || !key) return false;
    const int stored = value ? 1 : 0;
    return api->save_set(key, X4N_SAVE_BOOL, &stored, api) != 0;
}

/// Store a 64-bit integer under `key`.
inline bool set(const char* key, int64_t value) {
    auto* api = detail::api();
    if (!api || !api->save_set || !key) return false;
    return api->save_set(key, X4N_SAVE_INT, &value, api) != 0;
}

/// Store a 32-bit integer under `key` as a 64-bit one.
inline bool set(const char* key, int value) { return set(key, static_cast<int64_t>(value)); }

/// Store any other integer under `key` as a 64-bit one.
template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
inline bool set(const char* key, T value) {
    return set(key, static_cast<int64_t>(value));
}

/// Store a double under `key`.
inline bool set(const char* key, double value) {
    auto* api = detail::api();
    if (!api || !api->save_set || !key) return false;
    return api->save_set(key, X4N_SAVE_NUMBER, &value, api) != 0;
}

/// Store a NUL-terminated string under `key`.
inline bool set_string(const char* key, const char* value) {
    auto* api = detail::api();
    if (!api || !api->save_set || !key || !value) return false;
    return api->save_set(key, X4N_SAVE_STRING, value, api) != 0;
}

/// The same, for a string literal or a `const char*` that would otherwise be
/// read as a bool.
inline bool set(const char* key, const char* value) { return set_string(key, value); }

/// The stored bool, or `fallback` when the key is absent or of another type.
inline bool get(const char* key, bool fallback) {
    auto* api = detail::api();
    if (!api || !api->save_get || !key) return fallback;
    int stored = 0;
    return api->save_get(key, X4N_SAVE_BOOL, &stored, sizeof stored, api) ? stored != 0 : fallback;
}

/// The stored integer, or `fallback` when the key is absent or of another type.
inline int64_t get(const char* key, int64_t fallback) {
    auto* api = detail::api();
    if (!api || !api->save_get || !key) return fallback;
    int64_t stored = 0;
    return api->save_get(key, X4N_SAVE_INT, &stored, sizeof stored, api) ? stored : fallback;
}

/// The same, for an `int` fallback.
inline int get(const char* key, int fallback) {
    return static_cast<int>(get(key, static_cast<int64_t>(fallback)));
}

/// The same, for any other integer fallback.
template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
inline T get(const char* key, T fallback) {
    return static_cast<T>(get(key, static_cast<int64_t>(fallback)));
}

/// The stored double, or `fallback` when the key is absent or of another type.
inline double get(const char* key, double fallback) {
    auto* api = detail::api();
    if (!api || !api->save_get || !key) return fallback;
    double stored = 0;
    return api->save_get(key, X4N_SAVE_NUMBER, &stored, sizeof stored, api) ? stored : fallback;
}

/// The stored string, or `fallback` when the key is absent or of another type.
/// The pointer stays valid until the next call on this thread. A stored string
/// longer than 4095 bytes does not fit and returns the fallback.
inline const char* get_string(const char* key, const char* fallback) {
    auto* api = detail::api();
    if (!api || !api->save_get || !key) return fallback;

    static thread_local std::string buffer;
    buffer.resize(4096);
    if (!api->save_get(key, X4N_SAVE_STRING, buffer.data(), static_cast<uint32_t>(buffer.size()), api)) {
        return fallback;
    }
    buffer.resize(std::strlen(buffer.c_str()));
    return buffer.c_str();
}

/// The same, for a string literal or a `const char*` that would otherwise be
/// read as a bool.
inline const char* get(const char* key, const char* fallback) { return get_string(key, fallback); }

/// Drop `key`. Returns true if it was stored.
inline bool remove(const char* key) {
    auto* api = detail::api();
    if (!api || !api->save_remove || !key) return false;
    return api->save_remove(key, api) != 0;
}

}} // namespace x4n::save
