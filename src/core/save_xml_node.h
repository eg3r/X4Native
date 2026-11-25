#pragma once
// ---------------------------------------------------------------------------
// Save XML nodes for extensions
// ---------------------------------------------------------------------------
// An extension builds and reads the nodes of a save through these calls; the
// node layout stays here (save_xml_layout.h). Nodes are built as the game's own
// writers build them: allocated with GameAlloc, so the game frees them with the
// rest of the save once appended to one of its nodes.
//
// Free of MinHook and of the logger so the logic can be tested on its own;
// core.cpp hands it the game's functions and the log sink.
// ---------------------------------------------------------------------------

#include <x4_game_func_table.h>

#include <cstddef>
#include <cstdint>

namespace x4n {

class SaveXmlNode {
public:
    /// Receives the core's log lines. Null leaves them unsaid.
    using LogFn = void (*)(const char* line);

    /// The game's functions the nodes are built with; null until the core
    /// resolves them.
    static void attach(const X4GameFunctions* game, LogFn log);

    /// A new element node, or null when the game's functions are not resolved.
    static void* create(int32_t element);

    /// The longest value a save can carry: libxml2's XML_MAX_TEXT_LENGTH, as
    /// the game's reader is created without the huge-document option.
    static constexpr size_t kMaxValueBytes = 10000000;

    /// Sets or replaces an attribute. False for a null node or value, and for a
    /// value over kMaxValueBytes, which is refused and logged with `ext_id`.
    static bool set_attribute(void* node, int32_t attribute, const char* value, const char* ext_id);

    /// Appends `child` to `parent`'s children. False when either is null.
    static bool append_child(void* parent, void* child);

    /// The attribute's value, owned by the node, or null.
    static const char* get_attribute(const void* node, int32_t attribute);

    /// The node's element number; 0 for a text node or null.
    static int32_t element(const void* node);

    /// Writes up to `capacity` child pointers to `out` and returns the number
    /// of children; with `out` null, only the number.
    static size_t children(const void* node, void** out, size_t capacity);

private:
    static const X4GameFunctions* s_game;
    static LogFn                  s_log;
};

} // namespace x4n
