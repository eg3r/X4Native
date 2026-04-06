// ---------------------------------------------------------------------------
// x4native_event_probe — auto-discovers MD event struct layouts by scanning
// builder function machine code at runtime.
//
// PREREQUISITE: class_dump must have run first to produce event_type_ids.csv
// with vtable RVAs.
//
// PROCESS:
//   1. Reads event_type_ids.csv for (type_id, vtable_rva) pairs
//   2. For each event type, scans .text section for code references to the
//      vtable address (builder functions that write `mov [reg], vtable_addr`)
//   3. Parses each builder's machine code to extract:
//      - Allocation size (from `mov ecx, SIZE` before `call GameAlloc`)
//      - Field store offsets + sizes (from `mov [reg+OFF], src` patterns)
//   4. Writes event_layouts.csv with discovered layouts
//
// OUTPUT:
//   event_layouts.csv — id,alloc_size,fields
//     fields = semicolon-separated "offset:size" pairs (e.g. "24:4;32:8;40:4")
//
// WHY CODE SCANNING:
//   The event builder functions follow predictable MSVC codegen patterns.
//   Each builder allocates via GameAlloc, sets the vtable, then stores fields
//   at known offsets. Parsing these patterns is equivalent to what IDA does
//   but fully automated — no manual RE per game build.
// ---------------------------------------------------------------------------
#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <windows.h>

static int g_sub_loaded = 0;

struct FieldInfo {
    uint32_t offset;  // offset from event base (e.g. 0x18)
    uint32_t size;    // 1, 2, 4, or 8 bytes
};

struct EventLayout {
    uint32_t type_id;
    uint32_t alloc_size;           // total bytes allocated for event object
    std::vector<FieldInfo> fields; // discovered field stores (sorted by offset)
};

// ---------------------------------------------------------------------------
// x64 instruction pattern matchers
// ---------------------------------------------------------------------------

