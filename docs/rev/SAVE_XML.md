# X4 Save XML Names

> **Binary:** X4.exe v9.00 build 900-611726 | **Date:** 2026-11-03
>
> All addresses are absolute (imagebase `0x140000000`). Subtract imagebase to get RVA.
>
> Behind `sdk/x4n_savexml.h`, the core's SaveXML name hooks (`src/core/save_xml.*`), and the save store in `sdk/x4n_save.h` and `src/core/save_store.*`.

---

## 1. Summary

A save's element and attribute names are numbers from one fixed table in the game. X4Native adds names to that table at runtime (`sdk/x4n_savexml.h`), so an extension can write its own elements into a save and read them back. A save is written and read as a stream: the extension builds its nodes through the game's own calls and writes them to the save stream, the core reads them back whole where the game would skip them, and a save written this way still loads without the extension.

---

## 2. The SaveXML name table

A save is an XML document, but in memory the game holds it as nodes that carry no names: every element and attribute is a number. The numbers come from one fixed table, the **SaveXML lookup namespace**, built at startup and reached through the pointer slot `0x146CF15D8` (`X4_RVA_SAVEXML_NAMESPACE`).

| Field | Offset | Content |
|-------|--------|---------|
| entries | `+0xFB10` | 24-byte entries inline, sorted by id (`X4_SAVEXML_OFFSET_ENTRIES`) |
| entry count | `+0x17880` | `size_t` (`X4_SAVEXML_OFFSET_ENTRY_COUNT`) |
| entry name | entry `+0x00` | `const char*` |
| entry id | entry `+0x10` | `int32` |
| empty name | `+0x17890` | the `std::string` a failed id lookup returns |

Build 900-611726 holds 1337 names. A game reader of the in-memory nodes compares these numbers against constants and skips any it does not know.

---

## 3. The two lookups

Both are the SaveXML instantiation of the game's `LookupKeyName` template. Every lookup namespace has its own copy of each, sharing the error strings (74 copies of `FormatXMLName` in build 900-611726). These two load the SaveXML namespace from its slot themselves and take no namespace argument, so every call they receive is a SaveXML lookup.

| Function | Address | Declaration | Used |
|----------|---------|-------------|------|
| `LookupKeyName_FormatXMLName` | `0x140122550` | `const char*(const int32_t* id)` | number to name, as a save is written |
| `LookupKeyName_LookupName` | `0x140123690` | `int32_t(const char* name, const std::string* context)` | name to number, as a save file is parsed; `context` is the source the error line names |

- **FormatXMLName** binary-searches the entries by id. An unknown id logs `LookupKeyName::FormatXMLName(): The lookup ID %lld does not exist in '%s'` and returns the empty name.
- **LookupName** returns 0 at once when the slot is null. Otherwise it calls the namespace's name lookup (`0x140123C20`), which logs nothing; on a 0 result it calls the error logger (`0x1400E6D00`), which logs `LookupKeyName::LookupName(): The key name "%s" is not recognized in lookup %s.%s%s`, and returns 0.

A node whose name has no number cannot be written under that name, and a name in a save file with no number is read as 0: the game logs the line above and skips the node.

---

## 4. Names added at runtime

`x4n::savexml::register_tag` gives an extension's local name a number both lookups answer, held by the `x4n::savexml::SaveXmlTag` it returns.

```cpp
if (const auto tag = x4n::savexml::register_tag("my_element")) { /* build nodes with tag->number() */ }
```

