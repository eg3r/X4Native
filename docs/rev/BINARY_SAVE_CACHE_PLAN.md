# Binary Save Cache: Implementation Plan

## STATUS: INVALIDATED BY IN-GAME TESTING

**In-game measurement (2026-04-11)** proved the core assumption wrong:

| Phase | Estimated | Measured | Notes |
|-------|-----------|----------|-------|
| XML text parsing (`libxml2_ReadFile`) | 7-21 seconds (35% of load) | **184 ms** | 0.4% of total load |
| Component creation + state restore | ~30s | **~45s** | This IS the bottleneck |
| Total save_006 load | ~45s | ~45s | |

The game's embedded libxml2 parses 329 MB of XML in **184ms** -- it likely uses SAX/streaming
internally, not DOM construction. The entire 45-second load time is spent in `GameClass::Load`
creating game objects and restoring state, not in XML parsing.

**A binary DOM cache would save 184ms out of 45 seconds. The project is not viable.**

The hook infrastructure and tooling built during this research remain useful:
- `libxml2_ReadFile` hook at RVA `0x012965B0` works correctly (5500+ calls captured)
- Save URI format: `personal:///save/<name>?ext=xml.gz%20xml`
- The returned object is a game wrapper (NOT raw xmlDocPtr) with vtable at +0x38..+0x60
- The Rust `x4bin-writer` tool works and can convert saves in 2.4s

---

*Original plan below kept for reference.*

---

**Original Goal**: Skip XML text parsing during savegame load by pre-building the libxml2
DOM tree from a compact binary cache file.

**Original Expected speedup**: 15-40x faster on the XML parsing phase. ~33% total load
time reduction (20s -> 13s early game, 60s -> 40s late game).

**Effort**: ~3-4 weeks. No RE of game internals needed -- only libxml2 DOM construction.

---

## 1. Architecture Overview

```
SAVE PATH (async, zero game impact):
  Game saves .xml.gz normally
  → Companion process detects new save
  → Reads .xml.gz, decompresses, parses XML
  → Writes .x4bin binary cache alongside

LOAD PATH (one hook in DLL):
  GameClass::Load calls XMLManager::ParseFromMemory
  → OUR HOOK checks: .x4bin exists and matches .xml.gz hash?
  → YES: construct xmlDoc from binary (~450ms), return it
  → NO:  call original ParseFromMemory (~7-21s), return normally
  
  GameClass::Load proceeds with two-pass DOM traversal (unchanged)
```

---

## 2. Hook Target (Confirmed via RE)

### The Exact Function to Hook

```
XMLManager::ParseFromMemory
Address:  0x1411E9E60
Signature: xmlDocPtr __fastcall(XMLManager* this, const void* buffer, int size, int url_flag)
Returns:   xmlDocPtr (pointer to xmlDoc struct, or NULL on failure)
```

**Call chain from GameClass::Load**:

```
GameClass::Load                         0x1409AC8E0
  │
  ├─ sub_1411ECFD0  (signature verify)  0x1411ECFD0  @ call site 0x1409AD18F
  │    Returns: int status code (not a buffer!)
  │
  └─ XMLDocBase<SaveXMLNode>::Load      0x140A1DB50  @ call site 0x1409AD2F1
       │
       └─ XMLManager::LoadWithPatches   0x1411EA630  @ call site 0x140A1E67A
            │
            └─ XMLManager::LoadDoc      0x1411E9F90  @ call site 0x1411EA68D
                 │
                 └─► XMLManager::ParseFromMemory  0x1411E9E60  ◄── HOOK HERE
                      │
                      ├─ xmlNewParserCtxt          0x14129A5C0
                      ├─ xmlNewInputFromMemory     0x1412B3960
                      ├─ xmlNewIOInputStream       0x141299670
                      ├─ xmlParseDocument          0x1412B1330  ← THE SLOW PART
                      └─ xmlFreeParserCtxt         0x14129A1D0
```

