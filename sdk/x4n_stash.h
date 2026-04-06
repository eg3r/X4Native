// ---------------------------------------------------------------------------
// x4n_stash.h — In-Memory Key-Value Stash
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Survives /reloadui + extension hot-reload. Lost on game exit.
// Keys are scoped to a namespace (your extension name by default).
//
// Usage:
//   x4n::stash::set("hp", 100);
//   int hp; x4n::stash::get("hp", &hp);
//   x4n::stash::set_string("name", "test");
//   const char* s = x4n::stash::get_string("name");
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

namespace x4n {
namespace stash {

/// Stash a raw blob under the extension's default namespace.
inline bool set(const char* key, const void* data, uint32_t size) {
    auto* api = detail::g_api;
    const char* ns = api->_ext_name;
    return api->stash_set(ns, key, data, size) != 0;
}

/// Retrieve a raw blob. Returns nullptr if not found.
/// *out_size receives the byte count. Pointer valid until next set/remove.
inline const void* get(const char* key, uint32_t* out_size = nullptr) {
    auto* api = detail::g_api;
    const char* ns = api->_ext_name;
    return api->stash_get(ns, key, out_size);
}

/// Remove a single key. Returns true if the key existed.
inline bool remove(const char* key) {
    auto* api = detail::g_api;
    const char* ns = api->_ext_name;
    return api->stash_remove(ns, key) != 0;
}

/// Remove all keys belonging to this extension.
inline void clear() {
    auto* api = detail::g_api;
    const char* ns = api->_ext_name;
    api->stash_clear(ns);
}

/// Stash a trivially-copyable value.
template<typename T>
inline bool set(const char* key, const T& val) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "x4n::stash::set<T> requires a trivially-copyable type");
    return set(key, &val, static_cast<uint32_t>(sizeof(T)));
}

/// Retrieve a trivially-copyable value. Returns true if found and size matches.
template<typename T>
inline bool get(const char* key, T* out) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "x4n::stash::get<T> requires a trivially-copyable type");
    uint32_t size = 0;
    const void* p = get(key, &size);
    if (!p || size != sizeof(T)) return false;
    std::memcpy(out, p, sizeof(T));
    return true;
}

/// Stash a null-terminated string (includes the null terminator).
inline bool set_string(const char* key, const char* value) {
    if (!value) return false;
    return set(key, value, static_cast<uint32_t>(std::strlen(value) + 1));
}

/// Retrieve a stored string. Returns nullptr if not found.
inline const char* get_string(const char* key) {
    uint32_t size = 0;
    const void* p = get(key, &size);
    if (!p || size == 0) return nullptr;
    return static_cast<const char*>(p);
}

} // namespace stash
} // namespace x4n

