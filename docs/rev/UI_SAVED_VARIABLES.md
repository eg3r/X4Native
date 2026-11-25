# X4 UI Saved Variables (Lua Globals Stored in the Savegame)

> **Binary:** X4.exe v9.00 build 900-611726 | **Date:** 2026-11-25
>
> All addresses are absolute (imagebase `0x140000000`). Subtract imagebase to get RVA.
>
> Behind `sdk/x4n_save.h`, `src/core/save_store.*` and the save data part of `extension/ui/x4native.lua`.

---

## 1. Summary

A UI addon can declare Lua globals that the game stores for it, either in the player's profile or in the savegame. The game writes a declared savegame variable into every save as Lua source text and runs that text again when the save is loaded. X4Native declares one such variable and keeps the values of `x4n::save` in it, so extension data travels inside the save without any hook.

---

## 2. Declaring a variable

An addon's `ui.xml` names the variables inside its `<environment>`:

```xml
<savedvariable name="__X4NATIVE_SAVE" storage="savegame" />
```

- `storage` is `savegame` or `userdata` (the profile's `uidata.xml`).
- `ui/core/addon.xsd` allows this for extension addons; a name must not begin with `__CORE`.
- The addon constructor (`0x140ACFF00`) reads the `savedvariable` children and keeps the savegame names at `addon+0xB0` and the userdata names at `addon+0xC8`.

---

## 3. Writing: what a save contains

| Function | Role |
|----------|------|
| `0x140B3BCC0` | UI export. Sets the attribute `version` (id 1287) of `<ui>` to 1, creates `<addons>` (id 22) and calls the next one. |
| `0x140AD2D60` | For each environment: collects the text of all its addons into one `<data>` node (id 289) with the attribute `environment` (id 375). |
| `0x140AD0E50` | For one addon: reads each declared global with `lua_getfield(L, LUA_GLOBALSINDEX, name)` and formats it. |
| `0x140B22E70` | `LuaHelper::SerializeLuaTable`: formats a table, walking it with `lua_next`. |

The global is read from the live Lua state while the save is written, on the main thread. A save then holds:

```xml
<ui version="1">
<addons>
<data environment="menus">
__CORE_HELPTEXT_QUEUE = {
["version"]=2.000000,}
__X4NATIVE_SAVE = {
["ext"]={
[":my_mod"]={
[":note"]=":Red%20Wing",},},
["format"]=1.000000,}
</data>
</addons>
</ui>
```

How a value is written:

| Lua value | Written as |
|-----------|------------|
| number | `%f`, six decimals |
| boolean | `true` / `false` |
| string | through Lua's `string.format("%q", s)`; the string is cut at its first NUL byte |
| string that converts to a number | **as a number**: `"123"` becomes `123.000000`, `"0x10"` becomes `16.000000`, `"inf"` becomes a bare `inf` |
| table | nested, up to about 10 levels; keys are numbers or strings under the same rules |
| anything else | not written, with a log line |

All addons of one environment share one text node.

---

## 4. Reading: what a load does

`0x140B3BBE0` checks the `version` attribute and calls `0x140AD3350`, which finds the Lua state of each `<data>` node's environment and runs the text with `luaL_loadstring` and `lua_pcall`. It then fires the UI event `addonLoaded`.

- The text runs in the Lua state that already exists, after the addon's Lua files have run.
- One chunk holds every variable of the environment. An error in it stops the chunk, so the variables after the failing one stay unset. The log then shows `Error while executing the loaded savegame data chunk for environment: 'menus'`.
- A bare `inf` is such an error (`table index is nil`): it is an undefined global. This is how a single string value `"inf"` can cost every addon its saved variables.
- A `version` other than 1 skips the whole section and fires no event.

---

## 5. Order of events

Measured in the running game with a probe addon. Frame numbers are `onUpdate` calls of the new Lua state; they vary with the save.

**A save is loaded** (from the start menu or from inside a game):

| Step | State |
|------|-------|
| Lua files run | `GetPlayerID()` is 0, the variable is nil. `IsNewGame()` still reads true here: the load has not begun |
| frame 92 | `GetPlayerID()` is non-zero and `IsNewGame()` is false; X4Native fires `on_game_loaded` |
| frame 193 | `IsPlayerValid()` is true |
| frame 212 | the chunk has run, the variable holds the saved value, `addonLoaded` fires |

**A new game starts:** `IsNewGame()` (`0x1401B3420`, true while the global at `0x143D196F0` is 0) is true from the first script line and stays true. Nothing is restored and `addonLoaded` does not fire. Because the flag also reads true at the first script line of a load, it tells the two apart only once the player exists.

**`/reloadui`:** the game writes the variables out as they are and runs that text in the new Lua state. The Lua files run with the variable nil, the first `onUpdate` still sees nil, then the chunk runs and `addonLoaded` fires. Changes made since the last save are kept. `GetPlayerID()` is non-zero from the first script line, which tells a reload from a load.

When another game is loaded or started from inside a game, the old world is gone before the new Lua state starts: `GetPlayerID()` is 0 at the first script line.

---

## 6. Consequences for stored data

- **Keep every key and value a string that cannot convert to a number.** X4Native puts a `:` in front of each.
- **Keep quotes, control bytes and non-ASCII bytes out.** X4Native writes everything outside letters, digits, `_`, `-` and `.` as `%XX`. NUL bytes then survive as well.
- **Stay small.** The text node is shared with the game's own variables. The save reader is created without libxml2 parse options (`0x1412E7540`), so the default limit of 10 MB per text node should apply; this is read from the code and was not tested. X4Native caps the store at about 2 MiB.
- **Wait for `addonLoaded` after a load.** The values are not there at `on_game_loaded`.

`extension/ui/x4native.lua` describes how X4Native keeps the table and the core's values in step.
