// ---------------------------------------------------------------------------
// x4native_core.dll — Core Entry Point
//
// This is the hot-reloadable core. All framework logic lives here:
// logger, event system, version detection, extension management (Phase 2+).
//
// The proxy DLL loads this via copy-on-load and calls core_init() to fill
// the dispatch table. On hot-reload, the proxy FreeLibrary's the old core,
// copies the new build, LoadLibrary's it, and calls core_init() again.
// ---------------------------------------------------------------------------

#include "logger.h"
#include "event_system.h"
#include "extension_manager.h"
#include "game_api.h"
#include "hook_manager.h"
#include "settings_manager.h"
#include "version.h"
#include "x4native_defs.h"

#include <x4_game_func_table.h>
#include <x4_game_offsets.h>
#include <x4_manual_types.h>
#include <MinHook.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>
#include <string>
#include <cmath>

// ---------------------------------------------------------------------------
// Module-level state
// ---------------------------------------------------------------------------
// Runtime-resolved game offsets (extern'd by extension_manager.cpp).
// Integer fields are statically initialized from #defines.
// Pointer fields (needing exe_base) are filled by populate_offsets() at startup.
X4GameOffsets s_offsets = {
    // --- Pre-resolved pointers: nullptr until populate_offsets() ---
    .frame_game_time       = nullptr,
    .frame_raw_time        = nullptr,
    .frame_real_time       = nullptr,
    .frame_speed_mult      = nullptr,
    .component_registry    = nullptr,
    .game_universe         = nullptr,
    .session_seed          = nullptr,
    .construction_plan_db  = nullptr,
    .macro_registry        = nullptr,
    .radar_event_vtable    = nullptr,

    // --- Vtable slot indices (pre-divided by 8) ---
    .vtable_get_class_type   = X4_VTABLE_GET_CLASS_TYPE / 8,
    .vtable_get_class_id     = X4_VTABLE_GET_CLASS_ID / 8,
    .vtable_is_class_id      = X4_VTABLE_IS_CLASS_ID / 8,
    .vtable_is_derived_class = X4_VTABLE_IS_DERIVED_CLASS / 8,
    .vtable_get_id_code      = X4_VTABLE_GET_ID_CODE / 8,
    .vtable_set_world_xform  = X4_VTABLE_SET_WORLD_XFORM / 8,
    .vtable_set_position     = X4_VTABLE_SET_POSITION / 8,
    .vtable_destroy          = X4_VTABLE_DESTROY / 8,
    .vtable_get_faction_id   = X4_VTABLE_GET_FACTION_ID / 8,

    // --- Engine context offsets ---
    .enginectx_frame_counter = X4_ENGINECTX_OFFSET_FRAME_COUNTER,
    .enginectx_fps_timer     = X4_ENGINECTX_OFFSET_FPS_TIMER,
    .enginectx_fps           = X4_ENGINECTX_OFFSET_FPS,

    // --- Component struct offsets ---
    .component_id            = X4_COMPONENT_OFFSET_ID,
    .component_definition    = X4_COMPONENT_OFFSET_DEFINITION,
    .component_parent        = X4_COMPONENT_OFFSET_PARENT,
    .component_children      = X4_COMPONENT_OFFSET_CHILDREN,
    .component_exists        = X4_COMPONENT_OFFSET_EXISTS,
    .component_combined_seed = X4_COMPONENT_OFFSET_COMBINED_SEED,

    // --- Container / Space / Universe ---
    .container_spawntime         = X4_CONTAINER_OFFSET_SPAWNTIME,
    .space_has_sunlight          = X4_SPACE_OFFSET_HAS_SUNLIGHT,
    .space_sunlight              = X4_SPACE_OFFSET_SUNLIGHT,
    .game_universe_galaxy_offset = X4_GAME_UNIVERSE_GALAXY_OFFSET,

    // --- Object-class visibility ---
    .object_owner_faction_ptr    = X4_OBJECT_OFFSET_OWNER_FACTION_PTR,
    .object_known_read           = X4_OBJECT_OFFSET_KNOWN_READ,
    .object_known_to_all         = X4_OBJECT_OFFSET_KNOWN_TO_ALL,
    .object_known_factions_arr   = X4_OBJECT_OFFSET_KNOWN_FACTIONS_ARR,
    .object_known_factions_cap   = X4_OBJECT_OFFSET_KNOWN_FACTIONS_CAP,
    .object_known_factions_count = X4_OBJECT_OFFSET_KNOWN_FACTIONS_COUNT,
    .object_liveview_local       = X4_OBJECT_OFFSET_LIVEVIEW_LOCAL,
    .object_liveview_monitor     = X4_OBJECT_OFFSET_LIVEVIEW_MONITOR,
    .object_masstraffic_queue    = X4_OBJECT_OFFSET_MASSTRAFFIC_QUEUE,
    .object_radar_visible        = X4_OBJECT_OFFSET_RADAR_VISIBLE,
    .object_forced_radar_visible = X4_OBJECT_OFFSET_FORCED_RADAR_VISIBLE,

    // --- Space-class visibility ---
    .space_owner_faction_ptr     = X4_SPACE_OFFSET_OWNER_FACTION_PTR,
    .space_known_read            = X4_SPACE_OFFSET_KNOWN_READ,
    .space_known_to_all          = X4_SPACE_OFFSET_KNOWN_TO_ALL,
    .space_known_factions_arr    = X4_SPACE_OFFSET_KNOWN_FACTIONS_ARR,
    .space_known_factions_cap    = X4_SPACE_OFFSET_KNOWN_FACTIONS_CAP,
    .space_known_factions_count  = X4_SPACE_OFFSET_KNOWN_FACTIONS_COUNT,

    // --- Sector resource areas ---
    .sector_resarea_vec_begin    = X4_SECTOR_RESAREA_VEC_BEGIN,
    .sector_resarea_vec_end      = X4_SECTOR_RESAREA_VEC_END,

    // --- MacroData / ConnectionEntry ---
    .macrodata_connections_begin   = X4_MACRODATA_OFFSET_CONNECTIONS_BEGIN,
    .macrodata_connections_end     = X4_MACRODATA_OFFSET_CONNECTIONS_END,
    .connection_entry_size         = X4_CONNECTION_ENTRY_SIZE,
    .connection_offset_hash        = X4_CONNECTION_OFFSET_HASH,
    .connection_offset_name        = X4_CONNECTION_OFFSET_NAME,
    .macrodefaults_room_conn_begin = X4_MACRODEFAULTS_OFFSET_ROOM_CONNECTIONS_BEGIN,
    .macrodefaults_room_conn_end   = X4_MACRODEFAULTS_OFFSET_ROOM_CONNECTIONS_END,

    // --- Radar event layout ---
    .radar_event_entity_id = X4_RADAR_EVENT_OFFSET_ENTITY_ID,
    .radar_event_visible   = X4_RADAR_EVENT_OFFSET_VISIBLE,

    // --- Room offsets ---
    .room_roomtype   = X4_ROOM_OFFSET_ROOMTYPE,
    .room_name       = X4_ROOM_OFFSET_NAME,
    .room_private    = X4_ROOM_OFFSET_PRIVATE,
    .room_persistent = X4_ROOM_OFFSET_PERSISTENT,
};

