// ---------------------------------------------------------------------------
// x4native_save_cache -- Binary DOM cache for faster savegame loading
// ---------------------------------------------------------------------------
// Hooks libxml2's internal ReadFile wrapper to intercept savegame XML parsing.
// When a .x4bin binary cache exists alongside the .xml.gz save file,
// constructs the libxml2 DOM tree from binary (~450ms) instead of parsing
// XML text (~7-21s). The game's GameClass::Load runs unchanged.
//
// Phase 1: struct verification -- logs xmlDoc/xmlNode layout checks after
//          each savegame load, confirming our offset assumptions.
// Phase 2: binary cache -- when .x4bin exists, constructs DOM from binary
//          and returns it instead of parsing XML.
//
// See docs/rev/BINARY_SAVE_CACHE_PLAN.md for the full plan.
// ---------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <x4n_core.h>
#include <x4n_events.h>
#include <x4n_log.h>

#include "struct_verify.h"
#include "dom_builder.h"

#include <cstring>
#include <string>
#include <chrono>

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static bool g_layout_verified = false;
static bool g_layout_ok       = false;
static bool g_hook_installed   = false;

// Original function pointer (trampoline from MinHook via _ensure_detour)
// Signature: void* __fastcall(const char* filename)
using ReadFile_fn = void* (__fastcall*)(const char*);
static ReadFile_fn g_original_readfile = nullptr;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Check if a URI is a savegame full-load (not a header scan)
// Full load:   personal:///save/save_006?ext=xml.gz%20xml
// Header scan: personal:///save/save_006?ext=xml.gz%20xml&maxsize=0x1000
static bool is_savegame_load(const char* uri) {
    if (!uri) return false;
    // Must start with personal:///save/
    if (std::strncmp(uri, "personal:///save/", 17) != 0) return false;
    // Must NOT contain maxsize (header-only scan)
    if (std::strstr(uri, "maxsize") != nullptr) return false;
    return true;
}

// Derive .x4bin cache path from save file path
static std::string cache_path_for(const char* save_path) {
    std::string p(save_path);
    // Replace .xml.gz or .xml with .x4bin
    auto gz_pos = p.rfind(".xml.gz");
    if (gz_pos != std::string::npos) {
        return p.substr(0, gz_pos) + ".x4bin";
    }
    auto xml_pos = p.rfind(".xml");
    if (xml_pos != std::string::npos) {
        return p.substr(0, xml_pos) + ".x4bin";
    }
    return p + ".x4bin";
}

