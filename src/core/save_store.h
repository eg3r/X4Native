#pragma once
// ---------------------------------------------------------------------------
// x4native_core.dll — Save Store
// ---------------------------------------------------------------------------
// The core-owned key-value data in a save: one `<x4native.store>` element, the
// last child of `<universe>`, with one `<x4native.ext id="...">` block per
// loaded extension and one `<x4native.value>` per stored key.
//
// Extensions reach it through `x4n::save` (sdk/x4n_save.h); the core owns the
// XML. A plain /reloadui keeps the values in core memory; a core hot-reload
// carries them across in the proxy's stash, tagged with the world's player id.
// A block of an extension that is not loaded is dropped on the next save.
//
// The three element names are registered under the core's own id `x4native`,
// through the same scheme as an extension's, so they carry the separator and
// pass the load check. The attributes `id`, `key`, `type` and `value` are the
// game's names, read through game_number.
//
// The game's node calls, the SaveXML names and the loaded-extension check
// arrive as the host the core installs, so the store is free of the core's
// singletons and the bed drives it against its fake engine.
// ---------------------------------------------------------------------------

#include "x4native_defs.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct X4GameFunctions;
struct X4GameOffsets;

namespace x4n {

class SaveStore {
public:
    enum class Type : int32_t { Bool = 0, Int = 1, Number = 2, String = 3 };

    struct Value {
        Type        type    = Type::Bool;
        bool        boolean = false;
        int64_t     integer = 0;
        double      number  = 0.0;
        std::string text;
    };

    /// What the store needs from the framework: the game's node calls and
    /// offsets, the SaveXML names, the loaded extensions, and a log sink.
    struct Host {
        const X4GameFunctions* game = nullptr;
        const X4GameOffsets*   offsets = nullptr;
        int32_t           (*register_name)(const char* ext_id, const char* local_name) = nullptr;
        int32_t           (*game_number)(const char* name) = nullptr;
        const void*       (*savexml_namespace)() = nullptr;
        bool              (*is_loaded)(const char* ext_id) = nullptr;
        void              (*log)(const char* line) = nullptr;
    };

    /// Takes the host and the proxy's stash calls. Called once from the core's
    /// init.
    static void install(const Host& host, stash_set_fn stash_set, stash_get_fn stash_get,
                        stash_remove_fn stash_remove);

    /// Drops the host; the stash keeps whatever the last world left.
    static void uninstall();

    /// Registers the store's names for the current session, once the game has
    /// built the SaveXML namespace. Safe to call again; a missing namespace
    /// leaves it for the next call.
    static void ensure_names();

    /// Reads `<x4native.store>` from the `<universe>` node once the game has read
    /// it, every child then on the node; a new load starts the store over.
    static void begin_load(const void* universe_node);

    /// Appends `<x4native.store>` to `parent`, when the store holds values of a
    /// loaded extension. The core builds it under a holder node and writes it
    /// to the save stream as the last child of `<universe>`.
    static bool write(void* parent);

    /// The children of a save node, in order.
    static std::vector<void*> children(void* node);

    /// The world is ready: a save load keeps what it read, the same world keeps
    /// what it holds (a plain /reloadui), and a new world starts empty unless a
    /// stash with that player id is waiting.
    static void world_ready();

    /// Puts the store into the proxy's stash, tagged with `player_id`.
    static void stash(int64_t player_id);

    static bool set(const char* ext_id, const char* key, const Value& value);
    static bool get(const char* ext_id, const char* key, Value* out);
    static bool remove(const char* ext_id, const char* key);

private:
    static void read_node(const void* store_node);
    static void log_line(const std::string& line);

    static Host s_host;

    static std::mutex s_mutex;
    static std::map<std::string, std::map<std::string, Value>> s_values;  // extension id -> key -> value
    static int64_t s_imported_player;                                    // the world a save load read values for, or 0
    static int64_t s_last_player;                                        // the world the values belong to

    static stash_set_fn    s_stash_set;
    static stash_get_fn    s_stash_get;
    static stash_remove_fn s_stash_remove;

    static bool s_names_ready;
    static bool s_names_logged;
    static int32_t s_store_name;
    static int32_t s_ext_name;
    static int32_t s_value_name;
    static int32_t s_id_name;
    static int32_t s_key_name;
    static int32_t s_type_name;
    static int32_t s_value_attr_name;
};

} // namespace x4n