**Verification after return** (in GameClass::Load at `0x1409AD312`):
```c
if (!xmlDocPtr || *(DWORD*)(xmlDocPtr + 12) != 1009)  // XML_DOCUMENT_NODE = 1009
    // error: invalid savegame
```

So our constructed `xmlDoc` MUST have `type = 1009` at offset +12.

### Alternative Hook Points

| Function | Address | Pros | Cons |
|----------|---------|------|------|
| `ParseFromMemory` | `0x1411E9E60` | Cleanest -- replaces parse only. 4 params. | Need to detect "is this a savegame parse?" |
| `XMLDocBase::Load` | `0x140A1DB50` | Avoids file I/O entirely. | Complex function, harder to replicate fallback. |
| `LoadDoc` | `0x1411E9F90` | Thin wrapper around ParseFromMemory. | Same as above but simpler. |

**Recommended**: Hook `ParseFromMemory` (`0x1411E9E60`). It's called for ALL XML files,
so we filter: only intercept when the buffer starts with `<savegame>` (or when we detect
savegame context from the call stack/filename).

### Other Key Addresses

| Function | Address | Purpose |
|----------|---------|---------|
| `xmlFreeDoc` | `0x14128A300` | Frees the DOM tree -- our nodes must survive this |
| `xmlNewParserCtxt` | `0x14129A5C0` | Creates parser context (0x328 bytes) |
| `xmlParseDocument` | `0x1412B1330` | The actual recursive descent parser (the slow part) |
| `xmlFreeParserCtxt` | `0x14129A1D0` | Frees parser context |
| GameClass::Load | `0x1409AC8E0` | Main load function (consumer of our xmlDoc) |
| UniverseClass::Import | `0x14089AE80` | First-pass component creation (reads from DOM) |

---

## 3. libxml2 Struct Layouts

These are stable public API from `libxml2/tree.h`. All offsets x64.

### xmlDoc (returned from our hook)

```c
struct _xmlDoc {
    void           *_private;      // 0x00  = NULL
    xmlElementType  type;          // 0x08  = XML_DOCUMENT_NODE (9) 
    char           *name;          // 0x10  = NULL (or filename)
    xmlNode        *children;      // 0x18  = root element (<savegame>)
    xmlNode        *last;          // 0x20  = root element
    xmlNode        *parent;        // 0x28  = NULL
    xmlNode        *next;          // 0x30  = NULL
    xmlNode        *prev;          // 0x38  = NULL
    xmlDoc         *doc;           // 0x40  = self-pointer
    int             compression;   // 0x48  = 0
    int             standalone;    // 0x4C  = -1
    xmlDtd         *intSubset;     // 0x50  = NULL
    xmlDtd         *extSubset;     // 0x58  = NULL
    xmlNs          *oldNs;         // 0x60  = NULL
    const xmlChar  *version;       // 0x68  = "1.0"
    const xmlChar  *encoding;      // 0x70  = "UTF-8"
    // ... more fields, all NULL/0 for our purposes
};
```

**CRITICAL**: GameClass::Load checks `*(DWORD*)(doc + 12) == 1009`. But `XML_DOCUMENT_NODE`
is defined as `9` in libxml2. The value 1009 is suspicious -- it might be offset +12 BYTES
not +12 DWORDS, i.e. offset 0x0C which overlaps the `type` field at 0x08 (int).

Need to verify: is `type` at offset 0x08 (4 bytes, then padding) or is the struct packed?
On x64, `xmlElementType` is an `int` (4 bytes) at offset 0x08. The next field `name` is at
0x10 (8 bytes alignment). So offset 0x0C is padding. **The check might actually be
`*(int*)(doc + 0x08) == 9`** (i.e., `doc->type == XML_DOCUMENT_NODE`).

**Action item**: Verify at runtime by hooking `ParseFromMemory`, letting original run,
inspecting returned pointer layout.

### xmlNode (the main element type)

