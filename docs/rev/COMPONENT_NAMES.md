# Component Name System

## Summary

X4 component names can be both READ and SET at runtime. Reading uses the exported `GetComponentName` function. Setting uses the Lua global `SetComponentName` (not FFI, not PE-exported) which internally constructs a `SetComponentNameAction` and calls its `PerformAction` method. An MD action `set_object_name` also exists for script-driven naming.

There is no direct C++ exported function for setting names. The recommended approach for native extensions is to call `SetComponentName(id, name)` from within a Lua event handler triggered via `raise_lua`.

---

## GetComponentName (Reader)

**Address:** `0x140150C90` (PE export RVA `0x00150C90`)
**Signature:** `const char* GetComponentName(UniverseID componentid)`
**In SDK:** Yes (`x4_game_func_list.inc` line 346)

### Implementation

1. Calls `ComponentRegistry_Find(g_ComponentRegistry, componentid, 4)` to get the component object pointer.
2. If not found, logs error and returns empty string (`byte_1429953F2`).
3. Reads name via **virtual getter** on a sub-object at `component+0x30`:
   - `component+0x30` points to a sub-object with its own vtable
   - Calls `sub_object_vtable[3]` (offset `+0x18`) which returns a `std::string` (move semantics)
4. Result is cached in a **thread-safe static** `std::string` at `0x143144C00` (initialized via `Init_thread_header`/`Init_thread_footer`).
5. Returns `const char*` pointer to the static buffer's data.

### Thread Safety

The static cache at `0x143144C00` uses MSVC's thread-safe static initialization (`Init_thread_header`). However, the returned `const char*` pointer is to a **shared static buffer** that gets overwritten on the next call. Callers must copy the result immediately.

### Caller Count

Zero direct code callers in the binary. Called exclusively through the PE export table (function pointer resolution). Low call frequency -- only invoked when the game or mods explicitly request a component name.

---

## SetComponentName (Lua Global)

**Lua handler address:** `0x140283BA0` (`Lua_SetComponentName`)
**Registration:** In `LuaGlobalFunctionRegistration` at `0x140239DC2`
**Signature (Lua):** `SetComponentName(component_id, name_string)`

### NOT in FFI or PE Exports

`SetComponentName` is registered as a Lua global function via `lua_pushcclosure` + `lua_setfield`, **not** as an FFI `C.` function. It does not appear in:
- `x4_ffi_raw.txt`
- `x4_exports.txt`
- `x4_game_func_list.inc`

### Lua Handler Implementation (`0x140283BA0`)

1. Validates 2 arguments: `component` (number/component ID) and `name` (string).
2. Constructs a `SetComponentNameAction` struct on the stack:
   ```
   Offset  Size  Field
   0x00    8     vtable pointer (= 0x142BD56F8)
   0x08    8     component_id (uint64)
   0x10    32    std::string name (MSVC SSO layout)
   ```
3. Calls `SetComponentNameAction::PerformAction` at `0x140E9BE40`.
4. Cleans up the std::string on the stack.

### Usage in Game Code

Called from multiple UI menus:
- `menu_map.lua` -- rename ships/stations on the map
- `menu_station_configuration.lua` -- rename station from build menu
- `menu_playerinfo.lua` -- rename player
- `menu_mapeditor.lua` -- rename in map editor

All callers use the same pattern: `SetComponentName(componentID, nameString)`.

---

## SetComponentNameAction::PerformAction (Core Setter)

**Address:** `0x140E9BE40`
**Signature:** `char __fastcall PerformAction(SetComponentNameAction* this)`
**Returns:** 1 on success, 0 on failure

### Implementation

1. Looks up component via `ComponentRegistry_Find(g_ComponentRegistry, this->component_id, 4)`.
2. **Can-be-renamed check:** Calls `component->vtable[0x1240/8](component)` (vtable offset `0x1240`). If returns false, logs error "Tried to set name of component '%s' with ID '%llu' that cannot be renamed" and returns 0.
3. **Class 74 special case:** If `component->IsClass(74)`, routes the SetName call through `g_GameUniverse+560` (universe-level name setting). This handles galaxy/universe objects.
4. **Normal path:** Calls `component->vtable[0x1298/8](component, &this->name_string)` -- the SetName virtual method at vtable offset `0x1298` (4760 bytes). This is the primary name write.
5. **Map editor defaults:** Also updates map editor defaults for the component if they exist:
   - For class 71 (object): writes to map defaults offset +88
   - For other classes: writes to map defaults offset +56

### Error Paths

- Component not found: returns 0 silently (no log)
- Cannot be renamed: logs error, returns 0
- Map editor defaults not found: logs warning, still returns 1 (name was already set)

### Key Virtual Method Offsets (on component vtable)

| Offset | Method | Purpose |
|--------|--------|---------|
| `0x1240` (4672) | `CanBeRenamed()` | Returns bool: whether the component can be renamed |
| `0x1298` (4760) | `SetName(std::string*)` | Writes the name to the component |
| `0x11B8` (4536) | `IsClass(int)` | Returns bool: whether component is of given class |