static void*       g_lua       = nullptr;   // lua_State* (opaque here)
static std::string g_ext_root;
static std::string g_game_version;
static std::string g_version_string;        // cached for get_version()
static raise_lua_event_fn g_raise_lua_event = nullptr;  // proxy-provided Lua bridge
static register_lua_bridge_fn g_register_lua_bridge = nullptr;  // proxy-provided Lua→C++ bridge

// Stash — proxy-owned in-memory key-value, forwarded from CoreInitContext
static stash_set_fn    g_stash_set    = nullptr;
static stash_get_fn    g_stash_get    = nullptr;
static stash_remove_fn g_stash_remove = nullptr;
static stash_clear_fn  g_stash_clear  = nullptr;

// ---------------------------------------------------------------------------
// Resolve pointer fields in s_offsets (requires exe_base, called once at startup)
// ---------------------------------------------------------------------------
static void resolve_offset_pointers(uintptr_t base) {
    s_offsets.frame_game_time      = reinterpret_cast<double*>(base + X4_RVA_FRAME_GAME_TIME);
    s_offsets.frame_raw_time       = reinterpret_cast<double*>(base + X4_RVA_FRAME_RAW_TIME);
    s_offsets.frame_real_time      = reinterpret_cast<double*>(base + X4_RVA_FRAME_REAL_TIME);
    s_offsets.frame_speed_mult     = reinterpret_cast<double*>(base + X4_RVA_FRAME_SPEED_MULT);
    s_offsets.component_registry   = reinterpret_cast<void*>(base + X4_RVA_COMPONENT_REGISTRY);
    s_offsets.game_universe        = reinterpret_cast<void*>(base + X4_RVA_GAME_UNIVERSE);
    s_offsets.session_seed         = reinterpret_cast<uint64_t*>(base + X4_RVA_SESSION_SEED);
    s_offsets.construction_plan_db = reinterpret_cast<void*>(base + X4_RVA_CONSTRUCTION_PLAN_DB);
    s_offsets.macro_registry       = reinterpret_cast<void*>(base + X4_RVA_MACRO_REGISTRY);
    s_offsets.radar_event_vtable   = reinterpret_cast<void*>(base + X4_RADAR_EVENT_VTABLE_RVA);
}

