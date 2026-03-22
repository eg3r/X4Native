# Scripts

## update_references.ps1 (master script)

Runs the complete reference extraction pipeline after a game update. This is the main entry point — it calls the three scripts below in order.

```powershell
.\scripts\update_references.ps1
```

Then review and commit:

```powershell
git diff reference/
git add reference/
git commit -m "reference: update to v<version>"
```

### Prerequisites

- **X4: Foundations** — The game itself
- **X Tools** — Install via Steam (Library → Tools → "X Tools"). Provides `XRCatTool.exe`
- **VS2022 BuildTools** — Provides `dumpbin.exe` (auto-detected via vswhere)

All paths are auto-detected. Override with `-GameDir`, `-ToolDir`, or `-DumpbinPath`.

### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `-GameDir` | Auto-detect via Steam registry | X4 install path |
| `-ToolDir` | Auto-detect via Steam registry | X Tools install path |
| `-DumpbinPath` | Auto-detect via vswhere | Path to dumpbin.exe |
| `-SkipGameFiles` | — | Skip game file extraction |
| `-SkipExports` | — | Skip PE export extraction |
| `-SkipFFI` | — | Skip FFI extraction |
| `-SkipHeaders` | — | Skip C header generation |
| `-SkipGameData` | — | Skip game data table generation |

### Pipeline Output

| Step | Output | Description |
|------|--------|-------------|
| Game files | `reference/game/` | Lua, XML, XSD from cat/dat archives |
| PE exports | `reference/x4_exports.txt` | Named C function exports from X4.exe |
| FFI parsing | `reference/x4_ffi_raw.txt` | Raw FFI cdef content from Lua files |
| FFI parsing | `reference/x4_struct_names.txt` | Unique struct type names |
| FFI parsing | `reference/x4_ffi_summary.txt` | Cross-reference statistics |
| Headers | `sdk/x4_game_types.h` | Struct/typedef definitions (dependency-ordered) |
| Headers | `sdk/x4_game_functions.h` | Function declarations + untyped export comments |
| Game data | `extension/ui/x4n_game_data.lua` | Room macro/type tables + lookup functions (Lua) |

---

## extract_game_files.ps1

Extracts text-based reference files (Lua, XML, XSD) from X4's cat/dat archives into `reference/game/`. Binary assets (textures, models, audio) are excluded.

### What it extracts

| Directory | Contents |
|-----------|----------|
| `md/` | Mission Director cue scripts |
| `aiscripts/` | AI behavior trees |
| `libraries/` | Game data (wares, ships, weapons, factions) |
| `ui/` | Lua UI scripts and XSD schemas |
| `index/` | File index manifests |
| `extensions/` | DLC-specific overrides (ego_dlc_* only) |
| `VERSION` | Game build number |

### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `-GameDir` | Auto-detect via Steam registry | X4 install path |
| `-ToolDir` | Auto-detect via Steam registry | X Tools install path |
| `-OutDir` | `reference/game/` | Output directory |

---

## extract_exports.ps1

Dumps the PE export table from X4.exe using `dumpbin /exports` (MSVC toolchain). Produces `reference/x4_exports.txt` with all named function exports and their RVAs.

### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `-GameDir` | Auto-detect via Steam registry | X4 install path |
| `-DumpbinPath` | Auto-detect via vswhere | Path to dumpbin.exe |

---

## extract_ffi.ps1

Parses `ffi.cdef[[ ... ]]` blocks from all Lua files in `reference/game/ui/` and produces:
- `reference/x4_ffi_raw.txt` — All raw cdef content
- `reference/x4_struct_names.txt` — Sorted unique struct type names
- `reference/x4_ffi_summary.txt` — Statistics and cross-reference with PE exports

Requires `extract_game_files.ps1` and `extract_exports.ps1` to have been run first.

---

## generate_headers.ps1

Parses `reference/x4_ffi_raw.txt` to extract types and function signatures, cross-references with `reference/x4_exports.txt`, and generates C headers in `sdk/`:

- `sdk/x4_game_types.h` — Handle typedefs + 283 struct definitions (dependency-ordered via topological sort)
- `sdk/x4_game_functions.h` — 2,051 typed function declarations + 308 untyped exports as comments (with sibling hints)

Filters out 25 LuaJIT-internal structs (PE/ELF/Mach-O parser types). Applies fixups for C++ reserved words (`bool default` → `bool defaultorder`) and normalizes empty parameter lists `()` → `(void)`.

Requires `extract_ffi.ps1` and `extract_exports.ps1` to have been run first.

---

## generate_game_data.ps1

Parses `reference/game/libraries/` XML files and `sdk/x4_manual_types.h` to generate game data lookup tables:

- `extension/ui/x4n_game_data.lua` — Lua tables for room macro/type lookups + convenience functions

### What it generates

| Table (Lua) | Description |
|---|---|
| `x4n_ROOM_MACRO_TO_TYPE` | Room macro name -> roomtype string (e.g. `room_gen_bar_01_macro` -> `bar`) |
| `x4n_CORRIDOR_MACROS` | Set of known corridor macros |
| `x4n_ROOMTYPE_INDEX` | Roomtype string -> enum index (from `X4RoomType` in `x4_manual_types.h`) |
| `x4n_is_corridor(macro)` | Exact match + substring fallback for corridor detection |
| `x4n_lookup_room(macro)` | Returns `roomtype, index` with exact match + substring fallback |

Parses rooms.xml + roomgroups.xml (base game + all DLCs). Cross-references with `common.xsd` for drift detection. Reads the `X4RoomType` enum from `x4_manual_types.h` as the single source of truth for roomtype indices.

Requires `extract_game_files.ps1` and `generate_headers.ps1` to have been run first (needs `reference/game/` and `sdk/x4_manual_types.h`).

---

## extract_game_files.ps1 (standalone usage)

```powershell
.\scripts\extract_game_files.ps1
.\scripts\extract_game_files.ps1 -GameDir "D:\Games\X4 Foundations"
```
