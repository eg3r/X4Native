#include "event_system.h"
#include "logger.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>

namespace x4n {

// SEH wrapper — must be a separate function (no C++ objects needing unwind).
// Mirrors seh_call_hook in hook_manager.cpp: a faulting subscriber must not
// take down the game from the event dispatch path (which includes the
// per-frame on_native_frame_update and the engine's MD dispatch hot path).
static int seh_call_event(EventCallback cb, const char* event_name,
                          void* data, void* userdata) {
    __try {
        cb(event_name, data, userdata);
        return 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

std::unordered_map<std::string, std::vector<EventSystem::Subscription>>
    EventSystem::s_subscribers;
std::array<std::vector<EventSystem::Subscription>, EventSystem::MAX_MD_TYPE>
    EventSystem::s_md_before;
std::array<std::vector<EventSystem::Subscription>, EventSystem::MAX_MD_TYPE>
    EventSystem::s_md_after;
std::mutex EventSystem::s_mutex;
int        EventSystem::s_next_id = 1;

void EventSystem::init() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_subscribers.clear();
    for (auto& v : s_md_before) v.clear();
    for (auto& v : s_md_after) v.clear();
    s_next_id = 1;
}

void EventSystem::shutdown() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_subscribers.clear();
    for (auto& v : s_md_before) v.clear();
    for (auto& v : s_md_after) v.clear();
}

int EventSystem::subscribe(const char* event_name,
                           EventCallback callback,
                           void* userdata) {
    std::lock_guard<std::mutex> lock(s_mutex);
    int id = s_next_id++;
    s_subscribers[event_name].push_back({id, callback, userdata});
    Logger::debug("Event '{}': subscribed (id={})", event_name, id);
    return id;
}

void EventSystem::unsubscribe(int id) {
    std::lock_guard<std::mutex> lock(s_mutex);
    // Check named events
    for (auto& [name, subs] : s_subscribers) {
        subs.erase(
            std::remove_if(subs.begin(), subs.end(),
                [id](const Subscription& s) { return s.id == id; }),
            subs.end());
    }
    // Check MD event arrays
    auto remove_from = [id](auto& arr) {
        for (auto& v : arr) {
            v.erase(std::remove_if(v.begin(), v.end(),
                [id](const Subscription& s) { return s.id == id; }), v.end());
        }
    };
    remove_from(s_md_before);
    remove_from(s_md_after);
}

int EventSystem::md_subscribe_before(uint32_t type_id, EventCallback callback, void* userdata) {
    if (type_id >= MAX_MD_TYPE || !callback) return -1;
    std::lock_guard<std::mutex> lock(s_mutex);
    int id = s_next_id++;
    s_md_before[type_id].push_back({id, callback, userdata});
    Logger::debug("MD event {}: subscribed before (id={})", type_id, id);
    return id;
}

int EventSystem::md_subscribe_after(uint32_t type_id, EventCallback callback, void* userdata) {
    if (type_id >= MAX_MD_TYPE || !callback) return -1;
    std::lock_guard<std::mutex> lock(s_mutex);
    int id = s_next_id++;
    s_md_after[type_id].push_back({id, callback, userdata});
    Logger::debug("MD event {}: subscribed after (id={})", type_id, id);
    return id;
}

void EventSystem::md_fire_before(uint32_t type_id, void* payload) {
    if (type_id >= MAX_MD_TYPE) return;
    std::vector<Subscription> snapshot;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto& subs = s_md_before[type_id];
        if (subs.empty()) return;
        snapshot = subs;
    }
    for (auto& sub : snapshot) {
        if (seh_call_event(sub.callback, nullptr, payload, sub.userdata) != 0) {
            Logger::error("MD event {}: before-subscriber #{} crashed (SEH) — removed",
                          type_id, sub.id);
            unsubscribe(sub.id);
        }
    }
}

void EventSystem::md_fire_after(uint32_t type_id, void* payload) {
    if (type_id >= MAX_MD_TYPE) return;
    std::vector<Subscription> snapshot;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto& subs = s_md_after[type_id];
        if (subs.empty()) return;
        snapshot = subs;
    }
    for (auto& sub : snapshot) {
        if (seh_call_event(sub.callback, nullptr, payload, sub.userdata) != 0) {
            Logger::error("MD event {}: after-subscriber #{} crashed (SEH) — removed",
                          type_id, sub.id);
            unsubscribe(sub.id);
        }
    }
}

void EventSystem::fire(const char* event_name, void* data) {
    // Copy subscriber list before dispatching so callbacks can safely
    // subscribe/unsubscribe without deadlocking.
    std::vector<Subscription> snapshot;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_subscribers.find(event_name);
        if (it == s_subscribers.end() || it->second.empty())
            return;
        snapshot = it->second;
    }

    for (auto& sub : snapshot) {
        if (seh_call_event(sub.callback, event_name, data, sub.userdata) != 0) {
            Logger::error("Event '{}': subscriber #{} crashed (SEH) — removed",
                          event_name, sub.id);
            unsubscribe(sub.id);
        }
    }
}

void EventSystem::fire_to(const char* event_name, const std::vector<int>& ids, void* data) {
    std::vector<Subscription> snapshot;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_subscribers.find(event_name);
        if (it == s_subscribers.end())
            return;
        for (const auto& sub : it->second)
            if (std::find(ids.begin(), ids.end(), sub.id) != ids.end())
                snapshot.push_back(sub);
    }

    for (auto& sub : snapshot) {
        if (seh_call_event(sub.callback, event_name, data, sub.userdata) != 0) {
            Logger::error("Event '{}': subscriber #{} crashed (SEH) — removed",
                          event_name, sub.id);
            unsubscribe(sub.id);
        }
    }
}

} // namespace x4n
