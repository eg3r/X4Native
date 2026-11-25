-- X4Native
-- Lua module loaded via ui.xml (v7.5+)
-- Note: Players must disable Protected UI Mode in Settings > Extensions.
--
-- Multi-stage lifecycle:
--   Stage 1 (game start):    DLL loads immediately, extensions register via x4native_init()
--   Stage 2a (save loaded):  MD cue fires raise_lua_event → Lua forwards "on_game_loaded"
--   Stage 2b (world ready):  MD cue fires raise_lua_event → Lua forwards "on_game_started"
--                             NOTE: event_game_started only fires for NEW GAME, not save loads
--   Stage 2c (universe gen):  MD cue fires raise_lua_event → Lua forwards "on_universe_ready"
--                             event_universe_generated fires for BOTH new game and save loads
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
--   MD → raise_lua_event("x4native.game_loaded")    → RegisterEvent → DLL raise_event("on_game_loaded")
--   MD → raise_lua_event("x4native.game_started")   → RegisterEvent → DLL raise_event("on_game_started")
--   MD → raise_lua_event("x4native.universe_ready")  → RegisterEvent → DLL raise_event("on_universe_ready")
--   MD → raise_lua_event("x4native.game_saved")     → RegisterEvent → DLL raise_event("on_game_save")
--   Per-frame → SetScript("onUpdate") → DLL raise_event("on_frame_update")
--   Save data → the values of a game are in → DLL raise_event("on_save_data_ready")
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