static bool file_exists(const char* path) {
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

// ---------------------------------------------------------------------------
// Hook: libxml2_ReadFile
// ---------------------------------------------------------------------------
// Intercepts the game's internal xmlReadFile wrapper.
// Signature: void* __fastcall(const char* filename)
// Returns: xmlDocPtr (opaque pointer to libxml2 xmlDoc)

static volatile long g_call_count = 0;

// Try to read filename safely -- the arg might be xmlChar* (utf8) or wchar_t*
static const char* safe_read_filename(const char* ptr) {
    __try {
        // Check if it looks like a printable ASCII/UTF-8 string
        if (ptr && ptr[0] >= 0x20 && ptr[0] < 0x7F)
            return ptr;
    } __except(1) {}
    return nullptr;
}

static void* __fastcall hooked_readfile(const char* filename) {
    using Clock = std::chrono::high_resolution_clock;
    auto t0 = Clock::now();

    void* doc = g_original_readfile(filename);

    auto t1 = Clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Accumulate stats
    static double s_total_ms   = 0;
    static long   s_total_calls = 0;
    static double s_max_ms     = 0;
    static char   s_max_file[256] = {};
    s_total_ms += ms;
    s_total_calls++;
    if (ms > s_max_ms) {
        s_max_ms = ms;
        const char* n = safe_read_filename(filename);
        if (n) {
            strncpy(s_max_file, n, sizeof(s_max_file) - 1);
            s_max_file[sizeof(s_max_file) - 1] = 0;
        }
    }

    const char* safe_name = safe_read_filename(filename);
    bool is_personal = safe_name && std::strncmp(safe_name, "personal:", 9) == 0;
    bool savegame = is_savegame_load(filename);

    // Log personal:// calls and any call > 50ms
    if (is_personal || ms > 50.0) {
        x4n::log::info("save_cache: ReadFile %.1fms -> %p : %s",
                       ms, doc, safe_name ? safe_name : "(?)");
    }

    // On savegame load, dump cumulative stats
    if (savegame) {
        x4n::log::info("save_cache: === CUMULATIVE STATS at save load ===");
        x4n::log::info("save_cache:   total calls: %ld", s_total_calls);
        x4n::log::info("save_cache:   total time:  %.1f ms (%.1f s)", s_total_ms, s_total_ms / 1000.0);
        x4n::log::info("save_cache:   avg per call: %.3f ms", s_total_ms / s_total_calls);
        x4n::log::info("save_cache:   slowest: %.1f ms : %s", s_max_ms, s_max_file);
    }

    return doc;
}

// ---------------------------------------------------------------------------
// Hook: UniverseClass::Import — times the galaxy/component creation phase
// ---------------------------------------------------------------------------
// sub_14089AE80: the single biggest function in the load path (7068 bytes).
// Creates galaxy, all clusters/sectors/zones, all ships/stations/NPCs.

using UniverseImport_fn = char (__fastcall*)(void*, void*, void*, unsigned int*, unsigned int);
static UniverseImport_fn g_original_universe_import = nullptr;

static char __fastcall hooked_universe_import(void* a1, void* a2, void* a3,
                                               unsigned int* a4, unsigned int a5) {
    using Clock = std::chrono::high_resolution_clock;
    x4n::log::info("save_cache: >>> UniverseClass::Import START");
    auto t0 = Clock::now();

    char result = g_original_universe_import(a1, a2, a3, a4, a5);

    auto t1 = Clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    x4n::log::info("save_cache: <<< UniverseClass::Import END: %.1f ms (%.1f s)",
                   ms, ms / 1000.0);
    return result;
}

// ---------------------------------------------------------------------------
// Hook installation
// ---------------------------------------------------------------------------

static void install_hook() {
    // --- Hook 1: libxml2_ReadFile (XML file I/O timing) ---
    void* addr = x4n::detail::g_api->resolve_internal("libxml2_ReadFile");
    if (!addr) {
        x4n::log::error("save_cache: libxml2_ReadFile not found");
        return;
    }
    x4n::log::info("save_cache: libxml2_ReadFile at %p", addr);

    void* trampoline = x4n::detail::g_api->_ensure_detour(
        "libxml2_ReadFile",
        reinterpret_cast<void*>(&hooked_readfile));
    if (!trampoline) {
        x4n::log::error("save_cache: ReadFile detour failed");
        return;
    }
    g_original_readfile = reinterpret_cast<ReadFile_fn>(trampoline);
    g_hook_installed = true;
    x4n::log::info("save_cache: ReadFile hook installed");

    static auto keep_alive = [](X4HookContext*) -> int { return 0; };
    x4n::detail::g_api->hook_before(
        "libxml2_ReadFile", +keep_alive, nullptr, x4n::detail::g_api);

    // --- Hook 2: UniverseClass::Import (component creation timing) ---
    void* import_addr = x4n::detail::g_api->resolve_internal("UniverseClass_Import");
    if (import_addr) {
        x4n::log::info("save_cache: UniverseClass::Import at %p", import_addr);

        void* import_tramp = x4n::detail::g_api->_ensure_detour(
            "UniverseClass_Import",
            reinterpret_cast<void*>(&hooked_universe_import));

        if (import_tramp) {
            g_original_universe_import = reinterpret_cast<UniverseImport_fn>(import_tramp);
            x4n::log::info("save_cache: UniverseClass::Import hook installed");

            static auto keep_alive2 = [](X4HookContext*) -> int { return 0; };
            x4n::detail::g_api->hook_before(
                "UniverseClass_Import", +keep_alive2, nullptr, x4n::detail::g_api);
        } else {
            x4n::log::warn("save_cache: UniverseClass::Import detour failed");
        }
    } else {
        x4n::log::warn("save_cache: UniverseClass_Import not in version DB");
    }

    // --- Hook 3: AIDirector_LoadScripts ---
    {
        void* addr = x4n::detail::g_api->resolve_internal("AIDirector_LoadScripts");
        if (addr) {
            x4n::log::info("save_cache: AIDirector_LoadScripts at %p", addr);

            static void (__fastcall *s_orig)(void*, void*) = nullptr;
            static auto detour = +[](void* a1, void* a2) {
                using Clock = std::chrono::high_resolution_clock;
                x4n::log::info("save_cache: >>> AIDirector_LoadScripts START");
                auto t0 = Clock::now();
                s_orig(a1, a2);
                double ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
                x4n::log::info("save_cache: <<< AIDirector_LoadScripts END: %.1f ms", ms);
            };

            void* t = x4n::detail::g_api->_ensure_detour(
                "AIDirector_LoadScripts", reinterpret_cast<void*>(detour));
            if (t) {
                s_orig = reinterpret_cast<decltype(s_orig)>(t);
                x4n::log::info("save_cache: AIDirector_LoadScripts hook installed (tramp=%p)", t);
                static auto ka = [](X4HookContext*) -> int { return 0; };
                x4n::detail::g_api->hook_before("AIDirector_LoadScripts", +ka, nullptr, x4n::detail::g_api);
            } else {
                x4n::log::warn("save_cache: AIDirector_LoadScripts detour failed");
            }
        }
    }

    // --- Hook 4: AIDirector_Import ---
    {
        void* addr = x4n::detail::g_api->resolve_internal("AIDirector_Import");
        if (addr) {
            x4n::log::info("save_cache: AIDirector_Import at %p", addr);

            static char (__fastcall *s_orig)(void*, void*, void*, int) = nullptr;
            static auto detour = +[](void* a1, void* a2, void* a3, int a4) -> char {
                using Clock = std::chrono::high_resolution_clock;
                x4n::log::info("save_cache: >>> AIDirector_Import START");
                auto t0 = Clock::now();
                char r = s_orig(a1, a2, a3, a4);
                double ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
                x4n::log::info("save_cache: <<< AIDirector_Import END: %.1f ms (%.1f s)", ms, ms/1000);
                return r;
            };

            void* t = x4n::detail::g_api->_ensure_detour(
                "AIDirector_Import", reinterpret_cast<void*>(detour));
            if (t) {
                s_orig = reinterpret_cast<decltype(s_orig)>(t);
                x4n::log::info("save_cache: AIDirector_Import hook installed (tramp=%p)", t);
                static auto ka = [](X4HookContext*) -> int { return 0; };
                x4n::detail::g_api->hook_before("AIDirector_Import", +ka, nullptr, x4n::detail::g_api);
            } else {
                x4n::log::warn("save_cache: AIDirector_Import detour failed");
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Extension lifecycle
// ---------------------------------------------------------------------------

X4N_EXTENSION {
    x4n::log::info("save_cache: initializing");
    install_hook();
}

X4N_SHUTDOWN {
    x4n::log::info("save_cache: shutting down");
    // MinHook cleanup is handled by X4Native core on shutdown
}