// ---------------------------------------------------------------------------
// Native frame tick hook — fires on_native_frame_update to extensions
// ---------------------------------------------------------------------------
// Direct MinHook detour on the engine's internal frame tick function.
// Reads timing data from known game globals (verified v9.00 build 900).
// See docs/rev/GAME_LOOP.md for reverse engineering notes.

static uintptr_t g_x4_base = 0;
static void*     g_frame_tick_trampoline = nullptr;

using FrameTickFn = void(__fastcall*)(void*, bool);

static void __fastcall frame_tick_detour(void* engineCtx, bool isSuspended) {
    // Snapshot raw_time before the original runs
    double raw_time_before = *s_offsets.frame_raw_time;

    // Call original frame tick
    reinterpret_cast<FrameTickFn>(g_frame_tick_trampoline)(engineCtx, isSuspended);

    // Compute delta from the engine's own raw time accumulation
    double raw_time_after = *s_offsets.frame_raw_time;
    double delta = raw_time_after - raw_time_before;
    if (delta < 0.0) delta = 0.0;

    // Build event payload
    X4NativeFrameUpdate update{};
    update.delta            = delta;
    update.game_time        = *s_offsets.frame_game_time;
    update.real_time        = *s_offsets.frame_real_time;
    update.fps              = *(float*)((uintptr_t)engineCtx + s_offsets.enginectx_fps);
    update.speed_multiplier = (float)*s_offsets.frame_speed_mult;
    update.is_suspended     = isSuspended;
    update.frame_counter    = *(int*)((uintptr_t)engineCtx + s_offsets.enginectx_frame_counter);

    auto* table = x4n::GameAPI::table();
    update.game_paused = (table && table->IsGamePaused) ? table->IsGamePaused() : false;

    // Check for extension DLL changes and reload any that have autoreload enabled.
    // Must happen before firing the event so no extension code is on the stack.
    x4n::ExtensionManager::tick();
    x4n::ExtensionManager::flush_pending_reloads();

    x4n::EventSystem::fire("on_native_frame_update", &update);
}

static bool install_frame_tick_hook() {
    void* target = x4n::GameAPI::get_internal("X4_FrameTick");
    if (!target) {
        x4n::Logger::warn("Native frame hook: X4_FrameTick not resolved (missing RVA for this build?)");
        return false;
    }

    g_x4_base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    if (!g_x4_base) return false;

    MH_STATUS status = MH_CreateHook(target, &frame_tick_detour, &g_frame_tick_trampoline);
    if (status != MH_OK) {
        x4n::Logger::error("Native frame hook: MH_CreateHook failed: {}", MH_StatusToString(status));
        return false;
    }

    status = MH_EnableHook(target);
    if (status != MH_OK) {
        x4n::Logger::error("Native frame hook: MH_EnableHook failed: {}", MH_StatusToString(status));
        MH_RemoveHook(target);
        return false;
    }

    x4n::Logger::info("Native frame hook installed (on_native_frame_update)");
    return true;
}