// Check for `mov [reg+disp8], src_reg` (32-bit store: opcode 89 with ModR/M byte)
// Returns true and sets offset/size if matched.
static bool match_store_disp8_32(const uint8_t* p, uint32_t& out_offset) {
    // 89 ModRM(01 xxx rrr) disp8 — mov [reg+disp8], r32
    if (p[0] == 0x89) {
        uint8_t modrm = p[1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t rm = modrm & 7;
        if (mod == 1 && rm != 4) { // disp8, no SIB
            out_offset = p[2];
            return true;
        }
    }
    return false;
}

// Check for `mov [reg+disp32], src_reg` (32-bit store)
static bool match_store_disp32_32(const uint8_t* p, uint32_t& out_offset) {
    // 89 ModRM(10 xxx rrr) disp32 — mov [reg+disp32], r32
    if (p[0] == 0x89) {
        uint8_t modrm = p[1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t rm = modrm & 7;
        if (mod == 2 && rm != 4) { // disp32, no SIB
            out_offset = *reinterpret_cast<const uint32_t*>(p + 2);
            return true;
        }
    }
    return false;
}

// Check for REX.W + mov [reg+disp8], src_reg (64-bit store)
static bool match_store_disp8_64(const uint8_t* p, uint32_t& out_offset) {
    // 48 89 ModRM(01 xxx rrr) disp8 — mov [reg+disp8], r64
    // Also 4C 89 for r8-r15 source
    if ((p[0] == 0x48 || p[0] == 0x4C) && p[1] == 0x89) {
        uint8_t modrm = p[2];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t rm = modrm & 7;
        if (mod == 1 && rm != 4) {
            out_offset = p[3];
            return true;
        }
    }
    return false;
}

// Check for REX.W + mov [reg+disp32], src_reg (64-bit store)
static bool match_store_disp32_64(const uint8_t* p, uint32_t& out_offset) {
    if ((p[0] == 0x48 || p[0] == 0x4C) && p[1] == 0x89) {
        uint8_t modrm = p[2];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t rm = modrm & 7;
        if (mod == 2 && rm != 4) {
            out_offset = *reinterpret_cast<const uint32_t*>(p + 3);
            return true;
        }
    }
    return false;
}

// Check for `movss [reg+disp8], xmm` (float store)
static bool match_store_float_disp8(const uint8_t* p, uint32_t& out_offset) {
    // F3 0F 11 ModRM(01 xxx rrr) disp8 — movss [reg+disp8], xmmN
    if (p[0] == 0xF3 && p[1] == 0x0F && p[2] == 0x11) {
        uint8_t modrm = p[3];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t rm = modrm & 7;
        if (mod == 1 && rm != 4) {
            out_offset = p[4];
            return true;
        }
    }
    return false;
}

// Check for `mov byte [reg+disp8], imm8` (byte store)
static bool match_store_byte_disp8(const uint8_t* p, uint32_t& out_offset) {
    // C6 ModRM(01 000 rrr) disp8 imm8 — mov byte [reg+disp8], imm8
    if (p[0] == 0xC6) {
        uint8_t modrm = p[1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t reg = (modrm >> 3) & 7;
        uint8_t rm = modrm & 7;
        if (mod == 1 && reg == 0 && rm != 4) {
            out_offset = p[2];
            return true;
        }
    }
    return false;
}

// Check for `mov dword [reg+disp8], imm32` (immediate 32-bit store)
static bool match_store_imm32_disp8(const uint8_t* p, uint32_t& out_offset) {
    // C7 ModRM(01 000 rrr) disp8 imm32 — mov dword [reg+disp8], imm32
    if (p[0] == 0xC7) {
        uint8_t modrm = p[1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t reg = (modrm >> 3) & 7;
        uint8_t rm = modrm & 7;
        if (mod == 1 && reg == 0 && rm != 4) {
            out_offset = p[2];
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Builder scanner
// ---------------------------------------------------------------------------

// Scan a builder function (~first 256 bytes) for allocation size and field stores.
// Returns true if at least the allocation size was found.
static bool scan_builder(const uint8_t* func, size_t max_scan, EventLayout& layout) {
    bool found_alloc = false;

    for (size_t i = 0; i + 10 < max_scan; i++) {
        // Stop at ret (C3) or int3 (CC) — likely end of function
        if (found_alloc && (func[i] == 0xC3 || func[i] == 0xCC)) break;

        // --- Allocation size: mov ecx, IMM32 (B9 xx xx xx xx) before call ---
        if (!found_alloc && func[i] == 0xB9 && func[i + 5] == 0xE8) {
            uint32_t size = *reinterpret_cast<const uint32_t*>(func + i + 1);
            if (size >= 24 && size <= 256) { // reasonable event object size
                layout.alloc_size = size;
                found_alloc = true;
            }
        }
        // Also: mov edx, IMM32 (BA xx xx xx xx) — some builders pass size in edx
        if (!found_alloc && func[i] == 0xBA) {
            uint32_t size = *reinterpret_cast<const uint32_t*>(func + i + 1);
            if (size >= 24 && size <= 256) {
                // Verify it's followed by a call within ~10 bytes
                for (size_t j = i + 5; j < i + 15 && j + 1 < max_scan; j++) {
                    if (func[j] == 0xE8) { // call
                        layout.alloc_size = size;
                        found_alloc = true;
                        break;
                    }
                }
            }
        }

        // --- Field stores: only care about offsets >= 0x18 (payload area) ---
        uint32_t off = 0;

        // 64-bit store [reg+disp8]
        if (match_store_disp8_64(func + i, off) && off >= 0x18 && off < 0x80) {
            layout.fields.push_back({off, 8});
            continue;
        }
        // 64-bit store [reg+disp32]
        if (match_store_disp32_64(func + i, off) && off >= 0x18 && off < 0x80) {
            layout.fields.push_back({off, 8});
            continue;
        }
        // 32-bit store [reg+disp8]
        if (match_store_disp8_32(func + i, off) && off >= 0x18 && off < 0x80) {
            layout.fields.push_back({off, 4});
            continue;
        }
        // 32-bit store [reg+disp32]
        if (match_store_disp32_32(func + i, off) && off >= 0x18 && off < 0x80) {
            layout.fields.push_back({off, 4});
            continue;
        }
        // SIB-addressed stores: [reg+disp8] with SIB byte
        // 89 ModRM(01 xxx 100) SIB disp8 — mov [SIB+disp8], r32
        if (func[i] == 0x89) {
            uint8_t modrm = func[i+1];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod == 1 && rm == 4) { // SIB present
                off = func[i+3]; // disp8 after SIB byte
                if (off >= 0x18 && off < 0x80) {
                    layout.fields.push_back({off, 4});
                    continue;
                }
            }
        }
        // REX.W + SIB store: 48 89 ModRM(01 xxx 100) SIB disp8
        if ((func[i] == 0x48 || func[i] == 0x4C) && func[i+1] == 0x89) {
            uint8_t modrm = func[i+2];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod == 1 && rm == 4) { // SIB present
                off = func[i+4]; // disp8 after SIB byte
                if (off >= 0x18 && off < 0x80) {
                    layout.fields.push_back({off, 8});
                    continue;
                }
            }
        }
        // float store [reg+disp8]
        if (match_store_float_disp8(func + i, off) && off >= 0x18 && off < 0x80) {
            layout.fields.push_back({off, 4}); // float = 4 bytes
            continue;
        }
        // byte store [reg+disp8]
        if (match_store_byte_disp8(func + i, off) && off >= 0x18 && off < 0x80) {
            layout.fields.push_back({off, 1});
            continue;
        }
        // immediate 32-bit store [reg+disp8]
        if (match_store_imm32_disp8(func + i, off) && off >= 0x18 && off < 0x80) {
            layout.fields.push_back({off, 4});
            continue;
        }
    }

    // Merge fields: for duplicate offsets, keep the LARGEST size (prefer u64 over u32)
    if (!layout.fields.empty()) {
        std::sort(layout.fields.begin(), layout.fields.end(),
            [](const FieldInfo& a, const FieldInfo& b) {
                return a.offset < b.offset || (a.offset == b.offset && a.size > b.size);
            });
        // Deduplicate: keep first entry per offset (which is the largest due to sort)
        auto last = std::unique(layout.fields.begin(), layout.fields.end(),
            [](const FieldInfo& a, const FieldInfo& b) { return a.offset == b.offset; });
        layout.fields.erase(last, layout.fields.end());
    }

    return found_alloc || !layout.fields.empty();
}

// ---------------------------------------------------------------------------
// Find builder functions by scanning .text for vtable references
// ---------------------------------------------------------------------------

// A builder writes `lea reg, [rip+disp]` or `mov [reg], vtable_addr`.
// The LEA pattern is: 48 8D 0D/05/15/1D/2D/35/3D disp32 where disp32 resolves to vtable_va.
// The MOV immediate pattern: 48 C7 00 (and variants) with 8-byte immediate... less common on x64.
//
// Most reliable: LEA reg, [rip+disp32] where target == vtable_va.
// Pattern: 48 8D xx yy yy yy yy  (REX.W + LEA + ModRM(00,reg,101=RIP) + disp32)

struct BuilderCandidate {
    uintptr_t func_start;  // estimated function start (scan backwards for common prologues)
};

static std::vector<BuilderCandidate> find_builders_for_vtable(
    uintptr_t vtable_va, uintptr_t text_start, uintptr_t text_end)
{
    std::vector<BuilderCandidate> result;

    for (uintptr_t addr = text_start; addr + 7 <= text_end; addr++) {
        auto* p = reinterpret_cast<const uint8_t*>(addr);

        // LEA reg, [rip+disp32]: 48 8D ModRM(00,reg,101) disp32
        // REX.W prefix: 48 or 4C
        if ((p[0] == 0x48 || p[0] == 0x4C) && p[1] == 0x8D) {
            uint8_t modrm = p[2];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod == 0 && rm == 5) { // RIP-relative
                int32_t disp = *reinterpret_cast<const int32_t*>(p + 3);
                uintptr_t target = addr + 7 + disp; // RIP + disp (instruction is 7 bytes)
                if (target == vtable_va) {
                    // Found a vtable reference. Estimate function start by scanning
                    // backwards for a common function prologue (up to 256 bytes).
                    uintptr_t func_start = addr;
                    for (uintptr_t scan = addr; scan > addr - 256 && scan >= text_start; scan--) {
                        auto* sp = reinterpret_cast<const uint8_t*>(scan);
                        // Common prologues:
                        // 48 89 5C 24 xx — mov [rsp+xx], rbx
                        // 48 83 EC xx    — sub rsp, xx
                        // 40 53          — push rbx (with REX)
                        // CC             — int3 (padding before function)
                        if (sp[0] == 0xCC && scan + 1 < addr) {
                            func_start = scan + 1; // function starts after int3 padding
                            break;
                        }
                        if (sp[0] == 0x48 && sp[1] == 0x89 && sp[2] == 0x5C && sp[3] == 0x24) {
                            func_start = scan;
                            break;
                        }
                        if (sp[0] == 0x48 && sp[1] == 0x83 && sp[2] == 0xEC) {
                            func_start = scan;
                            break;
                        }
                        if (sp[0] == 0x40 && sp[1] == 0x53) {
                            func_start = scan;
                            break;
                        }
                    }
                    result.push_back({func_start});
                }
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// CSV loading (same as before)
// ---------------------------------------------------------------------------

static bool load_vtable_rvas(const std::string& csv_path,
    std::unordered_map<uint32_t, uintptr_t>& vtable_rvas)
{
    std::ifstream f(csv_path);
    if (!f) return false;

    std::string line;
    std::getline(f, line); // skip header

    while (std::getline(f, line)) {
        uint32_t id = 0;
        std::string rva_str;
        std::istringstream ss(line);
        std::string token;
        int col = 0;
        while (std::getline(ss, token, ',')) {
            switch (col) {
                case 0: id = static_cast<uint32_t>(std::stoul(token)); break;
                case 3: rva_str = token; break;
            }
            col++;
        }
        if (!rva_str.empty() && rva_str.size() > 2) {
            uintptr_t rva = std::stoull(rva_str, nullptr, 16);
            if (rva > 0) vtable_rvas[id] = rva;
        }
    }
    return !vtable_rvas.empty();
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

static void on_game_loaded() {
    x4n::log::info("event_probe: on_game_loaded — starting builder code scan");

    uintptr_t exe_base = x4n::detail::g_api->exe_base;
    if (!exe_base) {
        x4n::log::error("event_probe: exe_base not available");
        return;
    }

    // Find .text and .rdata section bounds
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(exe_base);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(exe_base + dos->e_lfanew);
    auto sections = IMAGE_FIRST_SECTION(nt);

    uintptr_t text_start = 0, text_end = 0;
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (std::strncmp(reinterpret_cast<char*>(sections[i].Name), ".text", 5) == 0) {
            text_start = exe_base + sections[i].VirtualAddress;
            text_end = text_start + sections[i].Misc.VirtualSize;
            break;
        }
    }
    if (!text_start) {
        x4n::log::error("event_probe: .text section not found");
        return;
    }

    // Load vtable RVAs from class_dump
    const std::string base = std::string(x4n::path()) + "/";
    std::string ext_root = base;
    while (!ext_root.empty() && (ext_root.back() == '/' || ext_root.back() == '\\'))
        ext_root.pop_back();
    auto sep = ext_root.find_last_of("/\\");
    std::string csv_path;
    if (sep != std::string::npos) {
        csv_path = ext_root.substr(0, sep + 1) + "x4native_class_dump\\event_type_ids.csv";
    }

    std::unordered_map<uint32_t, uintptr_t> vtable_rvas;
    if (!load_vtable_rvas(csv_path, vtable_rvas)) {
        if (!load_vtable_rvas(base + "event_type_ids.csv", vtable_rvas)) {
            x4n::log::error("event_probe: could not load event_type_ids.csv");
            return;
        }
    }

    x4n::log::info((std::string("event_probe: loaded ") +
        std::to_string(vtable_rvas.size()) + " vtable RVAs, scanning .text for builders...").c_str());

    // For each event type, find builders and scan their code
    std::unordered_map<uint32_t, EventLayout> layouts;
    uint32_t found_builders = 0;
    uint32_t found_layouts = 0;

    for (auto& [type_id, vtable_rva] : vtable_rvas) {
        uintptr_t vtable_va = exe_base + vtable_rva;

        auto builders = find_builders_for_vtable(vtable_va, text_start, text_end);
        if (builders.empty()) continue;
        found_builders++;

        // Scan each builder, merge results
        EventLayout layout{};
        layout.type_id = type_id;

        for (auto& b : builders) {
            EventLayout candidate{};
            candidate.type_id = type_id;
            size_t scan_len = std::min<size_t>(512, text_end - b.func_start);
            if (scan_builder(reinterpret_cast<const uint8_t*>(b.func_start), scan_len, candidate)) {
                // Take the best alloc_size (non-zero)
                if (candidate.alloc_size > 0 && (layout.alloc_size == 0 || candidate.alloc_size < layout.alloc_size))
                    layout.alloc_size = candidate.alloc_size;
                // Merge fields
                for (auto& f : candidate.fields)
                    layout.fields.push_back(f);
            }
        }

        // Deduplicate merged fields — prefer largest size per offset
        if (!layout.fields.empty()) {
            std::sort(layout.fields.begin(), layout.fields.end(),
                [](const FieldInfo& a, const FieldInfo& b) {
                    return a.offset < b.offset || (a.offset == b.offset && a.size > b.size);
                });
            auto last = std::unique(layout.fields.begin(), layout.fields.end(),
                [](const FieldInfo& a, const FieldInfo& b) { return a.offset == b.offset; });
            layout.fields.erase(last, layout.fields.end());
        }

        if (layout.alloc_size > 0 || !layout.fields.empty()) {
            layouts[type_id] = std::move(layout);
            found_layouts++;
        }
    }

    x4n::log::info((std::string("event_probe: found builders for ") +
        std::to_string(found_builders) + " types, layouts for " +
        std::to_string(found_layouts)).c_str());

    // Write event_layouts.csv
    {
        std::ofstream f(base + "event_layouts.csv");
        if (!f) {
            x4n::log::error("event_probe: could not open event_layouts.csv for writing");
            return;
        }

        f << "id,alloc_size,fields\n";
        // Sort by type_id
        std::vector<uint32_t> sorted_ids;
        for (auto& [id, _] : layouts) sorted_ids.push_back(id);
        std::sort(sorted_ids.begin(), sorted_ids.end());

        for (uint32_t id : sorted_ids) {
            auto& l = layouts[id];
            f << id << "," << l.alloc_size << ",";
            for (size_t i = 0; i < l.fields.size(); i++) {
                if (i > 0) f << ";";
                f << l.fields[i].offset << ":" << l.fields[i].size;
            }
            f << "\n";
        }
    }

    x4n::log::info((std::string("event_probe: wrote ") +
        std::to_string(found_layouts) + " event layouts → event_layouts.csv").c_str());
}

X4N_EXTENSION {
    // All scanning reads static .rdata/.text — no game state needed, run immediately.
    on_game_loaded();
}

X4N_SHUTDOWN {
}
