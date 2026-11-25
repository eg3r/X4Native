-- x4native_save_store — the UI half of the save store example.
--
-- The extension raises "x4native_save_store.loaded" with the count it stored;
-- this file hands that count to the Mission Director script through a UI
-- triggered event.

local function on_loaded(_, count)
    AddUITriggeredEvent("x4native_save_store", "loaded", tostring(count or 0))
end

RegisterEvent("x4native_save_store.loaded", on_loaded)