```c
struct _xmlNode {                  // sizeof ~0x70 on x64
    void           *_private;      // 0x00  = NULL
    xmlElementType  type;          // 0x08  = XML_ELEMENT_NODE (1)
    const xmlChar  *name;          // 0x10  = tag name (e.g., "component")
    xmlNode        *children;      // 0x18  = first child element
    xmlNode        *last;          // 0x20  = last child element
    xmlNode        *parent;        // 0x28  = parent node
    xmlNode        *next;          // 0x30  = next sibling
    xmlNode        *prev;          // 0x38  = previous sibling
    xmlDoc         *doc;           // 0x40  = owning document
    xmlNs          *ns;            // 0x48  = NULL (saves don't use namespaces)
    xmlChar        *content;       // 0x50  = NULL (elements have no text content)
    xmlAttr        *properties;    // 0x58  = first attribute (linked list)
    xmlNs          *nsDef;         // 0x60  = NULL
    void           *psvi;          // 0x68  = NULL
    unsigned short  line;          // 0x70  = 0
    unsigned short  extra;         // 0x72  = 0
};
```

### xmlAttr (attribute on an element)

```c
struct _xmlAttr {                  // sizeof ~0x60 on x64
    void           *_private;      // 0x00  = NULL
    xmlElementType  type;          // 0x08  = XML_ATTRIBUTE_NODE (2)
    const xmlChar  *name;          // 0x10  = attribute name (e.g., "class")
    xmlNode        *children;      // 0x18  = text node with value
    xmlNode        *last;          // 0x20  = same text node
    xmlNode        *parent;        // 0x28  = owning element
    xmlAttr        *next;          // 0x30  = next attribute
    xmlAttr        *prev;          // 0x38  = previous attribute
    xmlDoc         *doc;           // 0x40  = owning document
    xmlNs          *ns;            // 0x48  = NULL
    xmlAttributeType atype;        // 0x50  = 0
    void           *psvi;          // 0x58  = NULL
};
```

### xmlNode for text (attribute value carrier)

```c
// Each attribute value is stored as a text node child of the xmlAttr
struct _xmlNode {                  // text node
    void           *_private;      // 0x00  = NULL
    xmlElementType  type;          // 0x08  = XML_TEXT_NODE (3)
    const xmlChar  *name;          // 0x10  = xmlStringText ("text")
    xmlNode        *children;      // 0x18  = NULL
    xmlNode        *last;          // 0x20  = NULL
    xmlNode        *parent;        // 0x28  = owning xmlAttr (cast)
    xmlNode        *next;          // 0x30  = NULL
    xmlNode        *prev;          // 0x38  = NULL
    xmlDoc         *doc;           // 0x40  = owning document
    xmlNs          *ns;            // 0x48  = NULL
    xmlChar        *content;       // 0x50  = VALUE STRING (e.g., "ship_s")
    // ... rest NULL
};
```

**Key**: `xmlGetProp(node, "class")` walks `node->properties` linked list, finds
attr with `name == "class"`, returns `xmlNodeGetContent(attr->children)` which returns
`attr->children->content`. Our construction must build this 3-layer structure:
element -> attr -> text_node -> content string.

---

## 4. Binary Format Specification

### File Layout

```
Offset  Size    Field
─────────────────────────────────────────
0x00    4       magic: "X4BN"
0x04    4       format_version: u32 (1)
0x08    8       source_hash: u64 (xxhash of .xml.gz file)
0x10    4       node_count: u32 (number of element nodes)
0x14    4       text_node_count: u32 (number of text nodes for attr values)
0x18    4       attr_count: u32 (number of attributes)
0x1C    4       string_table_offset: u32 (byte offset to string table)
0x20    4       string_table_size: u32 (bytes)
0x24    4       node_table_offset: u32
0x28    4       attr_table_offset: u32
0x2C    4       text_table_offset: u32
0x30    16      reserved

String Table (deduplicated, null-terminated, packed):
  "savegame\0component\0class\0ship_s\0macro\0..."
  Each string referenced by byte offset from table start.

Node Table (20 bytes per record):
  u32   name_offset      (into string table)
  u32   first_child_idx  (node index, 0xFFFFFFFF = none)
  u32   next_sibling_idx (node index, 0xFFFFFFFF = none)
  u32   first_attr_idx   (attr index, 0xFFFFFFFF = none)
  u16   child_count
  u16   flags            (reserved)

Attr Table (12 bytes per record):
  u32   name_offset      (into string table)
  u32   value_offset     (into string table)
  u32   next_attr_idx    (attr index, 0xFFFFFFFF = last)
```

