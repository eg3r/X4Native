# X4 Save XML Names

> **Binary:** X4.exe v9.00 build 900-611726 | **Date:** 2026-11-03
>
> All addresses are absolute (imagebase `0x140000000`). Subtract imagebase to get RVA.
>
> Behind `sdk/x4n_savexml.h` and the core's SaveXML name hooks (`src/core/save_xml.*`).

---

## 1. Summary

A save's element and attribute names are numbers from one fixed table in the game. X4Native adds names to that table at runtime (`sdk/x4n_savexml.h`), so an extension can write its own elements into a save and read them back. The extension builds its nodes through the game's own calls, and a save written this way still loads without the extension.

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
| `LookupKeyName_LookupName` | `0x140123690` | `int32_t(const char* name, const void* context)` | name to number, as a save file is parsed |

- **FormatXMLName** binary-searches the entries by id. An unknown id logs `LookupKeyName::FormatXMLName(): The lookup ID %lld does not exist in '%s'` and returns the empty name.
- **LookupName** returns 0 at once when the slot is null. Otherwise it calls the namespace's name lookup (`0x140123C20`), which logs nothing; on a 0 result it calls the error logger (`0x1400E6D00`), which logs `LookupKeyName::LookupName(): The key name "%s" is not recognized in lookup %s.%s%s`, and returns 0.

A node whose name has no number cannot be written under that name, and a name in a save file with no number is read as 0: the game logs the line above and skips the node.

---

## 4. Names added at runtime

`x4n::savexml::register_tag` gives an extension's name a number both lookups answer, held by the `x4n::savexml::SaveXmlTag` it returns.

```cpp
if (const auto tag = x4n::savexml::register_tag("my_element")) { /* build nodes with tag->number() */ }
```

- **One table for every extension**, in the core. A name registered twice, by one extension or two, has one number.
- **A name the game knows keeps the game's number**, found by reading the namespace's entries, and its lookups stay the game's.
- **A new name gets the next number above the game's highest.** Numbers are given out first come first served for the process's life; they never reach the file, which holds names, so they may differ between sessions.
- **The core hooks both lookups** with MinHook and answers an added name or number before the game's; every other call goes to the game unchanged.
- **Registration fails, returning no tag,** when the two hooks are not both installed (a missing RVA, or the build-mismatch safe mode) or before the game has built the namespace.
- **Register at `x4native_init`.** A save's names are resolved while its file is parsed, before any list, station or other object is read from the nodes; a name registered later is not recognized in that save. Extensions are initialised again on every save load, before the world loads (`docs/ARCHITECTURE.md`, "Save-Load Lifecycle"), and the table lives for the process, so a name registered once is known to every later save.

The call is `X4NativeAPI::savexml_register`, taken from `_reserved` without an API version change.

---

## 5. Building a node

An extension adds a node the way the game's own writers do, through the game's calls, so the game frees it with the rest of the save's nodes. The list writer (`BlacklistData_Export`, `0x1403725E0`) is the model.

| Field | Offset | Content |
|-------|--------|---------|
| flags | `+0x00` (`X4_XML_NODE_OFFSET_FLAGS`) | `uint8`, `3` for an element (`X4_XML_NODE_FLAG_ELEMENT`); `8` marks a text node (`X4_XML_NODE_FLAG_TEXT`) |
| element | `+0x0C` (`X4_XML_NODE_OFFSET_ELEMENT`) | `int32`, the element's number |
| attributes | `+0x18` (`X4_XML_NODE_OFFSET_ATTRIBUTES`) | small vector of 0x28-byte entries (`X4_XML_ATTRIBUTE_SIZE`) `{int32 id; std::string value at +8}` (`X4_XML_ATTRIBUTE_OFFSET_ID`, `X4_XML_ATTRIBUTE_OFFSET_VALUE`): 4 inline (`X4_XML_NODE_INLINE_CAPACITY`), capacity `+0xB8`, count `+0xC0` (`X4_XML_NODE_OFFSET_ATTRIBUTE_CAPACITY`, `X4_XML_NODE_OFFSET_ATTRIBUTE_COUNT`) |
| children | `+0xC8` (`X4_XML_NODE_OFFSET_CHILDREN`) | small vector of node pointers: 4 inline, capacity `+0xE8`, count `+0xF0` (`X4_XML_NODE_OFFSET_CHILD_CAPACITY`, `X4_XML_NODE_OFFSET_CHILD_COUNT`) |

