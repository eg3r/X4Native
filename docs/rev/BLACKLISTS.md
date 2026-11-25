# X4 Blacklists (Travel and Activity Lists)

> **Binary:** X4.exe v9.00 build 900-611726 | **Date:** 2026-11-03
>
> All addresses are absolute (imagebase `0x140000000`). Subtract imagebase to get RVA.
>
> The internals behind the `Blacklist*`, `BlacklistData_*`, `AutoIDMap_Lookup` and `FindOfferAction` entries of `sdk/x4_internal_func_list.inc` and the blacklist offsets of `sdk/x4_manual_types.h`.

---

## 1. Summary

The player's blacklists (sector travel, sector activity and object activity lists) live in one registry keyed by list id. Each list record holds a compiled check object, which the game rebuilds from the record when a save is loaded, a list is edited or created, and when every list is rebuilt. A ship or station fetches the check of the list it uses, and the trade searches `find_sell_offer` and `find_buy_offer` test each candidate partner against it. One writer and one reader save and load the registry.

---

## 2. The registry and its records

The player's lists (the game's "blacklists", of the types `sectortravel`, `sectoractivity` and `objectactivity`) live in one registry, reached through the pointer slot `0x146CF1850` (`X4_RVA_BLACKLIST_REGISTRY`). The registry is a tree keyed by list id:

| Field | Offset | Content |
|-------|--------|---------|
| tree sentinel | registry `+0x10` | |
| tree root | registry `+0x18` | |
| node id | node `+0x20` | `int32`, the list id |
| node record | node `+0x28` | the list record |
| default built | registry `+0x50` | `uint8`, set once the shared default check exists |
| default check | registry `+0x58` | the shared default check, built on first use by `0x1403716F0` |

`BlacklistRecordById(registry, id)` (`0x140371540`) walks the tree and returns the record, or 0 for a non-positive id or a miss. The same function serves other registries of this layout: trade rules live in their own registry (the pointer slot `0x146CF17D8`), which the export `SetContainerTradeRule` passes to it. The exports `GetAllBlacklists`, `GetBlacklistInfo2`, `CreateBlacklist2` and `UpdateBlacklist2` serve list access by id from Lua and C.

| Record field | Offset | Content |
|--------------|--------|---------|
| type | `+0x08` | `int32`, the list type index (`X4_BLACKLIST_OFFSET_TYPE`) |
| name | `+0x10` | `std::string` (`X4_BLACKLIST_OFFSET_NAME`) |
| faction set | `+0x48` | the factions the list names |
| check | `+0x70` | the compiled check object (`X4_BLACKLIST_OFFSET_FILTER`) |

`BlacklistTypeFromName(unused, &string_view)` (`0x14030E2D0`) maps a type name (`sectortravel`, `sectoractivity`, `objectactivity`) to its index by a binary search over a table built at startup. Its second argument is a pointer to a 16-byte `{const char*; uint64_t}` view.

---

## 3. The check and its rebuild

A record's check at `+0x70` is a filter object with four virtual slots: slot 0 destroys it (called with argument 1), slot 3 tests a candidate component and returns `bool`. A filter's children sit in its header: capacity `+0x28`, count `+0x30`, a children-unowned flag `+0x38` and a shared flag `+0x39`, which the game reads directly.

`BlacklistFilterBuild(record)` (`0x140370740`) compiles the check from the record: it first destroys the object at `+0x70` through slot 0 with argument 1 and clears the slot, then builds a new root from the record's macros, factions, relation and hazardous flag. The slot stays null for a list that has none of them. It is the only function that writes the slot, and it is reached from six call sites:

| Call site | Caller | When |
|-----------|--------|------|
| `0x1403721CB`, `0x1403725BB` | `BlacklistData_Import` | a save is loaded |
| `0x1401BD80B` | the export `UpdateBlacklist` | a list is edited |
| `0x1401BDD7D` | the export `UpdateBlacklist2` | a list is edited |
| `0x140370722` | `0x140370650`, from `CreateBlacklist` / `CreateBlacklist2` through `0x140371370` | a list is created |
| `0x140371AB4` | `0x140371A80`, which rebuilds the shared default check and then every list in the registry | see below |

`0x140371A80` has six call sites of its own:

| Call site | Caller |
|-----------|--------|
| `0x140706117` | `0x140706110`, a thunk in slots 43 and 44 of the `U::Player` vtable (`0x142B329A8`) |
| `0x1408DC9F9` | `0x1408DC5F0`, the save reader that also calls `BlacklistData_Import` |
| `0x140BD3617`, `0x140BD363C` | `0x140BD3570`, a script action handler, after it sets or clears the value at `+0x390` of a space |
| `0x1407EDED6` | `0x1407EDE90`, after it clears the same value |
| `0x1409EDF7D` | `0x1409ED5B0` |

What fires the two `U::Player` methods, and what the last three callers stand for, is not known statically. A rebuild replaces the check object; the record itself is untouched.