### Relevant Class IDs

| ID | Class | Name Behavior |
|----|-------|---------------|
| 71 | object | Map defaults at offset +88 |
| 74 | (galaxy/universe) | Routed through GameUniverse+560 |
| 82 | controllable | Normal path (includes ships, stations) |

---

## MD Action: set_object_name

**String address:** `0x1429B6210`
**Action ID:** 2295 (`0x8F7`)
**Constructor:** `0x140B8D1C0` (`MD_SetObjectNameOrDescriptionAction_Ctor`)

### XSD Definition

```xml
<xs:element name="set_object_name">
  <xs:complexType>
    <xs:attribute name="object" type="object" use="required" />
    <xs:attribute name="name" type="expression" />  <!-- takes precedence -->
    <xs:attribute name="page" type="page" />
    <xs:attribute name="line" type="line" />
  </xs:complexType>
</xs:element>
```

### Notes

- Supports localized names via page/line, or raw string via `name` attribute
- The `name` attribute takes precedence over page/line
- Handled by `Scripts::SetObjectNameOrDescriptionAction` (shared with `set_object_description`, ID 2296)
- Requires MD cue infrastructure to invoke -- not suitable for direct C++ use
- The constructor at `0x140B8D1C0` resolves XML attributes into the action struct

---

## SetFleetName (FFI Export)

**Address:** `0x1401C5C00` (PE export RVA `0x001C5C00`)
**Signature:** `void SetFleetName(UniverseID controllableid, const char* fleetname)`
**In FFI:** Yes
**In SDK:** Should be in `x4_game_func_list.inc`

This is a **separate** API for fleet names only. It does NOT set the component name.

---

## Recommended Approach for Native Extensions

### Best: Lua Global via Event Handler

Call `SetComponentName(id, name)` from a Lua `RegisterEvent` handler triggered by `raise_lua`:

```lua
-- In extension Lua
RegisterEvent("mymod.set_name", function(_, params)
    local id_str, name = tostring(params):match("^(%d+):(.+)$")
    if id_str and name then
        local comp_id = ConvertStringTo64Bit(id_str)
        if comp_id and comp_id > 0 then
            SetComponentName(comp_id, name)
        end
    end
end)
```

```cpp
// In C++
std::string param = std::to_string(component_id) + ":" + name;
x4n::raise_lua("mymod.set_name", param.c_str());
```

**Pros:** Uses the game's own API, handles all validation, updates map editor defaults.
**Cons:** Async (fires on next Lua event dispatch), requires Lua bridge.

### Alternative: Direct C++ Call to PerformAction

Construct a `SetComponentNameAction` struct and call `0x140E9BE40` directly:

```cpp
// WARNING: Requires exact struct layout matching, fragile across game versions
struct SetComponentNameAction {
    void* vtable;           // = image_base + 0x2BD56F8
    uint64_t component_id;
    // MSVC std::string (32 bytes) follows
};
```

**Pros:** Synchronous, no Lua dependency.
**Cons:** Requires resolving vtable address at runtime, std::string ABI must match exactly, fragile.

### Alternative: Hook GetComponentName

Hook `GetComponentName` with `x4n::hook::after` to override the return value for specific components:

```cpp
// Maintain a map of component_id -> custom_name
std::unordered_map<uint64_t, std::string> custom_names;

x4n::hook::after<&X4GameFunctions::GetComponentName>(
    [](const char* result, UniverseID id) -> const char* {
        auto it = custom_names.find(id);
        if (it != custom_names.end()) {
            return it->second.c_str();
        }
        return result;  // pass through original
    }
);
```

**Pros:** No Lua dependency, synchronous, doesn't modify game state.
**Cons:** Only affects callers that go through the exported `GetComponentName` -- internal game code uses the vtable getter directly. Name won't appear in savegames or any code path that reads the name via the component's virtual method.

---

## Key Addresses

| Symbol | Address | Notes |
|--------|---------|-------|
| `GetComponentName` | `0x140150C90` | PE export, reads name via vtable |
| `Lua_SetComponentName` | `0x140283BA0` | Lua C handler for `SetComponentName` global |
| `SetComponentNameAction::PerformAction` | `0x140E9BE40` | Core name setter, virtual dispatch |
| `SetComponentNameAction vtable` | `0x142BD56F8` | `[0]=dtor, [1]=PerformAction` |
| `LuaGlobalFunctionRegistration` | `0x140236710` | Registration of SetComponentName at `0x140239DC2` |
| `MD set_object_name action ID` | 2295 (`0x8F7`) | MD action constructor at `0x140B8D1C0` |
| `GetComponentName static cache` | `0x143144C00` | Thread-safe static std::string for return value |
| `g_ComponentRegistry` | `0x146C73D30` | Global component registry |
| `std_string_assign` | `0x140097170` | MSVC `std::string::assign(void*, void* src, size_t)` |
| `SetFleetName` | `0x1401C5C00` | Separate FFI export for fleet names only |

---

## Version

Analyzed on X4.exe v9.00 build 600626, 2026-03-23.