static void remove_frame_tick_hook() {
    void* target = x4n::GameAPI::get_internal("X4_FrameTick");
    if (target && g_frame_tick_trampoline) {
        MH_DisableHook(target);
        MH_RemoveHook(target);
        g_frame_tick_trampoline = nullptr;
    }
}

// ---------------------------------------------------------------------------
// [OBSOLETE] Radar visibility hook — fires on_radar_changed to extensions
// Superseded by MD event hook: use x4n::md::on_radar_visibility_changed_before()
// Kept for backward compatibility. Will be removed in a future version.
// ---------------------------------------------------------------------------
// Direct MinHook detour on the engine's RadarVisibilityChanged_BuildEvent.
// Called from the sector update property change handler (case 378) when an
// entity enters or leaves radar range. See docs/rev/VISIBILITY.md Section 3.
// The original function creates a RadarVisibilityChangedEvent object; we
// read entity_id and visible from it after it returns.

static void* g_radar_event_trampoline = nullptr;

using RadarEventBuildFn = void*(__fastcall*)(void*);

static void* __fastcall radar_event_detour(void* property_data) {
    // Call original — creates the event object
    void* event = reinterpret_cast<RadarEventBuildFn>(g_radar_event_trampoline)(property_data);
    if (!event) return event;

    // Read event payload
    auto addr = reinterpret_cast<uintptr_t>(event);
    X4RadarChangedEvent payload{};
    payload.entity_id = *reinterpret_cast<uint64_t*>(addr + s_offsets.radar_event_entity_id);
    payload.visible   = *reinterpret_cast<uint8_t*>(addr + s_offsets.radar_event_visible);

    x4n::EventSystem::fire("on_radar_changed", &payload);
    return event;
}

static bool install_radar_visibility_hook() {
    void* target = x4n::GameAPI::get_internal("RadarVisibilityChanged_BuildEvent");
    if (!target) {
        x4n::Logger::warn("Radar visibility hook: RadarVisibilityChanged_BuildEvent not resolved (missing RVA for this build?)");
        return false;
    }

    MH_STATUS status = MH_CreateHook(target, &radar_event_detour, &g_radar_event_trampoline);
    if (status != MH_OK) {
        x4n::Logger::error("Radar visibility hook: MH_CreateHook failed: {}", MH_StatusToString(status));
        return false;
    }

    status = MH_EnableHook(target);
    if (status != MH_OK) {
        x4n::Logger::error("Radar visibility hook: MH_EnableHook failed: {}", MH_StatusToString(status));
        MH_RemoveHook(target);
        return false;
    }

    x4n::Logger::info("Radar visibility hook installed (on_radar_changed)");
    return true;
}

static void remove_radar_visibility_hook() {
    void* target = x4n::GameAPI::get_internal("RadarVisibilityChanged_BuildEvent");
    if (target && g_radar_event_trampoline) {
        MH_DisableHook(target);
        MH_RemoveHook(target);
        g_radar_event_trampoline = nullptr;
    }
}

// ---------------------------------------------------------------------------
// MD event dispatch hook — fires on_md_before / on_md_after per type_id
// ---------------------------------------------------------------------------
// MinHook detour on EventQueue_InsertOrDispatch. O(1) dispatch via flat
// arrays indexed by type_id. Extensions subscribe via on_md_before/after().

static void* g_md_event_trampoline = nullptr;