- **Allocate** 0xF8 bytes (`X4_XML_NODE_SIZE`) through `GameAlloc` with alignment 0x10 (`X4_XML_NODE_ALIGNMENT`).
- **Construct** as the writer's inlined constructor does: `+0x02` (`uint16`, `X4_XML_NODE_OFFSET_SOURCE_LINE`) and `+0x04` (`int32`, `X4_XML_NODE_OFFSET_SOURCE_COLUMN`) set to -1 (`X4_XML_NODE_NO_SOURCE_LINE`, `X4_XML_NODE_NO_SOURCE_COLUMN`), `+0x08` (`uint32`, `X4_XML_NODE_OFFSET_PREFIX`) and `+0x10` (`X4_XML_NODE_OFFSET_TEXT`) zeroed, both vectors' capacity 4 and count 0, flags 3, then the element number at `+0x0C`. The constructor zeroes `+0x08` with one 8-byte store that also clears `+0x0C`, so the element number is written after it; writing `+0x08` as 4 bytes leaves the order free.
- **Set an attribute** with `XmlNode_SetAttribute(node, id, value)` (`0x140123700`).
- **Append a child** with `XmlNodeVector_PushBack(parent + 0xC8, &child)` (`0x1400CB8C0`).

The writer writes a flag attribute such as `whitelist` as `"1"`, and only when it is on, and a faction set as the faction ids joined by a space.

---

## 6. Why the core owns the name table

An extension that writes into a save is native code with full process access, as every X4Native extension is (`docs/SDK_CONTRACT.md`, "Malicious extensions"). Any extension that hooks or injects can already crash the game or corrupt a save, and what it builds is its own responsibility, as each detour it installs is. The SDK does not guard an extension's nodes for the same reason it does not guard its hooks: an extension that wants save data writes it either way, and a capability the SDK withholds is reimplemented by every extension that needs it.

What no extension can do safely alone is number its names, because the numbers are shared by every extension in the process:

- **Without the core,** each extension hooks both lookups itself, and the detours chain. Each picks numbers above the game's highest on its own, so two extensions take the same number for different names. Both extensions' nodes are then written under one name and read back as one number, and each extension reads the other's data as its own. The game skips nothing, because every node carries a valid number: the corruption crosses extensions and nothing reports it.
- **With the core,** there is one table and one pair of hooks. A name has one number whichever extension registers it, a name the game knows keeps the game's number, the hooks are installed both or neither, and the originals are attached before either hook is enabled.

The core takes the shared, collision-prone part and does it once; each extension keeps what is its own, the nodes it builds.

| Owner | Guarantees |
|-------|------------|
| Core (`x4n::savexml`) | A registered name has one number in the process, both lookups answer it, and every other lookup reaches the game unchanged |
| Extension | The nodes it builds are well-formed, are added beside the game's, and can be skipped on load |

---

## 7. Why an addition does not break a save

`docs/SDK_CONTRACT.md` warns: never write to save data from native code, and points to the Lua/MD bridge instead. Data sent over the bridge reaches the same file, since the game stores Mission Director variables in the save, and `docs/EXTENSION_GUIDE.md` describes `save="1"` for an extension that adds persistent data. What the warning protects is a save the game can always load, and an addition made through `x4n_savexml.h` keeps to it:

- **It is additive.** An extension adds its own elements or attributes beside the game's; it changes and removes nothing the game wrote.
- **Nothing in the game reads the added nodes, so they block nothing.** The game's readers match the ids they know and skip every other node, whatever its contents.
- **The game rebuilds what it saves.** The list writer, for one, rebuilds every `<blacklist>` element from its list record on each save, so a save made without the extension drops the added child and nothing else.
- **The numbers never reach the file.** The file holds names; numbers are assigned again in each session.

**The worst case: a player removes the extension.** Neither the game nor the save breaks. A save made with the extension holds a little extra data. The next load logs one "not recognized" line per added element and skips it, and the next save no longer carries it.