---

## 4. The list a controllable uses

`BlacklistFetchForControllable(controllable, type, flag, group)` (`0x140463180`) resolves the list a ship or station uses for a list type and group and returns that list's check object (`[record+0x70]`). The effective-list resolver (`0x140463020`) reads the type (0 to 2) and the group and never reads `flag`; it takes the controllable's own list, else follows the link at `[controllable+0xC0]` (through `0x1404A76A0`) to another controllable and tries that one's, else takes the default of the owner. With no list, the function returns the registry's shared default check (`[registry+0x58]`) when the controllable's owner (vtable slot `0x1620`) is the player's faction (the global at `0x1438F4BD0`), and null otherwise. The result is also null for a list that has no criteria, so a caller has to check it. The export `IsComponentBlacklisted` and the match compiler call it.

The property `isblacklisted`, used by the trade order script `order.trade.perform` before a deal and when docking, fetches and tests in one evaluator case on the main thread: the fetch is followed at once by the test of the object it returned.

---

## 5. The trade searches

The script actions `find_sell_offer` and `find_buy_offer` run through one handler, `FindOfferAction` (`0x140B8F000`), declared `void*(void* action, void*, void*, void*)`: it reads `rcx` and `r8` and never reads `rdx` and `r9`. It is a virtual method, reached through the action's vtable. Its first use of `rcx` copies the action to `r13`, and it branches on the action's command id, the `uint32` at `+0x08` (`X4_SCRIPT_ACTION_OFFSET_COMMAND_ID`). It tests that id against `0x7B0` only (`0x140B8F132`, `0x140B8FBD3`) and runs `find_buy_offer` for any other id:

| Command id | Action | The searching ship |
|------------|--------|--------------------|
| `0x7B0` (`X4_SCRIPT_COMMAND_FIND_SELL_OFFER`) | `find_sell_offer` | buys |
| `0x74E` (`X4_SCRIPT_COMMAND_FIND_BUY_OFFER`) | `find_buy_offer` | sells |

Both ids are per-build values. A command id is the position of the action's name in the game's script name table at `0x1426406A0` (2576 entries of `{const char* name; uint64 id}` in this build), so a name added to the table shifts every id after it. Extensions read the two ids from `X4GameOffsets` (`script_command_find_sell_offer`, `script_command_find_buy_offer`). To re-derive them, find the two names in that table; the action factory also loads them as immediates, `mov edx, 0x7B0` at `0x140B684A2` and `mov edx, 0x74E` at `0x140B684EC`.

Both branches search the offers in range and test each candidate partner against the blacklist the action's `match_use_blacklist` node names (the object activity list in the game's trade scripts) before the handler returns, so a check made while the handler runs belongs to the side its command id names.

A trade itself is resolved from a handle through the order id map, `XLib::AutoIDMap<U::OrderIDBase>` at the slot `0x146CF1750` (`X4_RVA_ORDER_ID_MAP`): `AutoIDMap_Lookup(map, handle, 2)` (`0x14033D420`) returns the trade, which holds its seller's component id at `+0x10` and its buyer's at `+0x18` (`X4_TRADE_OFFSET_SELLER`, `X4_TRADE_OFFSET_BUYER`). An order that carries a trade holds the trade handle in its own field at `+0x60` (`X4_ORDER_OFFSET_TRADE`), 0 otherwise; the export `CancelPlayerInvolvedTradeDeal` resolves it this way. The field at `+0x58` (`X4_ORDER_OFFSET_PARAMS`) is not the parameter values themselves but a pointer to the order's parameter vector `{begin, end, capacity}`, null until the parameters are initialised; each entry is 16 bytes, the type at `+0x00` and the payload at `+0x08`.

---

## 6. The writer and the reader

The registry is saved and loaded through one pair of functions, each called once by the universe's save and load with the registry:

- `BlacklistData_Export(registry, parent)` (`0x1403725E0`) writes one `<blacklist>` element (id `0x67`) per record under `parent`. Each node is allocated through `GameAlloc` (0xF8 bytes), carries the list's `id` (`0x1FE`), `type`, `name`, its macros, its `<owner>` with `factions` (ids joined by a space) and `whitelist` (written as `"1"`, and only when on), and its hazardous flag, and is appended to `parent`'s children. The writer rebuilds every element from its record on each save. Node building is described in `docs/rev/SAVE_XML.md` §5.
- `BlacklistData_Import(registry, lists)` (`0x140371D20`) reads every `<blacklist>` child of `lists` into a record, matching attribute and child ids against the SaveXML numbers it knows and skipping any other node; it keeps the first `<owner>` only. It rebuilds each record's check through `BlacklistFilterBuild`. Its error strings begin `BlacklistData::Import()`.

Element and attribute names in a save are numbers from the SaveXML table; see `docs/rev/SAVE_XML.md`.