- **The core stores every name under the caller's extension id**, as `<extension id>.<local name>`. The caller is the `X4NativeAPI*` the extension was initialised with; its id is the `content.xml` id. An extension registers its local names and never writes or parses the separator.
- **Registering always adds a prefixed name.** No registration returns a game number: `blacklist` registered by `my_mod` is stored as `my_mod.blacklist` and gets a number of its own.
- **A game name is read, not registered.** `x4n::savexml::game_number("blacklist")` answers the game's own number and registers nothing. It reads the game's table only, and answers 0 for an unknown name or an empty name.
- **Never hard-code a game number.** A number such as `0x67` (`<blacklist>`) is a position in the game's table, and the table grows between builds: from 1334 to 1338 names within v9.00. Look the number up with `game_number` at load.
- **One table for every extension**, in the core. A name registered again by the same extension keeps its number; the same local name from two extensions gets two names and two numbers.
- **A new name gets the next number above the game's highest.** Numbers are given out first come first served for the life of the core; they never reach the file, which holds names, so they may differ between sessions.
- **The core hooks both lookups** with MinHook and answers an added name or number before the game's. Every name without the separator, and every number below the first added, goes to the game unchanged, without a lock or an allocation.
- **The core hooks the readers' skip** (`XmlNode_SkipChildren`, section 5) and reads an element carrying an added number whole, so it is on its parent node when the game's reader returns. Every other element goes to the game's skip unchanged.
- **An id or local name that is not a valid XML name is refused**, and so is one carrying the separator; the call returns 0 and the log names the id and the reason.
- **Registration fails, returning no tag,** when the three hooks are not all installed: a missing RVA, or the build-mismatch safe mode. The core then logs it, and once the world is up shows the player a notice that extensions cannot keep their data in the save this session.
- **Treat missing data as a first run.** A save may carry none of an extension's data: it was made before the extension was installed, while the extension was removed, or in a session without the hooks, whose next save drops what the loaded save carried. An extension that finds nothing starts as it would in a new game.
- **Register at `x4native_init`.** A save's names are resolved while its file is parsed, before any list, station or other object is read from the nodes; a name registered later is not recognized in that save. Extensions are initialised again on every save load, before the world loads (`docs/ARCHITECTURE.md`, "Save-Load Lifecycle"), and the table lives for the life of the core, so a name registered once is known to every later save.
- **Each registration is logged with its owner**, and a load that holds data from an extension that is not loaded logs it once per name per load:

```text
SaveXML: my_mod registered 'my_mod.state' = 1338
SaveXML: save contains data from 'my_mod' (not loaded): 'my_mod.state' skipped, it is dropped on the next save
```

The calls are `X4NativeAPI::savexml_register` and `X4NativeAPI::savexml_lookup`, taken from `_reserved` without an API version change.

---

## 5. Building and reading a node

An extension builds and reads nodes through `x4n::savexml`, and the core builds them the way the game's own writers do, so the game frees a node with the rest of the save once it is appended to one of the game's nodes. The node layout is MSVC's layout of a small vector and a `std::string`, so it stays inside the core (`src/core/save_xml_layout.h`) and is not part of the SDK.

| Call | Does |
|------|------|
| `create_node(element)` | a new element node, or null |
| `set_attribute(node, attribute, value)` | sets or replaces an attribute |
| `append_child(parent, child)` | appends a child |
| `get_attribute(node, attribute)` | the attribute's value, owned by the node, or null |
| `element(node)` | the element number; 0 for a text node |
| `children(node)` | the children, in order |

**A value is at most 10,000,000 bytes.** That is libxml2's `XML_MAX_TEXT_LENGTH`, and the game's save reader is created without the option that lifts it, so a longer value would break the load. `set_attribute` refuses one and logs the extension and the attribute; larger data is split across several attributes or child nodes by the extension.

```cpp
void* rules = x4n::savexml::create_node(rules_tag->number());
x4n::savexml::set_attribute(rules, enabled_tag->number(), "1");
x4n::savexml::append_child(list_node, rules);
```

A game writer that receives a node, such as the list writer `BlacklistData_Export` (`0x1403725E0`) with each `<blacklist>`, is where an extension hooks to add its own child; the list writer writes a flag attribute such as `whitelist` as `"1"`, and only when it is on, and a faction set as the faction ids joined by a space.

### The layout the core builds

| Field | Offset | Content |
|-------|--------|---------|
| flags | `+0x00` | `uint8`, `3` for an element; `8` marks a text node |
| element | `+0x0C` | `int32`, the element's number |
| attributes | `+0x18` | small vector of 0x28-byte entries `{int32 id; std::string value at +8}`: 4 inline, capacity `+0xB8`, count `+0xC0` |
| children | `+0xC8` | small vector of node pointers: 4 inline, capacity `+0xE8`, count `+0xF0` |