### Estimated Sizes (save_006)

| Section | Records | Bytes per | Total |
|---------|---------|-----------|-------|
| Header | 1 | 48 | 48 B |
| String table | ~80K unique strings | ~20 bytes avg | ~1.6 MB |
| Node table | ~420K elements | 20 bytes | 8.4 MB |
| Attr table | ~2M attributes | 12 bytes | 24 MB |
| **Total** | | | **~34 MB** |

vs. 29 MB compressed XML / 329 MB uncompressed XML.

---

## 5. Implementation Tasks

### Task 1: Verify libxml2 Struct Layout (DLL, 2 days)

**Goal**: Confirm that the game's statically-linked libxml2 uses the standard xmlNode layout.

**Method**: Hook `ParseFromMemory` (`0x1411E9E60`), let the original function run,
then inspect the returned `xmlDocPtr`:

```cpp
// In our hook:
xmlDocPtr result = original_ParseFromMemory(this_ptr, buffer, size, url_flag);
if (result) {
    // Verify xmlDoc layout
    assert(*(int*)((char*)result + 0x08) == 9);  // XML_DOCUMENT_NODE
    
    xmlNodePtr root = *(xmlNodePtr*)((char*)result + 0x18); // doc->children
    assert(root != nullptr);
    assert(*(int*)((char*)root + 0x08) == 1);    // XML_ELEMENT_NODE
    
    const char* name = *(const char**)((char*)root + 0x10); // root->name
    log("Root element name: %s", name);  // should be "savegame"
    
    // Walk first few children to verify pointer wiring
    xmlNodePtr child = *(xmlNodePtr*)((char*)root + 0x18); // root->children
    while (child) {
        const char* cname = *(const char**)((char*)child + 0x10);
        log("  Child: %s", cname);  // should be "info", "universe", etc.
        child = *(xmlNodePtr*)((char*)child + 0x30); // child->next
    }
    
    // Verify attribute access
    xmlAttrPtr prop = *(xmlAttrPtr*)((char*)root + 0x58); // root->properties
    // ... walk and log
}
return result;
```

**Deliverable**: Confirmed offset table. Go/no-go for the project.

### Task 2: Binary Writer (Companion/Rust, 1 week)

**Goal**: Watch save directory, convert .xml.gz to .x4bin.

**Implementation**:
1. `inotify`/`ReadDirectoryChangesW` watch on save folder
2. When .xml.gz modified: decompress with `flate2`
3. Parse XML with `quick-xml` (SAX streaming -- no DOM needed)
4. Build string table (deduplicated via `HashMap<String, u32>`)
5. Build node + attr tables (depth-first traversal)
6. Write .x4bin file atomically (write to .tmp, rename)
7. Compute xxhash of .xml.gz for cache invalidation

**Key consideration**: The XML `<?xml version="1.0" encoding="UTF-8"?>` processing
instruction should be stored as metadata, not as a node.

### Task 3: DOM Constructor (DLL/C++, 1 week)

**Goal**: Read .x4bin, construct valid xmlDoc/xmlNode tree.

