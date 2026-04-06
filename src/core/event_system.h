#pragma once
// ---------------------------------------------------------------------------
// Event System — Publish / Subscribe
//
// Thread-safe event bus. Extensions subscribe during x4native_init();
// events are dispatched by the core DLL or forwarded from Lua.
// ---------------------------------------------------------------------------

#include <string>
#include <functional>
#include <unordered_map>
#include <array>
#include <vector>
#include <mutex>

namespace x4n {

using EventCallback = void(*)(const char* event_name, void* data, void* userdata);

class EventSystem {
public:
    static void init();
    static void shutdown();

    // Named event pub/sub
    static int subscribe(const char* event_name, EventCallback callback, void* userdata = nullptr);
    static void unsubscribe(int id);
    static void fire(const char* event_name, void* data = nullptr);

    // MD event pub/sub (O(1) by type_id)
    static constexpr uint32_t MAX_MD_TYPE = 600;
    static int  md_subscribe_before(uint32_t type_id, EventCallback callback, void* userdata = nullptr);
    static int  md_subscribe_after(uint32_t type_id, EventCallback callback, void* userdata = nullptr);
    static void md_fire_before(uint32_t type_id, void* payload);  // called by hook detour
    static void md_fire_after(uint32_t type_id, void* payload);


    struct Subscription {
        int            id;
        EventCallback  callback;
        void*          userdata;
    };

    static std::unordered_map<std::string, std::vector<Subscription>> s_subscribers;
    static std::array<std::vector<Subscription>, MAX_MD_TYPE> s_md_before;
    static std::array<std::vector<Subscription>, MAX_MD_TYPE> s_md_after;
    static std::mutex s_mutex;
    static int        s_next_id;
};

} // namespace x4n
