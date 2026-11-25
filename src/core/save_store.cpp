#include "save_store.h"
#include "save_xml_node.h"

#include <x4_game_func_table.h>
#include <x4_game_offsets.h>
#include <x4_manual_types.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <format>
#include <optional>
#include <set>
#include <vector>

namespace x4n {

namespace {

constexpr const char* kStashNamespace = "x4native";
constexpr const char* kStashKey       = "save_store";

using json = nlohmann::json;

/// The host's game calls and offsets, set once by install().
const X4GameFunctions* g_game = nullptr;
const X4GameOffsets*   g_offsets = nullptr;

/// A save XML node, built and read through the core's node functions
/// (save_xml_node.h). The game frees the nodes with the save tree; this handle
/// only moves around the memory the game owns.
class SaveNode {
    void* node_ = nullptr;

public:
    SaveNode() = default;
    explicit SaveNode(void* node) : node_(node) {}

    explicit operator bool() const { return node_ != nullptr; }
    void* object() const { return node_; }

    int32_t element() const { return SaveXmlNode::element(node_); }

    std::vector<SaveNode> children() const {
        std::vector<void*> raw(SaveXmlNode::children(node_, nullptr, 0));
        if (!raw.empty()) raw.resize(SaveXmlNode::children(node_, raw.data(), raw.size()));
        return std::vector<SaveNode>(raw.begin(), raw.end());
    }

    std::optional<std::string> attribute(int32_t id) const {
        const char* value = SaveXmlNode::get_attribute(node_, id);
        if (!value) return std::nullopt;
        return std::string(value);
    }

    void set_attribute(int32_t id, const char* value) const {
        SaveXmlNode::set_attribute(node_, id, value, "x4native");
    }

    void append(SaveNode child) const { SaveXmlNode::append_child(node_, child.node_); }

    /// A new element node, as the game's writers build one.
    static SaveNode create(int32_t element_number) { return SaveNode(SaveXmlNode::create(element_number)); }
};

const char* type_name(SaveStore::Type type) {
    switch (type) {
        case SaveStore::Type::Bool:   return "bool";
        case SaveStore::Type::Int:    return "int";
        case SaveStore::Type::Number: return "number";
        case SaveStore::Type::String: return "string";
    }
    return "bool";
}

bool type_from_name(const std::string& name, SaveStore::Type* out) {
    if (name == "bool") { *out = SaveStore::Type::Bool; return true; }
    if (name == "int") { *out = SaveStore::Type::Int; return true; }
    if (name == "number") { *out = SaveStore::Type::Number; return true; }
    if (name == "string") { *out = SaveStore::Type::String; return true; }
    return false;
}

std::string value_text(const SaveStore::Value& value) {
    switch (value.type) {
        case SaveStore::Type::Bool:   return value.boolean ? "1" : "0";
        case SaveStore::Type::Int:    return std::to_string(value.integer);
        case SaveStore::Type::Number: return std::format("{}", value.number);
        case SaveStore::Type::String: return value.text;
    }
    return {};
}

bool parse_value(SaveStore::Type type, const std::string& text, SaveStore::Value* out) {
    out->type = type;
    switch (type) {
        case SaveStore::Type::Bool:
            if (text != "0" && text != "1") return false;
            out->boolean = text == "1";
            return true;
        case SaveStore::Type::Int: {
            int64_t number = 0;
            const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), number);
            if (ec != std::errc{} || ptr != text.data() + text.size()) return false;
            out->integer = number;
            return true;
        }
        case SaveStore::Type::Number: {
            double number = 0;
            const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), number);
            if (ec != std::errc{} || ptr != text.data() + text.size()) return false;
            out->number = number;
            return true;
        }
        case SaveStore::Type::String:
            out->text = text;
            return true;
    }
    return false;
}

}  // namespace

SaveStore::Host SaveStore::s_host;

std::mutex SaveStore::s_mutex;
std::map<std::string, std::map<std::string, SaveStore::Value>> SaveStore::s_values;
int64_t SaveStore::s_imported_player = 0;
int64_t SaveStore::s_last_player = 0;

stash_set_fn    SaveStore::s_stash_set = nullptr;
stash_get_fn    SaveStore::s_stash_get = nullptr;
stash_remove_fn SaveStore::s_stash_remove = nullptr;

bool SaveStore::s_names_ready = false;
bool SaveStore::s_names_logged = false;
int32_t SaveStore::s_store_name = 0;
int32_t SaveStore::s_ext_name = 0;
int32_t SaveStore::s_value_name = 0;
int32_t SaveStore::s_id_name = 0;
int32_t SaveStore::s_key_name = 0;
int32_t SaveStore::s_type_name = 0;
int32_t SaveStore::s_value_attr_name = 0;

void SaveStore::install(const Host& host, stash_set_fn stash_set, stash_get_fn stash_get,
                        stash_remove_fn stash_remove) {
    s_host = host;
    g_game = host.game;
    g_offsets = host.offsets;
    s_stash_set = stash_set;
    s_stash_get = stash_get;
    s_stash_remove = stash_remove;
    s_names_ready = false;
    s_names_logged = false;
    s_store_name = s_ext_name = s_value_name = 0;
    s_id_name = s_key_name = s_type_name = s_value_attr_name = 0;

    std::lock_guard<std::mutex> lock(s_mutex);
    s_values.clear();
    s_imported_player = 0;
    s_last_player = 0;
}

