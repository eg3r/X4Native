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
//     process, both lookups answer it, and an element carrying it is read
//     whole on load. Extensions share the numbers, so they are assigned in one
//     place; extensions numbering names on their own would collide and read
//     each other's data.
//   - The extension: the nodes it builds are well-formed, written beside the
//     game's, and skippable on load. Like a hook, what it writes is its own.
//   A save is written to and read from a stream: write a node with
//   XmlNode_Construct, XmlNode_Write and XmlNode_Destroy after
//   UniverseData_Export, and find it on the <universe> node after
//   UniverseData_Import. See docs/rev/SAVE_XML.md sections 4 to 6.
//
// Fails when the running X4Native has no SaveXML name support, or when its three
// SaveXML hooks are not installed (for example on a build mismatch). See
// docs/rev/SAVE_XML.md.
//
// Never hard-code a game number such as 0x67 (<blacklist>): it is a position in
// the game's table, which grows between builds. Use game_number at load.
//
// Thin handle: name pointer + number. No heap allocation. The name points at
// the core's stored copy and lives as long as the core's table.
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

#include <cstdint>
#include <optional>
#include <vector>

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
    const char* stored = nullptr;
    const int32_t number = api->savexml_register(local_name, api, &stored);
    if (!number || !stored) return std::nullopt;
    return SaveXmlTag(stored, number);
}

/// The game's own number for a name the game owns, or 0. Registers nothing.
inline int32_t game_number(const char* game_name) {
    auto* api = detail::g_api;
    if (!api || !api->savexml_lookup || !game_name) return 0;
    return api->savexml_lookup(game_name);
}

// ---------------------------------------------------------------------------
// Nodes. The core keeps the node layout; build and read nodes through these
// calls. A node from create_node belongs to the game once appended to one of
// the game's nodes.
// ---------------------------------------------------------------------------

/// A new element node, or null.
inline void* create_node(int32_t element) {
    auto* api = detail::g_api;
    return api && api->savexml_create_node ? api->savexml_create_node(element) : nullptr;
}

/// Sets or replaces an attribute. False when it cannot be set; a value over
/// 10,000,000 bytes, the most a save can carry, is refused and logged.
inline bool set_attribute(void* node, int32_t attribute, const char* value) {
    auto* api = detail::g_api;
    return api && api->savexml_set_attribute && api->savexml_set_attribute(node, attribute, value, api) != 0;
}

/// Appends `child` to `parent`'s children.
inline bool append_child(void* parent, void* child) {
    auto* api = detail::g_api;
    return api && api->savexml_append_child && api->savexml_append_child(parent, child) != 0;
}

/// The attribute's value, owned by the node, or null.
inline const char* get_attribute(const void* node, int32_t attribute) {
    auto* api = detail::g_api;
    return api && api->savexml_get_attribute ? api->savexml_get_attribute(node, attribute) : nullptr;
}

/// The node's element number; 0 for a text node or null.
inline int32_t element(const void* node) {
    auto* api = detail::g_api;
    return api && api->savexml_node_element ? api->savexml_node_element(node) : 0;
}

/// The node's children, in order.
inline std::vector<void*> children(const void* node) {
    auto* api = detail::g_api;
    if (!api || !api->savexml_node_children) return {};
    std::vector<void*> out(api->savexml_node_children(node, nullptr, 0));
    if (!out.empty()) out.resize(api->savexml_node_children(node, out.data(), out.size()));
    return out;
}

}} // namespace x4n::savexml
