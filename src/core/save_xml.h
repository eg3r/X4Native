#pragma once
// ---------------------------------------------------------------------------
// SaveXML names added at runtime
// ---------------------------------------------------------------------------
// The save XML stores every element and attribute name as a number from the
// SaveXML namespace's fixed table (docs/rev/SAVE_XML.md). A name outside that
// table has no number, so the game drops it on load. Extensions register the
// names they write; the core gives each one a number above the game's range
// and answers both lookups for it before the game's own:
//
//   LookupKeyName_FormatXMLName  number -> name, while a save is written
//   LookupKeyName_LookupName     name -> number, while a save is read
//
// One table serves every extension: a name registered twice, by one extension
// or by two, has one number. A name the game already knows keeps the game's
// number and is left to the game. The table lives for the process; numbers are
// never written to a save, so they may differ from one session to the next.
//
// Free of the game and of MinHook so the logic can be tested on its own;
// core.cpp hooks the two lookups with format_xml_name and lookup_name.
// ---------------------------------------------------------------------------

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace x4n {

class SaveXml {
public:
    /// The game's two lookups, and their originals as MinHook returns them.
    using FormatXMLNameFn = const char* (*)(const int32_t* id);
    using LookupNameFn    = int32_t (*)(const char* name, const void* context);

    /// Takes both originals once both lookups are hooked. Names are registered
    /// only while attached: a name with a number the game cannot write or read
    /// back would lose what it carries.
    static void attach(FormatXMLNameFn format, LookupNameFn lookup);

    /// Drops the originals; registration stops.
    static void detach();

    /// Whether both originals are held.
    static bool attached();

    /// Registers `name` and returns its number. `ns` is the SaveXML namespace
    /// object; without it no name is registered, because a name the game knows
    /// could not be told apart. Returns 0 while detached, for a null namespace or
    /// name, or an empty name.
    static int32_t register_name(const void* ns, const char* name);

    /// The detours, with the game's own signatures: an added number or name is
    /// answered from the table; any other call goes to the original unchanged.
    static const char* format_xml_name(const int32_t* id);
    static int32_t lookup_name(const char* name, const void* context);

private:
    static std::mutex                     s_mutex;
    static std::map<std::string, int32_t> s_numbers;  // added name -> its number
    static std::map<int32_t, std::string> s_names;    // added number -> its name
    static std::atomic<FormatXMLNameFn>   s_format_original;
    static std::atomic<LookupNameFn>      s_lookup_original;
    static std::atomic<bool>              s_any;      // a lookup with nothing added goes straight to the game
};

} // namespace x4n
