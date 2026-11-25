#pragma once
// ---------------------------------------------------------------------------
// SaveXML names added at runtime
// ---------------------------------------------------------------------------
// The save XML stores every element and attribute name as a number from the
// SaveXML namespace's fixed table (docs/rev/SAVE_XML.md). A name outside that
// table has no number, so the game drops it on load. Extensions register the
// local names they write; the core stores each one under the caller's
// extension id as `<id><separator><local_name>` and answers both lookups for
// it before the game's own:
//
//   LookupKeyName_FormatXMLName  number -> name, while a save is written
//   LookupKeyName_LookupName     name -> number, while a save is read
//
// A tag's `name()` is the full stored name, so logs and the file match.
//
// Every registered name carries the separator, and no game name does, so the
// lookups send a game name or a game number straight to the game without
// touching the core's table. The table is rebuilt on each registration and
// published through an atomic pointer; a replaced table is kept until the core
// shuts down, because a detour on another thread may still be reading it.
//
// One table serves every extension. The table lives for the life of the core:
// a core hot-reload clears it and numbers are assigned again when extensions
// re-register. Numbers are never written to a save, so they may differ from
// one session to the next.
//
// Free of the game, of MinHook and of the logger so the logic can be tested on
// its own; core.cpp hooks the two lookups and supplies the log sink.
// ---------------------------------------------------------------------------

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace x4n {

class SaveXml {
public:
    /// The game's two lookups, and their originals as MinHook returns them.
    using FormatXMLNameFn = const char* (*)(const int32_t* id);
    using LookupNameFn    = int32_t (*)(const char* name, const std::string* context);

    /// Receives the core's log lines. Null leaves them unsaid.
    using LogFn = void (*)(const char* line);

    /// Takes both originals once both lookups are hooked. Names are registered
    /// only while attached: a name with a number the game cannot write or read
    /// back would lose what it carries.
    static void attach(FormatXMLNameFn format, LookupNameFn lookup);

    /// Refuses registration; the originals and the tables stay, because a
    /// detour may still be inside one while the core removes the hooks.
    static void detach();

    /// Whether registration is open.
    static bool attached();

    /// Where the core's log lines go. Set once by the core; the bed sets its
    /// own sink.
    static void set_log(LogFn log);

    /// Registers `local_name` under `ext_id` and returns the number of the
    /// stored `<ext_id><separator><local_name>`; `out_name`, when given, is set
    /// to the stored name, which lives as long as the core. `ns` is the SaveXML namespace
    /// object; without it nothing is registered, because the game's own numbers
    /// could not be read. Returns 0 while detached, for a null or invalid id or
    /// local name, for an id or local name carrying the separator, or without
    /// the namespace.
    static int32_t register_name(const void* ns, const char* ext_id, const char* local_name,
                                 const char** out_name = nullptr);

    /// The game's own number for a game name, or 0. Reads the game's table only
    /// and registers nothing: a name an extension added is not a game name.
    static int32_t game_number(const void* ns, const char* name);

    /// Starts a new save load: the load-miss log names each missing extension's
    /// data once per load.
    static void begin_load();

    /// The detours, with the game's own signatures: an added number or name is
    /// answered from the table; any other call goes to the original unchanged.
    static const char* format_xml_name(const int32_t* id);
    static int32_t lookup_name(const char* name, const std::string* context);

private:
    /// One immutable set of added names, published whole. `names` is indexed by
    /// `number - first_added`; `numbers` finds a name without building a
    /// std::string.
    struct Table {
        std::map<std::string, int32_t, std::less<>> numbers;  // added name -> its number
        std::vector<std::string>                    names;   // number - first_added -> its name
    };

    /// The load-miss set, searchable by `const char*` without a std::string.
    struct NameHash {
        using is_transparent = void;
        size_t operator()(std::string_view name) const { return std::hash<std::string_view>{}(name); }
    };

    struct NameEqual {
        using is_transparent = void;
        bool operator()(std::string_view left, std::string_view right) const { return left == right; }
    };

    static void log_line(const std::string& line);

    static std::mutex                                   s_register_mutex;
    static std::atomic<const Table*>                    s_table;
    static std::vector<std::unique_ptr<Table>>          s_retired;       // guarded by s_register_mutex
    static std::atomic<int32_t>                         s_first_added;   // 0 = none added
    static std::atomic<bool>                            s_attached;
    static std::atomic<FormatXMLNameFn>                 s_format_original;
    static std::atomic<LookupNameFn>                    s_lookup_original;
    static std::atomic<LogFn>                           s_log;

    static std::mutex                                   s_miss_mutex;
    static std::unordered_set<std::string, NameHash, NameEqual> s_missed;  // logged this load
};

} // namespace x4n