static void __fastcall md_event_dispatch_detour(
    void* event_source, void* event_object, double timestamp, char immediate)
{
    uint32_t type_id = UINT32_MAX;

    if (event_object) {
        // Read type ID from vtable[1]: always `B8 imm32 C3` (mov eax, imm32; ret)
        __try {
            auto vtable = *reinterpret_cast<void***>(event_object);
            auto fn_bytes = reinterpret_cast<uint8_t*>(vtable[1]);
            if (fn_bytes[0] == 0xB8 && fn_bytes[5] == 0xC3) {
                type_id = *reinterpret_cast<uint32_t*>(fn_bytes + 1);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            type_id = UINT32_MAX;
        }
    }

    // Fire before-subscribers
    if (type_id < x4n::EventSystem::MAX_MD_TYPE) {
        uint64_t source_id = event_source ?
            *reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(event_source) + 8) : 0;
        X4MdEvent payload{type_id, source_id, timestamp, event_object};
        x4n::EventSystem::md_fire_before(type_id, &payload);
    }

    // Original dispatch — MD cue listeners fire here
    using OrigFn = void(__fastcall*)(void*, void*, double, char);
    reinterpret_cast<OrigFn>(g_md_event_trampoline)(
        event_source, event_object, timestamp, immediate);

    // Fire after-subscribers
    if (type_id < x4n::EventSystem::MAX_MD_TYPE) {
        uint64_t source_id = event_source ?
            *reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(event_source) + 8) : 0;
        X4MdEvent payload{type_id, source_id, timestamp, event_object};
        x4n::EventSystem::md_fire_after(type_id, &payload);
    }
}

static bool install_md_event_hook() {
    auto* table = x4n::GameAPI::table();
    void* target = table ? reinterpret_cast<void*>(table->EventQueue_InsertOrDispatch) : nullptr;
    if (!target) {
        x4n::Logger::warn("MD event hook: EventQueue_InsertOrDispatch not resolved (missing RVA for this build?)");
        return false;
    }

    MH_STATUS status = MH_CreateHook(target, &md_event_dispatch_detour, &g_md_event_trampoline);
    if (status != MH_OK) {
        x4n::Logger::error("MD event hook: MH_CreateHook failed: {}", MH_StatusToString(status));
        return false;
    }

    status = MH_EnableHook(target);
    if (status != MH_OK) {
        x4n::Logger::error("MD event hook: MH_EnableHook failed: {}", MH_StatusToString(status));
        MH_RemoveHook(target);
        return false;
    }

    x4n::Logger::info("MD event hook installed (on_md_before/on_md_after, {} type slots)",
                      x4n::EventSystem::MAX_MD_TYPE);
    return true;
}

static void remove_md_event_hook() {
    auto* table = x4n::GameAPI::table();
    void* target = table ? reinterpret_cast<void*>(table->EventQueue_InsertOrDispatch) : nullptr;
    if (target && g_md_event_trampoline) {
        MH_DisableHook(target);
        MH_RemoveHook(target);
        g_md_event_trampoline = nullptr;
    }
    // MD subscription cleanup handled by EventSystem::init() on next startup
}

// ---------------------------------------------------------------------------
// Dispatch implementations (called by proxy via function pointers)
// ---------------------------------------------------------------------------

static void impl_discover_extensions() {
    x4n::ExtensionManager::discover();
    x4n::ExtensionManager::load_all();
}

static void impl_raise_event(const char* event_name, const char* param) {
    x4n::EventSystem::fire(event_name, const_cast<char*>(param));
}

static const char* impl_get_version() {
    return g_version_string.c_str();
}

static const char* impl_get_loaded_extensions() {
    static std::string cached;
    cached = x4n::ExtensionManager::loaded_extensions_json();
    return cached.c_str();
}

static void impl_set_lua_state(void* L) {
    g_lua = L;
    x4n::Logger::info("Lua state updated (UI reload)");
}

static void impl_log(int level, const char* message) {
    auto lv = static_cast<x4n::LogLevel>(level);
    x4n::Logger::write(lv, message);
}

static void impl_prepare_reload() {
    x4n::Logger::info("Preparing for core hot-reload...");
    x4n::EventSystem::fire("on_before_reload");
    x4n::ExtensionManager::shutdown();
    remove_md_event_hook();
    remove_radar_visibility_hook();
    remove_frame_tick_hook();
    x4n::HookManager::remove_all();
}

static void impl_shutdown() {
    x4n::Logger::info("Core shutting down...");
    x4n::ExtensionManager::shutdown();
    x4n::SettingsManager::shutdown();
    remove_md_event_hook();
    remove_radar_visibility_hook();
    remove_frame_tick_hook();
    x4n::HookManager::shutdown();
    x4n::GameAPI::shutdown();
    x4n::EventSystem::shutdown();
    x4n::Logger::shutdown();
}