**Implementation**:
```cpp
xmlDocPtr BuildDOMFromBinary(const char* x4bin_path) {
    // 1. Memory-map or read the .x4bin file
    auto data = ReadFile(x4bin_path);
    auto header = (X4BinHeader*)data.data();
    
    // Validate magic + source hash
    if (memcmp(header->magic, "X4BN", 4) != 0) return nullptr;
    
    // 2. Build string pointer table (offsets → actual char* into mapped data)
    //    OR strdup all strings if we need them to survive munmap
    const char* str_base = data.data() + header->string_table_offset;
    
    // 3. Allocate all xmlNodes in one pass
    auto nodes = new xmlNode*[header->node_count];
    for (u32 i = 0; i < header->node_count; i++) {
        nodes[i] = (xmlNode*)malloc(sizeof(xmlNode));  // must use malloc!
        memset(nodes[i], 0, sizeof(xmlNode));
        nodes[i]->type = XML_ELEMENT_NODE;  // 1
    }
    
    // 4. Allocate all xmlAttrs + text nodes
    auto attrs = new xmlAttr*[header->attr_count];
    auto texts = new xmlNode*[header->attr_count]; // one text node per attr
    for (u32 i = 0; i < header->attr_count; i++) {
        attrs[i] = (xmlAttr*)malloc(sizeof(xmlAttr));
        texts[i] = (xmlNode*)malloc(sizeof(xmlNode));
        memset(attrs[i], 0, sizeof(xmlAttr));
        memset(texts[i], 0, sizeof(xmlNode));
        attrs[i]->type = XML_ATTRIBUTE_NODE;  // 2
        texts[i]->type = XML_TEXT_NODE;        // 3
    }
    
    // 5. Create xmlDoc
    auto doc = (xmlDoc*)malloc(sizeof(xmlDoc));
    memset(doc, 0, sizeof(xmlDoc));
    doc->type = XML_DOCUMENT_NODE;  // 9
    doc->doc = doc;                 // self-pointer
    doc->version = xmlStrdup("1.0");
    doc->encoding = xmlStrdup("UTF-8");
    doc->standalone = -1;
    
    // 6. Wire node tree (parent/child/sibling pointers)
    NodeRecord* node_table = (NodeRecord*)(data.data() + header->node_table_offset);
    for (u32 i = 0; i < header->node_count; i++) {
        auto& rec = node_table[i];
        nodes[i]->name = xmlStrdup(str_base + rec.name_offset);
        nodes[i]->doc = doc;
        
        if (rec.first_child_idx != 0xFFFFFFFF)
            nodes[i]->children = nodes[rec.first_child_idx];
        if (rec.next_sibling_idx != 0xFFFFFFFF)
            nodes[i]->next = nodes[rec.next_sibling_idx];
        if (rec.first_attr_idx != 0xFFFFFFFF)
            nodes[i]->properties = attrs[rec.first_attr_idx];
    }
    
    // 7. Wire parent + prev pointers (reverse pass)
    for (u32 i = 0; i < header->node_count; i++) {
        xmlNode* child = nodes[i]->children;
        xmlNode* prev = nullptr;
        while (child) {
            child->parent = nodes[i];
            child->prev = prev;
            nodes[i]->last = child;
            prev = child;
            child = child->next;
        }
    }
    
    // 8. Wire attributes
    AttrRecord* attr_table = (AttrRecord*)(data.data() + header->attr_table_offset);
    for (u32 i = 0; i < header->attr_count; i++) {
        auto& rec = attr_table[i];
        attrs[i]->name = xmlStrdup(str_base + rec.name_offset);
        attrs[i]->doc = doc;
        
        // Text node carries the value
        texts[i]->content = xmlStrdup(str_base + rec.value_offset);
        texts[i]->doc = doc;
        texts[i]->parent = (xmlNode*)attrs[i];
        attrs[i]->children = texts[i];
        attrs[i]->last = texts[i];
        
        if (rec.next_attr_idx != 0xFFFFFFFF)
            attrs[i]->next = attrs[rec.next_attr_idx];
    }
    
    // 9. Wire attr parent + prev pointers
    for (u32 i = 0; i < header->node_count; i++) {
        xmlAttr* attr = nodes[i]->properties;
        xmlAttr* prev = nullptr;
        while (attr) {
            attr->parent = nodes[i];
            attr->prev = prev;
            prev = attr;
            attr = attr->next;
        }
    }
    
    // 10. Set document root
    doc->children = nodes[0];  // <savegame> is root
    doc->last = nodes[0];
    nodes[0]->parent = (xmlNode*)doc;
    
    delete[] nodes;
    delete[] attrs;
    delete[] texts;
    return doc;
}
```