- **Allocated** as 0xF8 bytes through `GameAlloc` with alignment 0x10.
- **Constructed** as the list writer's inlined constructor does: `+0x02` (`uint16`) and `+0x04` (`int32`) set to -1, `+0x08` and `+0x10` zeroed, both vectors' capacity 4 and count 0, flags 3, then the element number at `+0x0C`. The constructor zeroes `+0x08` with one 8-byte store that also clears `+0x0C`, so the element number is written after it.
- **Attributes** are set with `XmlNode_SetAttribute(node, id, value)` (`0x140123700`), **children** appended with `XmlNodeVector_PushBack(parent + 0xC8, &child)` (`0x1400CB8C0`).

### The universe save and load

| Function | Address | Declaration |
|----------|---------|-------------|
| `UniverseData_Export` | `0x1408DE190` | `bool(void* universe, void* stream)` |
| `UniverseData_Import` | `0x1408DC5F0` | `bool(void* universe, void* node, void* reader, void* arg4, int32_t skip_sections)` |

- **The export writes inside the open `<universe>` element.** Its one caller opens `<universe>` (`0x4E7`) with `XmlNode_Construct(open = true)` and passes the stream. Each section, `<blacklists>` (`0x68`) among them, is a node of the export's own: constructed, filled, written with `XmlNode_Write`, destroyed. It returns false when a write fails, and the caller then abandons the save.
- **The import reads `<universe>` child by child.** The node holds no children on entry. A loop reads each child's start tag (`0x14058F5E0`), pushes it onto the node and dispatches by element number: a known section is read whole with `XmlNode_ReadChildren` and applied, an unknown one is skipped with `XmlNode_SkipChildren`. `skip_sections` set skips the sections a partial load leaves out. Every child stays on the node until the import returns.
- **A load runs the import twice.** Both runs read the same save, so a reader that sets its state from what it finds sets the same state twice.

### Writing an extension's element

In an after-hook on `UniverseData_Export`, when the game's sections were written, build the node in storage of the extension's own, write it to the stream, and destroy it. It lands as the last child of `<universe>`.

```cpp
x4n::hook::after<&X4GameFunctions::UniverseData_Export>([](bool& ok, void*, void* stream) {
    if (!ok) return;
    auto* game = x4n::game();
    alignas(16) std::uint8_t storage[0x200];  // a node is 0xF8 bytes in build 900-611726
    void* node = game->XmlNode_Construct(storage, stream, tag->number(), false);
    game->XmlNode_SetAttribute(node, value_tag->number(), "1");
    ok = game->XmlNode_Write(node, stream, true);
    game->XmlNode_Destroy(node);
});
```

- **A child** under that node is built with `create_node` and appended with `append_child`; `XmlNode_Destroy` on the node frees it.
- **A node the game passes in** is a node to add to. `BlacklistData_Export` receives the `<blacklists>` node before the export writes it, so a child appended there is written with it.
- The list writer writes a flag attribute such as `whitelist` as `"1"`, and only when it is on, and a faction set as the faction ids joined by a space.

### Reading it back

In an after-hook on `UniverseData_Import`, look for the element among the `<universe>` node's children. The core's skip hook has read it whole, its attributes and children included.

```cpp
x4n::hook::after<&X4GameFunctions::UniverseData_Import>(
    [](bool& ok, void*, void* universe, void*, void*, int32_t) {
        if (ok) read_my_element(universe);  // x4n::savexml::children(universe), by element
    });
```

---

## 6. Why the core owns the name table

An extension that writes into a save is native code with full process access, as every X4Native extension is (`docs/SDK_CONTRACT.md`, "Malicious extensions"). Any extension that hooks or injects can already crash the game or corrupt a save, and what it builds is its own responsibility, as each detour it installs is. The SDK does not guard an extension's nodes for the same reason it does not guard its hooks: an extension that wants save data writes it either way, and a capability the SDK withholds is reimplemented by every extension that needs it.

What no extension can do safely alone is number its names, because the numbers are shared by every extension in the process:

- **Without the core,** each extension hooks both lookups itself, and the detours chain. Each picks numbers above the game's highest on its own, so two extensions take the same number for different names. Both extensions' nodes are then written under one name and read back as one number, and each extension reads the other's data as its own. The game skips nothing, because every node carries a valid number: the corruption crosses extensions and nothing reports it.
- **With the core,** there is one table and one set of hooks. A name is stored under the caller's id, so the same local name from two extensions is two names with two numbers. A name the game adds in a later build cannot collide with an added one either, because every added name carries the separator and no game name does. The three hooks are installed all or none, and the originals are attached before any hook is enabled.