static int impl_enumerate_settings(const char* ext_id, const SettingInfo** out) {
    if (!ext_id) { if (out) *out = nullptr; return 0; }
    return x4n::SettingsManager::enumerate(ext_id, out);
}

static void impl_set_extension_setting(const char* ext_id, const char* key,
                                       const SettingValueC* value) {
    if (!ext_id || !key || !value) return;
    x4n::SettingsManager::set_from_abi(ext_id, key, *value);
}

// ---------------------------------------------------------------------------
// Exported functions (called by proxy DLL)
// ---------------------------------------------------------------------------

extern "C" __declspec(dllexport)
int core_init(CoreInitContext* ctx) {
    g_lua      = ctx->lua_state;
    g_ext_root = ctx->ext_root;

    // 1. Logger — two-phase init. Start buffering in memory; the file is
    //    opened in step 5 once we have GameAPI (needed to resolve the
    //    user's X4 profile path). Early log lines are replayed on open.
    x4n::Logger::init(g_ext_root);
    x4n::Logger::info("X4Native core v" X4_GAME_VERSION_LABEL " initializing...");
    x4n::Logger::info("Extension root: {}", g_ext_root);

    // 2. Event system
    x4n::EventSystem::init();

    // 3. Game version
    g_game_version  = x4n::Version::detect();
    g_version_string = std::string(X4_GAME_VERSION_LABEL) +
                       " (game: " + g_game_version + ")";

    // 4. Game API — resolve X4.exe function pointers
    x4n::GameAPI::init();
    x4n::GameAPI::load_internal_db(g_ext_root, X4_GAME_VERSION_LABEL,
                                    std::to_string(X4_GAME_TYPES_BUILD));

    // 5. Logger — open file sink in <profile>\x4native\x4native.log and
    //    flush the buffer produced by steps 1-4.
    x4n::Logger::open_files();

    // 4b. Resolve pointer fields in game offsets (used by extensions and core hooks)
    resolve_offset_pointers(x4n::GameAPI::exe_base());

    // 5. Hook manager — MinHook initialization
    x4n::HookManager::init();

    // 5b. Native frame tick hook (core-owned, fires on_native_frame_update)
    install_frame_tick_hook();

    // 5c. [OBSOLETE] Radar visibility hook — superseded by MD event hook (type_id 376)
    install_radar_visibility_hook();

    // 5d. MD event dispatch hook (core-owned, fires on_md_before/on_md_after)
    install_md_event_hook();

    // 6. Extension manager
    g_raise_lua_event = ctx->raise_lua_event;
    g_register_lua_bridge = ctx->register_lua_bridge;
    g_stash_set    = ctx->stash_set;
    g_stash_get    = ctx->stash_get;
    g_stash_remove = ctx->stash_remove;
    g_stash_clear  = ctx->stash_clear;
    x4n::ExtensionManager::init(g_ext_root, g_game_version,
                                g_raise_lua_event, g_register_lua_bridge,
                                g_stash_set, g_stash_get,
                                g_stash_remove, g_stash_clear);

    // 6. Fill the proxy's dispatch table
    ctx->dispatch->discover_extensions    = impl_discover_extensions;
    ctx->dispatch->raise_event            = impl_raise_event;
    ctx->dispatch->get_version            = impl_get_version;
    ctx->dispatch->get_loaded_extensions  = impl_get_loaded_extensions;
    ctx->dispatch->set_lua_state          = impl_set_lua_state;
    ctx->dispatch->prepare_reload         = impl_prepare_reload;
    ctx->dispatch->shutdown               = impl_shutdown;
    ctx->dispatch->log                    = impl_log;
    ctx->dispatch->enumerate_settings     = impl_enumerate_settings;
    ctx->dispatch->set_extension_setting  = impl_set_extension_setting;

    x4n::Logger::info("Core initialized successfully");
    return 0;
}

extern "C" __declspec(dllexport)
void core_shutdown() {
    impl_shutdown();
}

// ---------------------------------------------------------------------------
// DllMain — intentionally minimal
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) {
    return TRUE;
}