void SaveStore::uninstall() {
    s_host = Host{};
    g_game = nullptr;
    g_offsets = nullptr;
    s_stash_set = nullptr;
    s_stash_get = nullptr;
    s_stash_remove = nullptr;
    s_names_ready = false;
}

void SaveStore::ensure_names() {
    if (s_names_ready || !s_host.register_name || !s_host.game_number || !s_host.savexml_namespace) return;
    if (!s_host.savexml_namespace()) return;

    s_store_name = s_host.register_name("x4native", "store");
    s_ext_name = s_host.register_name("x4native", "ext");
    s_value_name = s_host.register_name("x4native", "value");
    s_id_name = s_host.game_number("id");
    s_key_name = s_host.game_number("key");
    s_type_name = s_host.game_number("type");
    s_value_attr_name = s_host.game_number("value");
    s_names_ready = s_store_name && s_ext_name && s_value_name && s_id_name && s_key_name && s_type_name &&
                    s_value_attr_name;
    if (!s_names_ready && !s_names_logged) {
        s_names_logged = true;
        log_line("SaveXML store: a name the store writes is missing from this build; the store is off");
    }
}

// The game raises the world's start and runs the universe load in either order, and
// runs the load twice for one save; the values read belong to the world loaded, named
// by its player id, so the world's start keeps them only for that world.
void SaveStore::begin_load(const void* universe_node) {
    const int64_t player = s_host.game && s_host.game->GetPlayerID
        ? static_cast<int64_t>(s_host.game->GetPlayerID()) : 0;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_values.clear();
        s_imported_player = player;
    }

    ensure_names();
    if (!s_names_ready || !universe_node) return;

    for (SaveNode child : SaveNode(const_cast<void*>(universe_node)).children()) {
        if (child.element() == s_store_name) {
            read_node(child.object());
            break;
        }
    }

    size_t count = 0;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        for (const auto& [ext, values] : s_values) count += values.size();
    }
    log_line("SaveXML store: a save load read " + std::to_string(count) + " value(s) for world " +
             std::to_string(player));
}

void SaveStore::read_node(const void* store_node) {
    std::map<std::string, std::map<std::string, Value>> loaded;
    std::set<std::string> logged;

    for (SaveNode block : SaveNode(const_cast<void*>(store_node)).children()) {
        if (block.element() != s_ext_name) continue;

        const auto id = block.attribute(s_id_name);
        if (!id || id->empty()) continue;

        if (s_host.is_loaded && !s_host.is_loaded(id->c_str())) {
            if (logged.insert(*id).second) {
                log_line("SaveXML store: save holds values of '" + *id +
                         "' (not loaded); they are dropped on the next save");
            }
            continue;
        }

        for (SaveNode item : block.children()) {
            if (item.element() != s_value_name) continue;

            const auto key = item.attribute(s_key_name);
            const auto type = item.attribute(s_type_name);
            const auto text = item.attribute(s_value_attr_name);
            if (!key || key->empty() || !type || !text) {
                log_line("SaveXML store: " + *id + " carries a value without a key, a type or a value; skipped");
                continue;
            }

            Value value;
            Type parsed_type;
            if (!type_from_name(*type, &parsed_type) || !parse_value(parsed_type, *text, &value)) {
                log_line("SaveXML store: " + *id + " key '" + *key + "': value '" + *text + "' is not a valid " +
                         *type + "; skipped");
                continue;
            }
            loaded[*id][*key] = value;
        }
    }

    std::lock_guard<std::mutex> lock(s_mutex);
    s_values = std::move(loaded);
}

bool SaveStore::write(void* parent) {
    if (!s_names_ready || !parent || !g_game || !g_offsets) return false;
    if (!g_game->GameAlloc || !g_game->XmlNode_SetAttribute || !g_game->XmlNodeVector_PushBack) return false;

    std::map<std::string, std::map<std::string, Value>> snapshot;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        snapshot = s_values;
    }

    const bool any = std::any_of(snapshot.begin(), snapshot.end(), [](const auto& entry) {
        return !entry.second.empty() && (!s_host.is_loaded || s_host.is_loaded(entry.first.c_str()));
    });
    if (!any) return false;

    SaveNode store_node = SaveNode::create(s_store_name);
    if (!store_node) {
        log_line("SaveXML store: the store element could not be built");
        return false;
    }

    for (const auto& [ext_id, values] : snapshot) {
        if (values.empty()) continue;
        if (s_host.is_loaded && !s_host.is_loaded(ext_id.c_str())) continue;

        SaveNode block = SaveNode::create(s_ext_name);
        if (!block) {
            log_line("SaveXML store: " + ext_id + "'s block could not be built");
            return false;
        }
        block.set_attribute(s_id_name, ext_id.c_str());

        for (const auto& [key, value] : values) {
            SaveNode item = SaveNode::create(s_value_name);
            if (!item) {
                log_line("SaveXML store: " + ext_id + " key '" + key + "' could not be built");
                return false;
            }
            item.set_attribute(s_key_name, key.c_str());
            item.set_attribute(s_type_name, type_name(value.type));
            const std::string text = value_text(value);
            item.set_attribute(s_value_attr_name, text.c_str());
            block.append(item);
        }

        store_node.append(block);
    }

    SaveNode(parent).append(store_node);
    return true;
}

