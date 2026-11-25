#include "save_store.h"

#include "logger.h"

#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace x4n {

namespace {

using Values = std::map<std::string, std::string, std::less<>>;   // key -> value, both raw

std::mutex                                     g_mutex;
std::map<std::string, Values, std::less<>>     g_blocks;    // extension id -> its values
std::set<std::pair<std::string, std::string>>  g_dirty;     // (extension id, key) changed since the last take
bool   g_dirty_all = false;    // the Lua side has to be rewritten completely
bool   g_active    = false;    // a game is loaded and g_blocks holds its values
size_t g_total     = 0;        // encoded bytes of all keys and values
size_t g_import_rejected = 0;
bool   g_warned_inactive = false;   // each warning is logged once per Lua state
bool   g_warned_full     = false;
bool   g_warned_rejected = false;

// What take_changes hands out: the encoded strings and the array pointing at them.
struct Change {
    std::string ext, key, value;
    bool removed;
};
std::vector<Change>      g_out_data;
std::vector<SaveChangeC> g_out;

constexpr char kStashNamespace[] = X4N_SAVE_STASH_NAMESPACE;
constexpr char kStashKey[]       = X4N_SAVE_STASH_KEY;
constexpr char kStashMagic[4]    = {'X', 'N', 'S', '1'};

bool is_plain(unsigned char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        || c == '_' || c == '-' || c == '.';
}

size_t encoded_size(std::string_view raw) {
    size_t size = 1;   // the leading ':'
    for (const unsigned char c : raw) size += is_plain(c) ? 1 : 3;
    return size;
}

size_t entry_size(std::string_view key, std::string_view value) {
    return encoded_size(key) + encoded_size(value);
}

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

void drop_all_locked() {
    g_blocks.clear();
    g_dirty.clear();
    g_dirty_all = false;
    g_total = 0;
}

// --- Stash blob: magic, active flag, then length-prefixed strings ----------

void put_u32(std::string& blob, size_t value) {
    const uint32_t v = static_cast<uint32_t>(value);
    blob.append(reinterpret_cast<const char*>(&v), sizeof(v));
}

void put_string(std::string& blob, std::string_view s) {
    put_u32(blob, s.size());
    blob.append(s);
}

bool take_u32(std::string_view& blob, uint32_t& value) {
    if (blob.size() < sizeof(value)) return false;
    std::memcpy(&value, blob.data(), sizeof(value));
    blob.remove_prefix(sizeof(value));
    return true;
}

bool take_string(std::string_view& blob, std::string& s) {
    uint32_t size = 0;
    if (!take_u32(blob, size) || blob.size() < size) return false;
    s.assign(blob.data(), size);
    blob.remove_prefix(size);
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Encoding
// ---------------------------------------------------------------------------

std::string SaveStore::encode(std::string_view raw) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(encoded_size(raw));
    out.push_back(':');
    for (const unsigned char c : raw) {
        if (is_plain(c)) {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(kHex[c >> 4]);
            out.push_back(kHex[c & 15]);
        }
    }
    return out;
}

bool SaveStore::decode(std::string_view encoded, std::string& out) {
    out.clear();
    if (encoded.empty() || encoded[0] != ':') return false;
    out.reserve(encoded.size() - 1);
    for (size_t i = 1; i < encoded.size(); ++i) {
        const char c = encoded[i];
        if (is_plain(static_cast<unsigned char>(c))) {
            out.push_back(c);
            continue;
        }
        if (c != '%' || i + 2 >= encoded.size()) return false;
        const int high = hex_value(encoded[i + 1]);
        const int low  = hex_value(encoded[i + 2]);
        if (high < 0 || low < 0) return false;
        out.push_back(static_cast<char>(high * 16 + low));
        i += 2;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Extensions
// ---------------------------------------------------------------------------

bool SaveStore::set(const char* ext_id, const char* key, const void* data, uint32_t size) {
    if (!ext_id || !*ext_id || !key || (!data && size > 0)) return false;
    const std::string_view key_view(key);
    if (key_view.empty() || key_view.size() > kMaxKeyLength || size > kMaxValueSize) {
        std::lock_guard lock(g_mutex);
        if (!g_warned_rejected) {
            g_warned_rejected = true;
            Logger::warn("Save data: rejected '{}' of {} ({} bytes): a key has 1 to {} bytes, a value at most {}",
                         key_view.substr(0, kMaxKeyLength), ext_id, size, kMaxKeyLength, kMaxValueSize);
        }
        return false;
    }
    const std::string_view value(size ? static_cast<const char*>(data) : "", size);

    std::lock_guard lock(g_mutex);
    if (!g_active) {
        if (!g_warned_inactive) {
            g_warned_inactive = true;
            Logger::warn("Save data: {} set '{}' before this game's values were loaded; "
                         "values can be read and stored from on_save_data_ready on",
                         ext_id, key_view);
        }
        return false;
    }

    size_t old_size = 0;
    const auto block = g_blocks.find(std::string_view(ext_id));
    if (block != g_blocks.end()) {
        const auto existing = block->second.find(key_view);
        if (existing != block->second.end()) {
            if (existing->second == value) return true;   // unchanged: nothing to write
            old_size = entry_size(key_view, existing->second);
        }
    }
    const size_t new_total = g_total - old_size + entry_size(key_view, value);
    if (new_total > kMaxTotalSize) {
        if (!g_warned_full) {
            g_warned_full = true;
            Logger::warn("Save data: store is full ({} of {} bytes); rejected '{}' of {}",
                         g_total, kMaxTotalSize, key_view, ext_id);
        }
        return false;
    }

    g_blocks[ext_id].insert_or_assign(std::string(key_view), std::string(value));
    g_total = new_total;
    g_dirty.emplace(ext_id, key_view);
    return true;
}

const void* SaveStore::get(const char* ext_id, const char* key, uint32_t* out_size) {
    if (out_size) *out_size = 0;
    if (!ext_id || !key) return nullptr;

    std::lock_guard lock(g_mutex);
    if (!g_active) return nullptr;
    const auto block = g_blocks.find(std::string_view(ext_id));
    if (block == g_blocks.end()) return nullptr;
    const auto value = block->second.find(std::string_view(key));
    if (value == block->second.end()) return nullptr;
    if (out_size) *out_size = static_cast<uint32_t>(value->second.size());
    return value->second.data();   // std::string: the bytes are followed by a NUL
}

bool SaveStore::remove(const char* ext_id, const char* key) {
    if (!ext_id || !key) return false;

    std::lock_guard lock(g_mutex);
    if (!g_active) return false;
    const auto block = g_blocks.find(std::string_view(ext_id));
    if (block == g_blocks.end()) return false;
    const auto value = block->second.find(std::string_view(key));
    if (value == block->second.end()) return false;

    g_total -= entry_size(value->first, value->second);
    g_dirty.emplace(block->first, value->first);
    block->second.erase(value);
    if (block->second.empty()) g_blocks.erase(block);
    return true;
}

void SaveStore::clear(const char* ext_id) {
    if (!ext_id) return;

    std::lock_guard lock(g_mutex);
    if (!g_active) return;
    const auto block = g_blocks.find(std::string_view(ext_id));
    if (block == g_blocks.end()) return;
    for (const auto& [key, value] : block->second) {
        g_total -= entry_size(key, value);
        g_dirty.emplace(block->first, key);
    }
    g_blocks.erase(block);
}

bool SaveStore::ready() {
    std::lock_guard lock(g_mutex);
    return g_active;
}

// ---------------------------------------------------------------------------
// The Lua side
// ---------------------------------------------------------------------------

bool SaveStore::reset(bool game_active) {
    std::lock_guard lock(g_mutex);
    g_out.clear();
    g_out_data.clear();
    g_warned_inactive = false;
    g_warned_full = false;
    g_warned_rejected = false;
    if (game_active && g_active) {
        g_dirty_all = true;   // /reloadui: the values stay, the new Lua state needs all of them
        return true;
    }
    drop_all_locked();
    g_active = false;
    return false;
}

void SaveStore::import_begin() {
    std::lock_guard lock(g_mutex);
    drop_all_locked();
    g_active = false;
    g_import_rejected = 0;
}

void SaveStore::import_entry(const char* ext, const char* key, const char* value) {
    std::string ext_id, raw_key, raw_value;
    const bool ok = ext && key && value && decode(ext, ext_id) && decode(key, raw_key)
                 && decode(value, raw_value) && !ext_id.empty() && !raw_key.empty();

    std::lock_guard lock(g_mutex);
    if (!ok) {
        ++g_import_rejected;
        return;
    }
    Values& values = g_blocks[ext_id];
    const auto existing = values.find(raw_key);
    if (existing != values.end()) g_total -= entry_size(existing->first, existing->second);
    g_total += entry_size(raw_key, raw_value);
    values.insert_or_assign(std::move(raw_key), std::move(raw_value));
}

void SaveStore::import_end() {
    std::lock_guard lock(g_mutex);
    g_active = true;
    g_dirty.clear();
    g_dirty_all = false;

    size_t values = 0;
    for (const auto& [ext, block] : g_blocks) values += block.size();
    Logger::info("Save data: {} values of {} extensions in this game ({} bytes)",
                 values, g_blocks.size(), g_total);
    if (g_import_rejected)
        Logger::warn("Save data: skipped {} entries of the savegame that are not in X4Native's format",
                     g_import_rejected);
}

int SaveStore::take_changes(bool full, const SaveChangeC** out, bool* out_full) {
    if (out) *out = nullptr;
    if (out_full) *out_full = false;

    std::lock_guard lock(g_mutex);
    g_out.clear();
    g_out_data.clear();
    if (!g_active) return 0;

    const bool all = full || g_dirty_all;
    if (!all && g_dirty.empty()) return 0;

    if (all) {
        for (const auto& [ext, values] : g_blocks)
            for (const auto& [key, value] : values)
                g_out_data.push_back({encode(ext), encode(key), encode(value), false});
    } else {
        for (const auto& [ext, key] : g_dirty) {
            const auto block = g_blocks.find(ext);
            const Values::const_iterator value =
                block != g_blocks.end() ? block->second.find(key) : Values::const_iterator{};
            if (block != g_blocks.end() && value != block->second.end())
                g_out_data.push_back({encode(ext), encode(key), encode(value->second), false});
            else
                g_out_data.push_back({encode(ext), encode(key), {}, true});
        }
    }
    g_dirty.clear();
    g_dirty_all = false;

    // g_out_data is complete, so its strings no longer move.
    g_out.reserve(g_out_data.size());
    for (const Change& change : g_out_data)
        g_out.push_back({change.ext.c_str(), change.key.c_str(),
                         change.removed ? nullptr : change.value.c_str()});
    if (out) *out = g_out.data();
    if (out_full) *out_full = all;
    return static_cast<int>(g_out.size());
}

// ---------------------------------------------------------------------------
// Core hot-reload
// ---------------------------------------------------------------------------

void SaveStore::stash_snapshot(stash_set_fn stash_set) {
    if (!stash_set) return;

    std::string blob(kStashMagic, sizeof(kStashMagic));
    {
        std::lock_guard lock(g_mutex);
        blob.push_back(g_active ? 1 : 0);
        put_u32(blob, g_blocks.size());
        for (const auto& [ext, values] : g_blocks) {
            put_string(blob, ext);
            put_u32(blob, values.size());
            for (const auto& [key, value] : values) {
                put_string(blob, key);
                put_string(blob, value);
            }
        }
    }
    stash_set(kStashNamespace, kStashKey, blob.data(), static_cast<uint32_t>(blob.size()));
}

void SaveStore::stash_restore(stash_get_fn stash_get, stash_remove_fn stash_remove) {
    if (!stash_get) return;
    uint32_t size = 0;
    const void* data = stash_get(kStashNamespace, kStashKey, &size);
    if (!data) return;

    std::string_view blob(static_cast<const char*>(data), size);
    std::map<std::string, Values, std::less<>> blocks;
    bool active = false;
    size_t total = 0;
    uint32_t block_count = 0;
    bool ok = blob.size() > sizeof(kStashMagic)
           && std::memcmp(blob.data(), kStashMagic, sizeof(kStashMagic)) == 0;
    if (ok) {
        active = blob[sizeof(kStashMagic)] != 0;
        blob.remove_prefix(sizeof(kStashMagic) + 1);
        ok = take_u32(blob, block_count);
    }
    for (uint32_t b = 0; ok && b < block_count; ++b) {
        std::string ext;
        uint32_t value_count = 0;
        ok = take_string(blob, ext) && take_u32(blob, value_count);
        for (uint32_t v = 0; ok && v < value_count; ++v) {
            std::string key, value;
            ok = take_string(blob, key) && take_string(blob, value);
            if (ok) {
                total += entry_size(key, value);
                blocks[ext].insert_or_assign(std::move(key), std::move(value));
            }
        }
    }

    if (ok) {
        std::lock_guard lock(g_mutex);
        g_blocks = std::move(blocks);
        g_dirty.clear();
        g_dirty_all = true;   // the Lua side is rewritten from these values
        g_active = active;
        g_total = total;
    } else {
        Logger::warn("Save data: could not restore the values kept over the core reload");
    }
    if (stash_remove) stash_remove(kStashNamespace, kStashKey);
}

} // namespace x4n
