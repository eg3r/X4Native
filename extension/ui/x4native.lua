-- X4Native
-- Lua module loaded via ui.xml (v7.5+)
-- Note: Players must disable Protected UI Mode in Settings > Extensions.
--
-- Multi-stage lifecycle:
--   Stage 1 (game start):    DLL loads immediately, extensions register via x4native_init()
--   Stage 2a (save loaded):  MD cue fires raise_lua_event → Lua forwards "on_game_loaded"
--   Stage 2b (world ready):  MD cue fires raise_lua_event → Lua forwards "on_game_started"
--   Stage 3 (runtime):       MD signals game_saved; per-frame via SetScript("onUpdate")
--
-- DLL lifecycle (two-DLL architecture):
--   package.loadlib() loads x4native_64.dll (thin proxy, ~100 lines).
--   The proxy copies x4native_core.dll → x4native_core_live.dll and loads
--   the copy — so the on-disk core is never file-locked by Windows.
--   /reloadui destroys the Lua state, but the proxy stays loaded (file locked
--   = irrelevant, it rarely changes). The proxy detects if x4native_core.dll
--   is newer, unloads the old core, copies the new one, and reloads it.
--   Result: new native code running without restarting X4.
--
-- Event wiring:
--   MD → raise_lua_event("x4native.game_loaded")  → RegisterEvent → DLL raise_event("on_game_loaded")
--   MD → raise_lua_event("x4native.game_started") → RegisterEvent → DLL raise_event("on_game_started")
--   MD → raise_lua_event("x4native.game_saved")   → RegisterEvent → DLL raise_event("on_game_save")
--   Per-frame → SetScript("onUpdate") → DLL raise_event("on_frame_update")
--   UI reload → Lua file re-executes, DLL receives on_ui_reload + fresh lua_State*

-- FFI: access to game C functions (IsPlayerValid, GetPlayerID, etc.)
local ffi = require("ffi")
local C = ffi.C
-- pcall to tolerate redefinition if another addon already declared these
pcall(ffi.cdef, [[
    typedef uint64_t UniverseID;
    bool IsPlayerValid(void);
    UniverseID GetPlayerID(void);
]])

local x4native_api = nil

-- ============================================================
-- Table-driven Lua→C++ event bridge
-- Maps a Lua event name (from RegisterEvent) to a C++ event name.
-- ============================================================
local forwarded_events = {}

local function bridge_forward_lua_event(lua_event_name, cpp_event_name)
    if forwarded_events[lua_event_name] then return end
    local handler = function(_, arg)
        if x4native_api then
            x4native_api.log(1, "Lua bridge: received '" .. lua_event_name .. "' -> forwarding as '" .. (cpp_event_name or lua_event_name) .. "'")
            x4native_api.raise_event(cpp_event_name or lua_event_name, arg ~= nil and tostring(arg) or nil)
        else
            DebugError("X4Native: bridge received '" .. lua_event_name .. "' but x4native_api is nil")
        end
    end
    RegisterEvent(lua_event_name, handler)
    forwarded_events[lua_event_name] = handler
end

-- ============================================================
-- Stage 1: Load the core DLL immediately at game start.
-- Extensions get x4native_init() here — they register events
-- but must NOT call game functions yet.
-- ============================================================
local function load_dll()
    if x4native_api then return true end

    -- Must be Windows with protected mode disabled
    if package == nil
       or package.config:sub(1,1) ~= "\\"
       or GetUISafeModeOption() ~= false then
        DebugError("X4Native: Requires Windows with Protected UI Mode disabled.")
        return false
    end

    local dll_path = ".\\extensions\\x4native\\native\\x4native_64.dll"
    local loader = package.loadlib(dll_path, "luaopen_x4native")
    if not loader then
        DebugError("X4Native: Failed to locate DLL at " .. dll_path)
        return false
    end

    local success, result = pcall(loader)
    if not success then
        DebugError("X4Native: DLL init failed: " .. tostring(result))
        return false
    end

    x4native_api = result

    -- Discover and load extension DLLs (calls x4native_init on each)
    x4native_api.discover_extensions()
    return true
end

-- Load DLL right away (game start)
load_dll()

