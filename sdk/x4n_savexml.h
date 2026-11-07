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
//   x4n::savexml::register_tag — registers one name, nullopt when it cannot
//
//   - Register at x4native_init, before a save is read: the names of a save are
//     resolved as its file is parsed.
//   - A name registered by any extension has one number; registering it again
//     returns that number. A name the game already knows returns the game's.
//   - Numbers are assigned per session and never reach the save file, which
//     holds names only. Do not store a number.
//
// Usage:
//   auto tag = x4n::savexml::register_tag("my_element");
//   if (!tag) { /* name unavailable */ }
//   int32_t number = tag->number();  // build nodes with it
//
// Responsibility:
//   - The core: a registered name has one number for every extension in the
//     process, and both lookups answer it. Extensions share the numbers, so
//     they are assigned in one place; extensions numbering names on their own
//     would collide and read each other's data.
//   - The extension: the nodes it builds are well-formed, added beside the
//     game's, and skippable on load. Like a hook, what it writes is its own.
//   See docs/rev/SAVE_XML.md sections 6 and 7.
//
// Fails when the running X4Native has no SaveXML name support, when its two
// lookup hooks are not installed (for example on a build mismatch), or before
// the game has built the SaveXML table. See docs/rev/SAVE_XML.md.
//
// Thin handle: name pointer + number. No heap allocation. The name must
// outlive the tag.
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

    const char* name() const { return name_; }

    /// The name's number for this session.
    int32_t number() const { return number_; }
};

/// Registers one element or attribute name.
/// Returns std::nullopt when the name cannot be registered.
inline std::optional<SaveXmlTag> register_tag(const char* name) {
    auto* api = detail::g_api;
    if (!api || !api->savexml_register || !name) return std::nullopt;
    const int32_t number = api->savexml_register(name);
    if (!number) return std::nullopt;
    return SaveXmlTag(name, number);
}

}} // namespace x4n::savexml
