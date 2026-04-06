# class_dump — X4Native Runtime Data Extractor

One-shot research extension that dumps X4's internal type registries to CSV files.
Run once per game build, copy CSVs to `X4Native/reference/`, regenerate SDK headers.

Companion extension: `event_probe` (scans builder code for struct layouts).

## What It Dumps

| Output | Source | When |
|--------|--------|------|
| `event_type_ids.csv` | RTTI scan + PropertyChange table | Immediate (X4N_EXTENSION init) |
| `class_ids.csv` | `GetComponentClassMatrix()` FFI | On game load |
| `class_matrix.csv` | Same FFI | On game load |

## How To Run

1. Build: `cmake -B build -G "Visual Studio 17 2022" -A x64 && cmake --build build --config Release`
2. Deploy to `<X4>/extensions/x4native_class_dump/`:
   ```
   x4native_class_dump/
     content.xml
     x4native.json
     native/
       x4native_class_dump.dll
   ```
3. Also deploy `event_probe` (same structure, separate extension)
4. Launch X4 — event CSVs are written immediately at startup
5. Load any save — class CSVs are written on game load
6. Disable or remove both extensions after copying files

## Post-Run: Regenerate SDK Headers

```powershell
# Copy CSVs to reference folder
cp <X4>/extensions/x4native_class_dump/event_type_ids.csv  X4Native/reference/
cp <X4>/extensions/x4native_event_probe/event_layouts.csv  X4Native/reference/
cp <X4>/extensions/x4native_class_dump/class_ids.csv       X4Native/reference/

# Regenerate
.\scripts\generate_event_type_ids.ps1      # -> sdk/x4_md_events.h
.\scripts\generate_class_ids.ps1           # -> sdk/x4_game_class_ids.inc

# Or run the full pipeline:
.\scripts\update_references.ps1
```

## When To Re-Run

Every major X4 game update. Both class IDs and event type IDs are compile-time
constants that shift when Egosoft adds/removes classes or events between builds.

Check `reference/game/VERSION` — if the build number changed, re-run.

## How The Event Dump Works

Three data sources merged into `event_type_ids.csv`:

### 1. PropertyChange Type Table (.rdata)

Static table of ~556 entries: `{const char* name, uint32_t type_id, uint32_t pad}`.
Found by scanning for sequential `type_id == index` pattern with known spot-checks.
Short names are the engine's internal identifiers (e.g., `killed`, `factionrelationchanged`).

### 2. RTTI Scan (.rdata COL walk)

Walks MSVC RTTI Complete Object Locators for `Event@U@@` classes.
Reads `vtable[1]` bytes (`mov eax, IMM32; ret`) to extract type IDs.
Produces C++ class names (e.g., `KilledEvent`) + vtable RVAs.

### 3. XSD Event Descriptors (.rdata)

Scans for `"event_*"` string pointers paired with descriptor IDs.
Dumped for reference — the generator matches XSD names algorithmically.

### Output Format

```csv
id,rtti_name,short_name,vtable_rva
29,AttackedEvent,attacked,0x2b48300
55,BuildFinishedEvent,buildfinished,0x2b4a398
233,KilledEvent,killed,0x2b4a168
```

## Addresses That Need RE (version_db)

One address per game build, in `native/version_db/internal_functions.json`:

### EventQueue_InsertOrDispatch

Central MD event dispatch function — all events funnel through it.

| Build | RVA |
|-------|-----|
| 900-603098 | `0x00958390` |

**How to find:**
1. Search for RTTI string `"EventSource@U@@"`, find vtable via COL
2. Find the function with 1245+ callers (central dispatch funnel)
3. Or: byte pattern `48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 20`
4. Verify: 4 args (source, event, timestamp, immediate), dispatches or queues