-- ============================================================
-- Stage 2: MD cue detects savegame loaded, signals Lua via
-- raise_lua_event("x4native.game_loaded").
-- After this, extensions can safely call game functions.
-- ============================================================
bridge_forward_lua_event("x4native.game_loaded",  "on_game_loaded")
bridge_forward_lua_event("x4native.game_started", "on_game_started")
bridge_forward_lua_event("x4native.game_saved",   "on_game_save")

-- Round-trip echo: C++ sends ping via raise_lua_event, Lua forwards back to C++
bridge_forward_lua_event("x4native_event_test.ping", "on_event_test_pong")

-- ============================================================
-- Stage 3: Runtime events
-- ============================================================

-- UI reload: When /reloadui runs, X4 destroys the Lua state and
-- re-executes all Lua files. The DLL remains loaded in the process
-- (C++ state survives). load_dll() calls luaopen_x4native() again,
-- giving the DLL the new lua_State*. Extensions receive on_ui_reload
-- so they can re-register any Lua-side bindings.
if x4native_api then
    x4native_api.raise_event("on_ui_reload")
    x4native_api.log(1, "UI reloaded — DLL re-initialized with fresh Lua state")

    -- Detect when a game world becomes active.
    -- C.IsPlayerValid() returns true once a savegame is fully loaded.
    -- Three scenarios:
    --   1. /reloadui while save is loaded → IsPlayerValid() = true → fire immediately
    --   2. Game just started, no save yet → poll via onUpdate until it becomes true
    --   3. MD cue fires first → guard prevents double-fire
    local game_loaded_fired = false

    local function fire_game_loaded(source)
        if game_loaded_fired then return end
        game_loaded_fired = true
        x4native_api.log(1, "on_game_loaded triggered via " .. source)
        x4native_api.raise_event("on_game_loaded")
    end

    -- Override the bridge handler for game_loaded to use the guard
    local orig_handler = forwarded_events["x4native.game_loaded"]
    if orig_handler then
        UnregisterEvent("x4native.game_loaded", orig_handler)
        local guarded_handler = function(_, arg)
            if x4native_api then
                fire_game_loaded("MD cue")
            end
        end
        RegisterEvent("x4native.game_loaded", guarded_handler)
        forwarded_events["x4native.game_loaded"] = guarded_handler
    end

    -- Named function so RemoveScript can find it by reference
    local poll_count = 0
    local function poll_for_player()
        poll_count = poll_count + 1
        local ok_p, pid = pcall(C.GetPlayerID)
        -- Check GetPlayerID ~= 0 (more reliable than IsPlayerValid)
        if ok_p and pid and tonumber(pid) ~= 0 then
            fire_game_loaded(string.format("poll frame %d (pid=%s)", poll_count, tostring(pid)))
        end
    end

    -- Per-frame handler: fires on_frame_update and handles game_loaded polling
    -- Debug builds: also checks if core DLL was modified for autoreload
    local autoreload_counter = 0
    local autoreload_pending = false
    local function on_frame_update()
        if not game_loaded_fired then
            poll_for_player()
        end
        x4native_api.raise_event("on_frame_update", nil)

        -- Autoreload: check every ~60 frames (~1s at 60fps)
        -- Only present in debug builds (should_autoreload is nil in release)
        if x4native_api.should_autoreload and not autoreload_pending then
            autoreload_counter = autoreload_counter + 1
            if autoreload_counter >= 60 then
                autoreload_counter = 0
                if x4native_api.should_autoreload() then
                    autoreload_pending = true
                    x4native_api.log(1, "Autoreload: core DLL changed on disk — scheduling /reloadui")
                    ScheduleReloadUI()
                end
            end
        end
    end

    local imm_ok, imm_result = pcall(C.IsPlayerValid)
    local imm_ok2, imm_pid = pcall(C.GetPlayerID)
    x4native_api.log(1, string.format("Initial check: valid=%s pid=%s", tostring(imm_result), tostring(imm_pid)))
    if (imm_ok and imm_result) or (imm_ok2 and imm_pid and tonumber(imm_pid) ~= 0) then
        fire_game_loaded("IsPlayerValid (immediate)")
    else
        if not imm_ok then
            x4native_api.log(2, "IsPlayerValid not available: " .. tostring(imm_result) .. " — relying on MD cue")
        end
    end

    -- Always register the per-frame handler
    SetScript("onUpdate", on_frame_update)
end