**Note**: `xmlStrdup` must be the GAME'S `xmlStrdup` (which wraps `malloc`), not our own.
Since libxml2 is statically linked, we need to either:
- Use `_strdup` from the CRT (same allocator)
- Or find the game's `xmlStrdup` address and call it

Since both the game and our DLL link to the same MSVC CRT, `_strdup` should work.
`xmlFreeDoc` calls `xmlFree` which calls `free` -- compatible with our `malloc`.

### Task 4: Hook Integration (DLL/C++, 2-3 days)

**Goal**: Wire the hook into X4Native's extension system.

```cpp
// Original function pointer
typedef void* (__fastcall *ParseFromMemory_t)(void* this_ptr, const void* buffer, 
                                               int size, int url_flag);
ParseFromMemory_t orig_ParseFromMemory = nullptr;

void* __fastcall Hook_ParseFromMemory(void* this_ptr, const void* buffer, 
                                       int size, int url_flag) {
    // Detect savegame: check if buffer starts with "<?xml" followed by "<savegame>"
    const char* buf = (const char*)buffer;
    bool is_savegame = size > 30 && 
                       memmem(buf, min(size, 200), "<savegame>", 10) != nullptr;
    
    if (is_savegame) {
        // Try binary cache
        std::string x4bin_path = GetCurrentSavePath() + ".x4bin";  // derive from context
        if (FileExists(x4bin_path) && ValidateCache(x4bin_path, buffer, size)) {
            void* doc = BuildDOMFromBinary(x4bin_path.c_str());
            if (doc) {
                x4n::log::info("Loaded savegame from binary cache");
                return doc;
            }
            x4n::log::info("Binary cache failed, falling back to XML parse");
        }
    }
    
    return orig_ParseFromMemory(this_ptr, buffer, size, url_flag);
}
```

**Cache validation**: compute xxhash64 of the decompressed XML buffer (we have it right
here in the hook!), compare against the hash stored in .x4bin header. If mismatch, fall
back to normal parse and signal companion to rebuild cache.

### Task 5: Testing (1 week)

1. **Struct layout verification**: Load a save normally, inspect DOM via hook, log all
   field offsets. Compare against expected layout.

2. **Small save test**: Create a minimal save (new game, immediate save). Convert to
   .x4bin, load via cache. Verify game state is identical.

3. **Large save test**: Use save_001 (93 MB compressed). Compare load time with and
   without cache. Verify no crashes, no missing data, no gameplay differences.

4. **Cache invalidation test**: Modify the .xml.gz (re-save in game), verify cache is
   invalidated and rebuilt.

5. **Stress test**: Load/save 10 times with cache. Check for memory leaks (our nodes
   must be freed correctly by xmlFreeDoc).

6. **Game update test**: Simulate a version change (modify source hash). Verify graceful
   fallback to normal XML parse.

---

## 6. Risk Register