pcall(ffi.cdef, [[
    bool IsNewGame(void);
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
-- Save data (x4n::save)
-- ============================================================
-- Extensions store values that belong in the savegame through the core. The
-- core's values are mirrored in the table below, which ui.xml declares as a
-- savegame variable: the game writes it into every save and puts it back when
-- that save is loaded. Keys and values are strings in the core's encoding;
-- nothing here interprets them.
--
--   __X4NATIVE_SAVE = { format = 1, ext = { [extension] = { [key] = value } } }
--
-- A Lua state starts in one of three ways:
--   a save is loaded   The core dropped its values. The game restores the
--                      table some frames after the player exists and fires
--                      "addonLoaded"; the core then takes its values from it.
--   a new game starts  Nothing is restored, the store starts empty.
--   /reloadui          The core kept its values and they are written into the
--                      new Lua state. The game also puts its copy of the table
--                      back; the core's values go on top of it.
-- Extensions get "on_save_data_ready" once the values can be read.
local SAVE_GLOBAL = "__X4NATIVE_SAVE"
local SAVE_FORMAT = 1
-- A loaded save's table is back before the MD cue for the generated universe
-- fires. Should "addonLoaded" never come, the store takes what is there this
-- many seconds after that cue, or, without the cue, after the player is valid.
local SAVE_RESTORE_GRACE = 10
local SAVE_RESTORE_TIMEOUT = 180

local save_supported = false  -- proxy and core both have the save store
local save_reloaded = false   -- this Lua state began with /reloadui and the core kept its values
local save_ready = false      -- the core holds this game's values
local save_root = nil         -- the table the core's values are mirrored in
local save_waiting = false    -- a save is loading and its table is not back yet
local save_deadline = nil     -- when to stop waiting for "addonLoaded"

-- The lookup of a C function fails when the game lacks the export, so it
-- happens inside the protected call.
local function save_player_exists()
    local ok, pid = pcall(function() return C.GetPlayerID() end)
    return ok and pid ~= nil and tonumber(pid) ~= 0
end

-- Runs before the extensions are initialised: tells the core whether this Lua
-- state continues a running game, so no extension ever reads another game's values.
local function save_start()
    if not x4native_api.save_reset then return end      -- proxy without the save store
    local kept = x4native_api.save_reset(save_player_exists())
    if kept == nil then return end                       -- core without the save store
    save_supported = true
    save_reloaded = kept
end

-- The core takes its values from `root`, the table of a loaded save, or starts
-- empty. Returns false for a table this version must not touch.
local function save_adopt(root)
    if type(root) ~= "table" then root = nil end
    if root and type(root.format) == "number" and root.format > SAVE_FORMAT then
        -- Written by a later X4Native. Leave it as it is, so nothing in it is lost.
        x4native_api.log(2, "Save data: this savegame's values are in format " .. tostring(root.format)
            .. ", this X4Native reads format " .. SAVE_FORMAT .. "; save data is off for this game")
        save_supported = false
        save_waiting = false
        return false
    end
    x4native_api.save_import_begin()
    local ext_table = root and root.ext
    if type(ext_table) == "table" then
        for ext, block in pairs(ext_table) do
            if type(ext) == "string" and type(block) == "table" then
                for key, value in pairs(block) do
                    if type(key) == "string" and type(value) == "string" then
                        x4native_api.save_import_entry(ext, key, value)
                    end
                end
            end
        end
    end
    x4native_api.save_import_end()
    save_root = root
    return true
end

local function save_announce(source)
    save_ready = true
    save_waiting = false
    x4native_api.log(1, "Save data ready (" .. source .. ")")
    x4native_api.raise_event("on_save_data_ready")
end

-- Writes what changed in the core into the table.
local function save_sync()
    if not save_ready then return end
    local root = rawget(_G, SAVE_GLOBAL)
    local rewrite = false
    if root ~= save_root then
        if type(root) ~= "table" then
            -- Gone or overwritten: put ours back.
            root = save_root
            rawset(_G, SAVE_GLOBAL, root)
        elseif save_reloaded then
            -- /reloadui: the game put its copy back. The core's values go on top.
            save_root = root
            rewrite = true
        else
            -- The game restored a save's table after the store was declared ready.
            if save_adopt(root) then save_announce("late restore") end
            return
        end
    end

    -- Nothing to do in most frames: the core then returns nothing.
    local changes, full = x4native_api.save_take_changes(rewrite)
    if not changes then return end
    if root == nil then
        if #changes == 0 then return end   -- nothing stored: leave the save without a table
        root = { format = SAVE_FORMAT }
        rawset(_G, SAVE_GLOBAL, root)
        save_root = root
    end
    local ext_table = root.ext
    if full or type(ext_table) ~= "table" then
        ext_table = {}
        root.ext = ext_table
    end
    for i = 1, #changes, 3 do
        local ext, key, value = changes[i], changes[i + 1], changes[i + 2]
        local block = ext_table[ext]
        if value then
            if type(block) ~= "table" then
                block = {}
                ext_table[ext] = block
            end
            block[key] = value
        elseif type(block) == "table" then
            block[key] = nil
            if next(block) == nil then ext_table[ext] = nil end
        end
    end
end

-- The game world exists (on_game_loaded was raised). Extensions get exactly one
-- "on_save_data_ready" per Lua state with a game in it, whichever way it began.
local function save_game_loaded()
    if not save_supported or save_ready then return end
    if save_reloaded then
        -- /reloadui shut down and re-initialised every extension, so each one needs
        -- the signal again to restore its state. The values never left the core.
        save_announce("/reloadui")
        return
    end
    local ok, new_game = pcall(function() return C.IsNewGame() end)
    if ok and new_game then
        -- A new game has nothing saved: the store starts empty and is ready at once.
        save_adopt(nil)
        save_announce("new game")
    else
        -- A loaded save: "addonLoaded" follows once the game has restored the table.
        save_waiting = true
    end
end

-- The game restored the savegame variables: after a load, and after /reloadui.
local function save_restored()
    if not save_supported or not save_player_exists() then return end   -- also fires in the start menu
    if save_reloaded then
        save_sync()
    elseif save_adopt(rawget(_G, SAVE_GLOBAL)) then
        save_announce("savegame")
    end
end

-- The MD cue for the generated universe fired: the load is over.
local function save_universe_ready()
    if save_waiting then save_deadline = getElapsedTime() + SAVE_RESTORE_GRACE end
end

local function save_tick()
    if save_ready then
        save_sync()
    elseif save_waiting then
        -- Should "addonLoaded" never come, take what is there once the load is over.
        if not save_deadline then
            local ok, valid = pcall(function() return C.IsPlayerValid() end)
            if ok and valid then save_deadline = getElapsedTime() + SAVE_RESTORE_TIMEOUT end
        elseif getElapsedTime() > save_deadline then
            if save_adopt(rawget(_G, SAVE_GLOBAL)) then save_announce("no restore signal") end
        end
    end
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

    -- Expose to other X4Native UI files (e.g. x4n_settings_menu.lua).
    -- Single well-known global; consumers should treat it as read-only.
    _G.__X4NATIVE_API = x4native_api

    -- Before the extensions initialise: drop or keep the save data of the previous Lua state
    save_start()

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
bridge_forward_lua_event("x4native.game_loaded",    "on_game_loaded")
bridge_forward_lua_event("x4native.game_started",   "on_game_started")
bridge_forward_lua_event("x4native.universe_ready",  "on_universe_ready")
bridge_forward_lua_event("x4native.game_saved",     "on_game_save")

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
    local universe_ready_fired = false

    local function fire_universe_ready(source)
        if universe_ready_fired then return end
        universe_ready_fired = true
        x4native_api.log(1, "on_universe_ready triggered via " .. source)
        x4native_api.raise_event("on_universe_ready")
        save_universe_ready()
    end

    local function fire_game_loaded(source)
        if game_loaded_fired then return end
        game_loaded_fired = true
        x4native_api.log(1, "on_game_loaded triggered via " .. source)
        x4native_api.raise_event("on_game_loaded")
        save_game_loaded()
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

    -- Override the bridge handler for universe_ready to use the guard.
    -- event_universe_generated fires for BOTH new game and save loads.
    -- This is the definitive "world ready" signal (all stations built).
    local orig_universe_handler = forwarded_events["x4native.universe_ready"]
    if orig_universe_handler then
        UnregisterEvent("x4native.universe_ready", orig_universe_handler)
        local guarded_universe_handler = function(_, arg)
            if x4native_api then
                fire_universe_ready("MD cue (event_universe_generated)")
            end
        end
        RegisterEvent("x4native.universe_ready", guarded_universe_handler)
        forwarded_events["x4native.universe_ready"] = guarded_universe_handler
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
        save_tick()

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

    -- The game fires this after it has restored the savegame variables
    RegisterEvent("addonLoaded", save_restored)

    -- Always register the per-frame handler
    SetScript("onUpdate", on_frame_update)
end
