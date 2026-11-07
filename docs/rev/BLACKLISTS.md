# X4 Blacklists (Trade, Travel and Activity Lists)

> **Binary:** X4.exe v9.00 build 900-611726 | **Date:** 2026-11-03
>
> All addresses are absolute (imagebase `0x140000000`). Subtract imagebase to get RVA.
>
> The internals behind the `Blacklist*`, `BlacklistData_*`, `AutoIDMap_Lookup` and `FindOfferAction` entries of `sdk/x4_internal_func_list.inc` and the blacklist offsets of `sdk/x4_manual_types.h`.

---

## 1. Summary

The player's trade, travel and activity lists live in one registry keyed by list id. Each list record holds a compiled check object, which the game rebuilds from the record when a save is loaded, a list is edited or created, and when every list is rebuilt. A ship or station fetches the check of the list it uses, and the trade searches `find_sell_offer` and `find_buy_offer` test each candidate partner against it. One writer and one reader save and load the registry.

---

## 2. The registry and its records

The player's lists (the game's "blacklists": trade, sector travel and object activity lists) live in one registry, reached through the pointer slot `0x146CF1850` (`X4_RVA_BLACKLIST_REGISTRY`). The registry is a tree keyed by list id:

| Field | Offset | Content |
|-------|--------|---------|
| tree sentinel | registry `+0x10` | |
| tree root | registry `+0x18` | |
| node id | node `+0x20` | `int32`, the list id |
| node record | node `+0x28` | the list record |

`BlacklistRecordById(registry, id)` (`0x140371540`) walks the tree and returns the record, or 0 for a non-positive id or a miss. The exports `GetAllBlacklists`, `GetBlacklistInfo2`, `CreateBlacklist2` and `UpdateBlacklist2` serve list access by id from Lua and C.

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
| `0x140371AB4` | `0x140371A80`, which rebuilds every list in the registry | reached through `0x140706110` from slots 43 and 44 of the `U::Player` vtable (`0x142B329A8`) |

What fires the two `U::Player` methods is not known statically. A rebuild replaces the check object; the record itself is untouched.

---

## 4. The list a controllable uses

`BlacklistFetchForControllable(controllable, type, flag, group)` (`0x140463180`) resolves the list a ship or station uses for a list type and group and returns that list's check object (`[record+0x70]`). Where the controllable has no list of its own it returns the controllable's default through its vtable slot `0x1620`, a shared default filter. The export `IsComponentBlacklisted` and the match compiler call it.

The property `isblacklisted`, used by the trade order script `order.trade.perform` before a deal and when docking, fetches and tests in one evaluator case on the main thread: the fetch is followed at once by the test of the object it returned.

---

## 5. The trade searches

The script actions `find_sell_offer` and `find_buy_offer` run through one handler, `FindOfferAction` (`0x140B8F000`), declared `void*(void* action, void*, void*, void*)`: it reads `rcx` and `r8` and passes `rdx` and `r9` through. Its first use of `rcx` copies the action to `r13`, and it branches on the action's command id, the `uint32` at `+0x08` (`X4_SCRIPT_ACTION_OFFSET_COMMAND_ID`):

| Command id | Action | The searching ship |
|------------|--------|--------------------|
| `0x7B0` (`X4_SCRIPT_COMMAND_FIND_SELL_OFFER`) | `find_sell_offer` | buys |
| `0x74E` (`X4_SCRIPT_COMMAND_FIND_BUY_OFFER`) | `find_buy_offer` | sells |

Both branches search the offers in range and test each candidate partner against the ship's trade list (`match_use_blacklist`) before the handler returns, so a check made while the handler runs belongs to the side its command id names.

A trade itself is resolved from a handle through the order id map, `XLib::AutoIDMap<U::OrderIDBase>` at the slot `0x146CF1750` (`X4_RVA_ORDER_ID_MAP`): `AutoIDMap_Lookup(map, handle, 2)` (`0x14033D420`) returns the trade, which holds its seller's component id at `+0x10` and its buyer's at `+0x18` (`X4_TRADE_OFFSET_SELLER`, `X4_TRADE_OFFSET_BUYER`). A trade order carries its trade handle as the payload of its first parameter (`+0x58`, payload at `+0x08`).

---

## 6. The writer and the reader

The registry is saved and loaded through one pair of functions, each called once by the universe's save and load with the registry:

- `BlacklistData_Export(registry, parent)` (`0x1403725E0`) writes one `<blacklist>` element (id `0x67`) per record under `parent`. Each node is allocated through `GameAlloc` (0xF8 bytes), carries the list's `id` (`0x1FE`), `type`, `name`, its macros, its `<owner>` with `factions` (ids joined by a space) and `whitelist` (written as `"1"`, and only when on), and its hazardous flag, and is appended to `parent`'s children. The writer rebuilds every element from its record on each save. Node building is described in `docs/rev/SAVE_XML.md` §5.
- `BlacklistData_Import(registry, lists)` (`0x140371D20`) reads every `<blacklist>` child of `lists` into a record, matching attribute and child ids against the SaveXML numbers it knows and skipping any other node; it keeps the first `<owner>` only. It rebuilds each record's check through `BlacklistFilterBuild`. Its error strings begin `BlacklistData::Import()`.

Element and attribute names in a save are numbers from the SaveXML table; see `docs/rev/SAVE_XML.md`.