| # | Risk | Impact | Likelihood | Mitigation |
|---|------|--------|------------|------------|
| 1 | xmlNode layout differs from standard | **Blocker** | Low | Task 1 verifies at runtime. No-go if wrong. |
| 2 | xmlFreeDoc crashes on our nodes | **Blocker** | Medium | Use `malloc`/`_strdup` (same CRT). Test early. |
| 3 | Game reads xmlNode fields we don't populate | Corrupt load | Low | Zero-initialize all structs. Only `type`, `name`, `children`, `next`, `prev`, `parent`, `doc`, `properties`, `content` matter. |
| 4 | Attribute access uses xmlHashTable instead of linked list | Wrong data | Low | Verify: hook `xmlGetProp`, check if it walks `properties` list. |
| 5 | Game uses xmlNode->line for error reporting | Minor glitch | Medium | Set `line` to 0. Error messages will show line 0 instead of correct line. |
| 6 | Game allocates into the DOM tree (adds nodes during load) | Crash | Low | Unlikely -- load is read-only traversal. Monitor via hook. |
| 7 | String encoding mismatch (UTF-8 BOM, etc.) | Corrupt text | Low | Always strdup with null terminator. No BOM. |
| 8 | Performance worse than expected | Wasted effort | Low | Binary read (~50ms) + node alloc (~120ms) + string copy (~150ms) + wiring (~80ms) = ~400ms total. Hard to be slower than XML parse. |

---

## 7. Success Criteria

- [ ] Save_006 (29 MB gz, 329 MB XML) loads in <15s with cache vs ~20s without
- [ ] Save_001 (93 MB gz, ~1 GB XML) loads in <42s with cache vs ~60s without
- [ ] No gameplay differences detected (faction relations, ship counts, NPC skills identical)
- [ ] `xmlFreeDoc` completes without crash
- [ ] Cache automatically invalidated when save file changes
- [ ] Graceful fallback when cache missing or corrupt

---

## 8. File Locations

This is a standalone X4Native example extension -- lives in `examples/save_cache/` alongside
the other examples (`hello`, `event_probe`, `entity_inspector`, etc.).

```
X4Native/
  examples/
    save_cache/                      ← NEW example extension
      CMakeLists.txt                 ← Builds x4native_save_cache.dll
      content.xml                    ← X4 extension manifest
      x4native.json                  ← X4Native config (priority, hooks)
      save_cache.cpp                 ← Main: hook setup, ParseFromMemory intercept
      dom_builder.h / .cpp           ← xmlNode/xmlDoc construction from binary
      binary_format.h                ← X4BN format structs + reader
      struct_verify.h / .cpp         ← Runtime xmlNode layout verification (Task 1)
      tools/
        x4bin_writer.py              ← Standalone script: .xml.gz → .x4bin converter
                                        (or Rust CLI, see below)
```

**Why a Python/standalone tool for the writer**: The binary writer runs outside the game --
it reads .xml.gz files and produces .x4bin cache files. It doesn't need the X4Native SDK.
Options:
- **Python script** (`x4bin_writer.py`): fastest to prototype, uses `lxml` or `xml.etree`
- **Rust CLI** (`x4bin-writer/`): faster for large saves, could be a separate cargo project
- Either can be invoked manually or via a file watcher

The DLL only needs the **reader/constructor** side (read .x4bin, build DOM).

### Deployment

```
<X4>/extensions/
  save_cache/
    x4native_save_cache.dll          ← Built DLL (the hook)
    content.xml
    x4native.json

<save_dir>/
  save_006.xml.gz                    ← Original save (untouched)
  save_006.x4bin                     ← Binary cache (generated by tool)
  autosave_01.xml.gz
  autosave_01.x4bin
```

The user runs the converter tool on their save folder. The DLL picks up .x4bin files
automatically at load time.

---

## 9. Sequence of Work

```
Week 1:  Task 1 (verify struct layout) + Task 2 start (binary writer tool)
         GO/NO-GO decision at end of Week 1 based on struct verification

Week 2:  Task 2 finish (binary writer) + Task 3 (DOM constructor in DLL)

Week 3:  Task 4 (hook integration) + Task 5 start (testing)

Week 4:  Task 5 finish (testing) + fixes + optimization
```

---

*Standalone X4Native example extension. No dependency on X4Strategos.*
*Depends on: docs/rev/SAVE_LOAD.md research (complete).*
