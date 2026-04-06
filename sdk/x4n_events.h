// ---------------------------------------------------------------------------
// x4n_events.h — Event Subscribe / Raise API
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Usage:
//   int id = x4n::on("on_game_loaded", [] { ... });   // save data ready, gamestart MD NOT yet fired
//   x4n::on("on_game_started", [] { ... });            // world fully ready, all gamestart MD cues done
//   x4n::on("on_frame_update", [](const X4NativeFrameUpdate* f) { ... });
//   x4n::off(id);
//   x4n::raise("my_event", data_ptr);
//   x4n::raise_lua("lua_event_name", "param");
//   x4n::bridge_lua_event("luaEvent", "cppEvent");
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

namespace x4n {

/// Subscribe with a void() callback (ignores event name and data).
inline int on(const char* name, void(*callback)()) {
    return detail::g_api->subscribe(
        name, detail::trampoline_void, reinterpret_cast<void*>(callback),
        detail::g_api);
}

/// Subscribe with a void(void*) callback (receives event data pointer).
inline int on(const char* name, void(*callback)(void*)) {
    return detail::g_api->subscribe(
        name, detail::trampoline_data, reinterpret_cast<void*>(callback),
        detail::g_api);
}

/// Subscribe with a typed X4NativeFrameUpdate callback (for on_native_frame_update).
inline int on(const char* name, void(*callback)(const X4NativeFrameUpdate*)) {
    return detail::g_api->subscribe(
        name, detail::trampoline_frame_update, reinterpret_cast<void*>(callback),
        detail::g_api);
}

/// Subscribe with a void(const char*) callback (receives string param from Lua bridges).
inline int on(const char* name, void(*callback)(const char*)) {
    return detail::g_api->subscribe(
        name, detail::trampoline_str, reinterpret_cast<void*>(callback),
        detail::g_api);
}

/// @deprecated Superseded by MD event hook system (type_id 376).
[[deprecated("Superseded by MD event hook system. Subscribe via on_md_before(376, trampoline, ud)")]]
inline int on(const char* name, void(*callback)(const X4RadarChangedEvent*)) {
    return detail::g_api->subscribe(
        name, detail::trampoline_radar_changed, reinterpret_cast<void*>(callback),
        detail::g_api);
}

/// Subscribe with the raw 3-argument callback and explicit userdata.
inline int on(const char* name, X4NativeEventCallback callback,
              void* userdata = nullptr) {
    return detail::g_api->subscribe(name, callback, userdata, detail::g_api);
}

/// Unsubscribe by subscription ID (returned by on()).
inline void off(int subscription_id) {
    detail::g_api->unsubscribe(subscription_id);
}

/// Raise a C++ event (dispatched to all subscribers).
inline void raise(const char* name, void* data = nullptr) {
    detail::g_api->raise_event(name, data);
}

/// Raise a Lua event (C++ -> Lua bridge via CallEventScripts).
/// Must be called from UI thread. Returns 0 on success.
inline int raise_lua(const char* name, const char* param = nullptr) {
    return detail::g_api->raise_lua_event(name, param);
}

/// Register a dynamic Lua->C++ event bridge.
/// When the Lua event fires, the framework will raise the C++ event.
/// Returns 0 on success. Must be called from UI thread (during init is fine).
inline int bridge_lua_event(const char* lua_event, const char* cpp_event) {
    return detail::g_api->register_lua_bridge(lua_event, cpp_event);
}

} // namespace x4n

