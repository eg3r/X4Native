// ---------------------------------------------------------------------------
// x4n_savexml.h — Element and attribute names for the save XML
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// A save's XML nodes carry their element and attribute names as numbers from
// the game's fixed SaveXML table; a name outside it cannot be written or read.
// Registering a name gives it a number both of the game's lookups answer, so
// nodes built with that number are saved under the name and loaded back to it.
//
// Provides:
//   x4n::savexml::SaveXmlTag   — lightweight handle for a registered name
//   x4n::savexml::register_tag — registers one local name, nullopt when it
//                                cannot
//   x4n::savexml::game_number  — the game's own number for a game name
//
//   - Register at x4native_init, before a save is read: the names of a save are
//     resolved as its file is parsed.
//   - The core stores the name under the calling extension's id, as
//     `<extension id>.<local name>`; an extension registers its local names and
//     never writes or parses the separator. A name registered again by the same
//     extension keeps its number; the same local name from two extensions gets
//     two names and two numbers.
//   - A game name is not registered: `game_number` reads the game's own table
//     and registers nothing. Use it for a name the game owns, such as
//     `blacklist` or `id`.
//   - Numbers are assigned per session and never reach the save file, which
//     holds names only. Do not store a number.
//
// Usage:
//   auto tag = x4n::savexml::register_tag("my_element");
//   if (!tag) { /* name unavailable */ }
//   int32_t number = tag->number();  // build nodes with it
//   const int32_t blacklist = x4n::savexml::game_number("blacklist");
//
// Responsibility:
//   - The core: a registered name has one number for every extension in the
//     process, and both lookups answer it. Extensions share the numbers, so
//     they are assigned in one place; extensions numbering names on their own
//     would collide and read each other's data.
//   - The extension: the nodes it builds are well-formed, written beside the
//     game's, and skippable on load. Like a hook, what it writes is its own.
//   See docs/rev/SAVE_XML.md sections 4 to 6.
//
// Fails when the running X4Native has no SaveXML name support, when its two
// lookup hooks are not installed (for example on a build mismatch), or before
// the game has built the SaveXML table. See docs/rev/SAVE_XML.md.
//
// Thin handle: name pointer + number. No heap allocation. The name points at
// the core's stored copy and lives as long as the core's table.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

#include <cstdint>
#include <optional>

namespace x4n { namespace savexml {

/// Lightweight handle for a name registered in the SaveXML table.
class SaveXmlTag {
    const char* name_;
    int32_t     number_;

public:
    SaveXmlTag(const char* name, int32_t number) : name_(name), number_(number) {}

    /// The full stored name, `<extension id>.<local name>`.
    const char* name() const { return name_; }

    /// The name's number for this session.
    int32_t number() const { return number_; }
};

/// Registers one local element or attribute name under the calling
/// extension's id.
/// Returns std::nullopt when the name cannot be registered.
inline std::optional<SaveXmlTag> register_tag(const char* local_name) {
    auto* api = detail::g_api;
    if (!api || !api->savexml_register || !local_name) return std::nullopt;
    const int32_t number = api->savexml_register(local_name, api);
    if (!number) return std::nullopt;

    // The core built the full name; the formatter answers it from the core's
    // table, so the tag points at the stored copy rather than the caller's
    // local name. A registration that succeeded has both hooks, so the
    // fallback is only for a core that does not answer the formatter.
    const char* full = local_name;
    using FormatFn = const char* (*)(const int32_t*);
    if (auto* format = reinterpret_cast<FormatFn>(::x4n::game_internal("LookupKeyName_FormatXMLName"))) {
        if (const char* stored = format(&number); stored && *stored) full = stored;
    }
    return SaveXmlTag(full, number);
}

/// The game's own number for a name the game owns, or 0. Registers nothing.
inline int32_t game_number(const char* game_name) {
    auto* api = detail::g_api;
    if (!api || !api->savexml_lookup || !game_name) return 0;
    return api->savexml_lookup(game_name);
}

}} // namespace x4n::savexml
