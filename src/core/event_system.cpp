#include "event_system.h"
#include "logger.h"

#include <algorithm>

namespace x4n {

std::unordered_map<std::string, std::vector<EventSystem::Subscription>>
    EventSystem::s_subscribers;
std::mutex EventSystem::s_mutex;
int        EventSystem::s_next_id = 1;

void EventSystem::init() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_subscribers.clear();
    s_next_id = 1;
}

void EventSystem::shutdown() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_subscribers.clear();
}

int EventSystem::subscribe(const std::string& event_name,
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
    for (auto& [name, subs] : s_subscribers) {
        subs.erase(
            std::remove_if(subs.begin(), subs.end(),
                [id](const Subscription& s) { return s.id == id; }),
            subs.end());
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
        sub.callback(event_name, data, sub.userdata);
    }
}

} // namespace x4n