std::vector<void*> SaveStore::children(void* node) {
    std::vector<void*> out;
    if (!g_offsets) return out;
    for (SaveNode child : SaveNode(node).children()) out.push_back(child.object());
    return out;
}

void SaveStore::world_ready() {
    const int64_t player = s_host.game && s_host.game->GetPlayerID
        ? static_cast<int64_t>(s_host.game->GetPlayerID()) : 0;

    bool imported = false;
    int64_t last_player = 0;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        imported = player != 0 && s_imported_player == player;
        last_player = s_last_player;
    }

    if (imported) {
        if (s_stash_remove) s_stash_remove(kStashNamespace, kStashKey);
        if (player) {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_last_player = player;
        }
        log_line("SaveXML store: world " + std::to_string(player) + " ready, values from its save");
        return;
    }

    // The same world still loaded: a plain /reloadui, which keeps the core and
    // its values. A stash here could only belong to another world.
    if (player != 0 && last_player == player) {
        if (s_stash_remove) s_stash_remove(kStashNamespace, kStashKey);
        log_line("SaveXML store: world " + std::to_string(player) + " ready again, values kept");
        return;
    }

    bool restored = false;
    if (s_stash_get) {
        uint32_t size = 0;
        const auto* text = static_cast<const char*>(s_stash_get(kStashNamespace, kStashKey, &size));
        if (text && size > 1) {
            int64_t stashed_player = 0;
            std::map<std::string, std::map<std::string, Value>> values;
            const json parsed = json::parse(text, nullptr, false);
            if (!parsed.is_discarded() && parsed.is_object()) {
                stashed_player = parsed.value("player", static_cast<int64_t>(0));
                if (const auto found = parsed.find("values"); found != parsed.end() && found->is_array()) {
                    for (const json& item : *found) {
                        if (!item.is_object()) continue;
                        const std::string ext = item.value("ext", std::string{});
                        const std::string key = item.value("key", std::string{});
                        const std::string type = item.value("type", std::string{});
                        const std::string stored = item.value("value", std::string{});
                        Value value;
                        Type parsed_type;
                        if (ext.empty() || key.empty() || !type_from_name(type, &parsed_type) ||
                            !parse_value(parsed_type, stored, &value)) {
                            continue;
                        }
                        values[ext][key] = value;
                    }
                }
            }
            if (stashed_player != 0 && player != 0 && stashed_player == player) {
                std::lock_guard<std::mutex> lock(s_mutex);
                s_values = std::move(values);
                restored = true;
            }
        }
        if (s_stash_remove) s_stash_remove(kStashNamespace, kStashKey);
    }

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!restored) s_values.clear();
        s_last_player = player;
    }
    log_line("SaveXML store: world " + std::to_string(player) + " ready, " +
             (restored ? "values restored from the stash" : "a new world, values cleared"));
}

void SaveStore::stash(int64_t player_id) {
    if (!s_stash_set) return;

    json root;
    root["player"] = player_id;
    root["values"] = json::array();
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        for (const auto& [ext_id, values] : s_values) {
            for (const auto& [key, value] : values) {
                json item;
                item["ext"] = ext_id;
                item["key"] = key;
                item["type"] = type_name(value.type);
                item["value"] = value_text(value);
                root["values"].push_back(std::move(item));
            }
        }
    }

    const std::string text = root.dump();
    s_stash_set(kStashNamespace, kStashKey, text.c_str(), static_cast<uint32_t>(text.size() + 1));
}

bool SaveStore::set(const char* ext_id, const char* key, const Value& value) {
    if (!ext_id || !*ext_id || !key || !*key) return false;
    std::lock_guard<std::mutex> lock(s_mutex);
    s_values[ext_id][key] = value;
    return true;
}

bool SaveStore::get(const char* ext_id, const char* key, Value* out) {
    if (!ext_id || !key || !out) return false;
    std::lock_guard<std::mutex> lock(s_mutex);
    const auto ext = s_values.find(ext_id);
    if (ext == s_values.end()) return false;
    const auto value = ext->second.find(key);
    if (value == ext->second.end()) return false;
    *out = value->second;
    return true;
}

bool SaveStore::remove(const char* ext_id, const char* key) {
    if (!ext_id || !key) return false;
    std::lock_guard<std::mutex> lock(s_mutex);
    const auto ext = s_values.find(ext_id);
    if (ext == s_values.end()) return false;
    const bool removed = ext->second.erase(key) > 0;
    if (ext->second.empty()) s_values.erase(ext);
    return removed;
}

void SaveStore::log_line(const std::string& line) {
    if (s_host.log) s_host.log(line.c_str());
}

} // namespace x4n
