#pragma once
// ---------------------------------------------------------------------------
// Save data — per-extension values stored in the savegame (x4n::save)
// ---------------------------------------------------------------------------
// The game saves the Lua global an addon declares as a savegame variable
// (extension/ui.xml, <savedvariable storage="savegame">) and restores it when
// that save is loaded. X4Native declares one such variable. This class holds
// the values behind it; x4native.lua and the proxy move them between here and
// the Lua table.
//
//   extension --set--> SaveStore --take_changes--> Lua table --> savegame
//   savegame --> Lua table --import_*--> SaveStore --get--> extension
//
// The values belong to the game that is loaded. They are dropped when another
// game starts or loads, and they survive /reloadui and a core hot-reload.
//
// Keys and values reach Lua in an encoded form (encode/decode): a ':' followed
// by letters, digits, '_', '-', '.' and %XX escapes. The game writes the table
// as Lua text and reads a string that looks like a number back as a number;
// the ':' keeps every key and value a string. The escapes keep quotes, control
// bytes and non-ASCII bytes out of the save file.
//
// Free of Lua and of the game, so the logic can be tested on its own.
// ---------------------------------------------------------------------------

#include "x4native_defs.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace x4n {

class SaveStore {
public:
    /// Longest key, in bytes.
    static constexpr size_t kMaxKeyLength = 128;
    /// Largest single value, in bytes.
    static constexpr size_t kMaxValueSize = 256 * 1024;
    /// Most bytes all extensions may hold together, counted in encoded form
    /// (up to three per byte). The game stores every addon's savegame variables
    /// in one XML text node, so this stays far below the size a save file can
    /// still be read back with.
    static constexpr size_t kMaxTotalSize = 2 * 1024 * 1024;

    // --- Extensions (through X4NativeAPI) ---------------------------------

    /// Stores a value. False while no game is loaded, for an empty or too long
    /// key, for a too large value, or when the store is full.
    static bool set(const char* ext_id, const char* key, const void* data, uint32_t size);

    /// The stored value, or null. The bytes are followed by a NUL, and stay
    /// valid until the extension changes that key or another game is loaded.
    static const void* get(const char* ext_id, const char* key, uint32_t* out_size);

    /// Removes one value. True when it existed.
    static bool remove(const char* ext_id, const char* key);

    /// Removes every value of one extension.
    static void clear(const char* ext_id);

    /// Whether a game's values are loaded, so they can be read and written.
    static bool ready();

    // --- The Lua side (through the proxy's dispatch table, UI thread) ------

    /// A new Lua state exists. With no game running the values of the previous
    /// game are dropped; with a game running (/reloadui) they are kept and
    /// written into the new state by the next take_changes. True when the
    /// values were kept.
    static bool reset(bool game_active);

    /// Replaces all values with the ones of a loaded save: begin, one entry
    /// per stored value in encoded form, end. After end the store is active.
    static void import_begin();
    static void import_entry(const char* ext, const char* key, const char* value);
    static void import_end();

    /// The values changed since the last call, in encoded form. With `full`,
    /// or after a /reloadui or core reload, it is every value instead, and
    /// `*out_full` tells the Lua side to replace what it holds. A null `value`
    /// means the key was removed. The array stays valid until the next call.
    static int take_changes(bool full, const SaveChangeC** out, bool* out_full);

    // --- Core hot-reload ---------------------------------------------------

    /// Hands the values to the proxy's stash before the core is unloaded, and
    /// takes them back in the new core.
    static void stash_snapshot(stash_set_fn stash_set);
    static void stash_restore(stash_get_fn stash_get, stash_remove_fn stash_remove);

    // --- Encoding ----------------------------------------------------------

    /// The form a key or value has in the Lua table.
    static std::string encode(std::string_view raw);
    /// False when `encoded` is not a string encode() can produce.
    static bool decode(std::string_view encoded, std::string& out);
};

} // namespace x4n