The core takes the shared, collision-prone part and does it once; each extension keeps what is its own, the nodes it builds. The detours cost the game nothing: a game name, or a game number below the first added, is answered by the game without a table read, a lock or an allocation, and the skip hook runs only for an element the game does not know.

| Owner | Guarantees |
|-------|------------|
| Core (`x4n::savexml`) | A registered name is stored under its extension's id, both lookups answer it, an element carrying it is read whole on load, and every game name, game number and game element reaches the game unchanged |
| Extension | The nodes it builds are well-formed, are written beside the game's, and can be skipped on load |

---

## 7. The core's value store

The core writes one element of its own into a save that holds store values, `<x4native.store>`, as the last child of `<universe>`. An extension stores key-value data in it through `x4n::save` (`sdk/x4n_save.h`); the core owns the XML, so an extension never builds a node for it.

```xml
<universe>
  ...
  <x4native.store>
    <x4native.ext id="my_mod">
      <x4native.value key="intro_seen" type="bool" value="1"/>
      <x4native.value key="fleet_name" type="string" value="Red Wing"/>
    </x4native.ext>
  </x4native.store>
</universe>
```

- **The three element names are registered by the core** through the same scheme as an extension's, under the core's own id `x4native`, so they carry the separator and pass the load check of section 4. The attributes `id`, `key`, `type` and `value` are the game's names, used through `game_number`; the game never reads inside the store, so sharing a number is harmless.
- **The core hooks the universe save and load** (section 5). After `UniverseData_Export` it builds `<x4native.store>` under a holder node of its own, writes it to the stream and destroys the holder; after `UniverseData_Import` it reads `<x4native.store>` from the `<universe>` node. Both entries are in `native/version_db/internal_functions.json` with find hints and byte signatures, and the core joins its own hook manager's chain on them, so an extension hooking the same functions joins the same chain.
- **One block per loaded extension, and only for loaded extensions.** At load, a block whose id is not a loaded extension is dropped and logged once:

```text
SaveXML store: save holds values of 'my_mod' (not loaded); they are dropped on the next save
```

- **The block is written only when an extension has at least one value;** with no values at all the core writes no `<x4native.store>`.
- **Values survive a UI reload.** A plain `/reloadui` keeps the core and its values in memory. A core hot-reload carries them across in the proxy's stash under its own namespace, together with the loaded world's player id, and restores them on start when that id matches, as an extension's own stash state does. A new game or another save does not inherit them.
- **The types are `bool`, `int` (64-bit), `number` (double) and `string`.** A `get` with the wrong type logs a warning and returns its fallback, as the settings calls do. A key and a value are attribute values, so anything the XML writer escapes (a quote, `<`, `&`) survives the round trip.

---

## 8. Why an addition does not break a save

`docs/SDK_CONTRACT.md` allows native code to add save data only through `x4n::savexml`, beside the game's own, never changing or removing what the game wrote; the save store (`x4n::save`) writes through that same core surface for key-value data, and the Lua/MD bridge and the stash stay for the other cases. `docs/EXTENSION_GUIDE.md` describes `save="1"` for an extension that adds persistent data. What the rule protects is a save the game can always load, and an addition made through these surfaces keeps to it:

- **It is additive.** An extension adds its own elements or attributes beside the game's; it changes and removes nothing the game wrote.
- **Nothing in the game reads the added nodes, so they block nothing.** The game's readers match the ids they know and skip every other node, whatever its contents; the core reads an added one whole in place of the skip, and the game's reader goes on as it does after a skip.
- **The game rebuilds what it saves.** Every writer writes from the game's own objects on each save, so a save made without the extension drops what the extension added and nothing else. Shown in game for build 900-611726: an element the game does not know, added under `<universe>` and at the top level of a save, is gone from the next save, and a save written with an extension's element loads cleanly without the extension and drops it on the next save.
- **The numbers never reach the file.** The file holds names; numbers are assigned again in each session.

**The worst case: a player removes the extension.** Neither the game nor the save breaks. A save made with the extension holds a little extra data. The next load logs one line per added name, naming the missing extension, and skips its elements, and the next save no longer carries them.
