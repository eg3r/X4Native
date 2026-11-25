#include "save_xml.h"

#include <x4_manual_types.h>

#include <cstddef>
#include <cstring>

namespace x4n {

namespace {

/// The one character that separates an extension id from its local name. Every
/// registered name carries it and no game name does, so the lookups can send a
/// game name or a game number straight to the game without a table read.
constexpr char kSeparator = '.';

bool ascii_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool ascii_digit(char c) { return c >= '0' && c <= '9'; }

/// An XML name as the game's parser accepts one: a letter or `_` first, then
/// letters, digits, `_`, `-` or `.`. The separator is checked apart, as a name
/// carrying it is stored under another extension's form.
bool valid_xml_name(const char* name) {
    if (!name || !*name) return false;
    if (!ascii_letter(*name) && *name != '_') return false;
    for (const char* p = name + 1; *p; ++p) {
        if (!ascii_letter(*p) && !ascii_digit(*p) && *p != '_' && *p != '-' && *p != '.') return false;
    }
    return true;
}

bool carries_separator(const char* name) { return std::strchr(name, kSeparator) != nullptr; }

}  // namespace

std::mutex                          SaveXml::s_register_mutex;
std::atomic<const SaveXml::Table*>  SaveXml::s_table{nullptr};
std::vector<std::unique_ptr<SaveXml::Table>> SaveXml::s_retired;
std::atomic<int32_t>                SaveXml::s_first_added{0};
std::atomic<bool>                   SaveXml::s_attached{false};
std::atomic<SaveXml::FormatXMLNameFn> SaveXml::s_format_original{nullptr};
std::atomic<SaveXml::LookupNameFn>  SaveXml::s_lookup_original{nullptr};
std::atomic<SaveXml::LogFn>         SaveXml::s_log{nullptr};

std::mutex                          SaveXml::s_miss_mutex;
std::unordered_set<std::string, SaveXml::NameHash, SaveXml::NameEqual> SaveXml::s_missed;

/// Handle over the game's SaveXML namespace object and its built-in name
/// table: one 24-byte entry per name the game knows, the name at +0x00 and its
/// number at +0x10, held inline in the namespace object and sorted by number.
class SaveXmlHandle {
    const uint8_t* ns_;

    template <typename T>
    const T& at(size_t offset) const { return *reinterpret_cast<const T*>(ns_ + offset); }

    size_t entry(size_t index) const { return X4_SAVEXML_OFFSET_ENTRIES + index * X4_SAVEXML_ENTRY_SIZE; }

public:
    explicit SaveXmlHandle(const void* ns) : ns_(static_cast<const uint8_t*>(ns)) {}

    size_t count() const { return at<size_t>(X4_SAVEXML_OFFSET_ENTRY_COUNT); }
    const char* name(size_t index) const { return at<const char*>(entry(index) + X4_SAVEXML_ENTRY_OFFSET_NAME); }
    int32_t number(size_t index) const { return at<int32_t>(entry(index) + X4_SAVEXML_ENTRY_OFFSET_ID); }
};

/// The highest number the game's table uses.
static int32_t highest_game_number(SaveXmlHandle table) {
    int32_t highest = 0;
    for (size_t i = 0; i < table.count(); ++i) {
        const int32_t number = table.number(i);
        if (number > highest) highest = number;
    }
    return highest;
}

void SaveXml::attach(FormatXMLNameFn format, LookupNameFn lookup) {
    s_format_original = format;
    s_lookup_original = lookup;
    s_attached = true;
}

void SaveXml::detach() {
    // The originals and the tables stay: a detour may be inside one while the
    // core removes the hooks. Nothing frees them before the process exits.
    s_attached = false;

    std::lock_guard<std::mutex> miss_lock(s_miss_mutex);
    s_missed.clear();
}

bool SaveXml::attached() { return s_attached.load(std::memory_order_acquire); }

void SaveXml::set_log(LogFn log) { s_log.store(log, std::memory_order_relaxed); }

void SaveXml::log_line(const std::string& line) {
    const LogFn sink = s_log.load(std::memory_order_relaxed);
    if (sink) sink(line.c_str());
}

int32_t SaveXml::register_name(const void* ns, const char* ext_id, const char* local_name) {
    if (!attached()) return 0;

    if (!ext_id || !*ext_id) {
        log_line("SaveXML: a registration arrived without a calling extension; refused");
        return 0;
    }
    if (!valid_xml_name(ext_id)) {
        log_line(std::string("SaveXML: extension id '") + ext_id +
                 "' refused: not a valid XML name, so its names cannot be written to a save");
        return 0;
    }
    if (carries_separator(ext_id)) {
        log_line(std::string("SaveXML: extension id '") + ext_id + "' refused: it contains the separator '" +
                 kSeparator + "'");
        return 0;
    }
    if (!local_name || !*local_name) {
        log_line(std::string("SaveXML: ") + ext_id + ": an empty local name refused");
        return 0;
    }
    if (!valid_xml_name(local_name)) {
        log_line(std::string("SaveXML: ") + ext_id + ": local name '" + local_name +
                 "' refused: not a valid XML name");
        return 0;
    }
    if (carries_separator(local_name)) {
        log_line(std::string("SaveXML: ") + ext_id + ": local name '" + local_name +
                 "' refused: it contains the separator '" + kSeparator + "'");
        return 0;
    }
    if (!ns) return 0;

    std::string full(ext_id);
    full.push_back(kSeparator);
    full += local_name;

    std::lock_guard<std::mutex> lock(s_register_mutex);

    const Table* old = s_table.load(std::memory_order_acquire);
    if (old) {
        const auto found = old->numbers.find(full);
        if (found != old->numbers.end()) return found->second;
    }

    int32_t first = s_first_added.load(std::memory_order_relaxed);
    if (first == 0) {
        first = highest_game_number(SaveXmlHandle(ns)) + 1;
        if (first <= 0) return 0;
    }

    auto next = std::make_unique<Table>();
    if (old) {
        next->numbers = old->numbers;
        next->names   = old->names;
    }

    const int32_t number = first + static_cast<int32_t>(next->names.size());
    next->numbers.emplace(full, number);
    next->names.push_back(full);

    // The table is published before its first number, so a detour that sees a
    // number at or above `first_added` also sees the table holding it. Both are
    // kept until the core shuts down: a detour on another thread may still be
    // reading a replaced table.
    const Table* published = next.get();
    s_retired.push_back(std::move(next));
    s_table.store(published, std::memory_order_release);
    s_first_added.store(first, std::memory_order_release);

    log_line(std::string("SaveXML: ") + ext_id + " registered '" + full + "' = " + std::to_string(number));
    return number;
}

int32_t SaveXml::game_number(const void* ns, const char* name) {
    if (!ns || !name || !*name) return 0;

    const SaveXmlHandle table(ns);
    for (size_t i = 0; i < table.count(); ++i) {
        const char* entry_name = table.name(i);
        if (entry_name && std::strcmp(entry_name, name) == 0) return table.number(i);
    }
    return 0;
}

void SaveXml::begin_load() {
    std::lock_guard<std::mutex> lock(s_miss_mutex);
    s_missed.clear();
}

const char* SaveXml::format_xml_name(const int32_t* id) {
    if (id) {
        const int32_t first = s_first_added.load(std::memory_order_acquire);
        if (first != 0 && *id >= first) {
            const Table* table = s_table.load(std::memory_order_acquire);
            if (table) {
                const size_t index = static_cast<size_t>(*id - first);
                if (index < table->names.size()) return table->names[index].c_str();
            }
        }
    }
    return s_format_original.load()(id);
}

int32_t SaveXml::lookup_name(const char* name, const void* context) {
    if (name && carries_separator(name)) {
        const Table* table = s_table.load(std::memory_order_acquire);
        if (table) {
            const auto added = table->numbers.find(name);
            if (added != table->numbers.end()) return added->second;
        }

        {
            std::lock_guard<std::mutex> lock(s_miss_mutex);
            if (s_missed.find(std::string_view(name)) != s_missed.end()) return 0;
            s_missed.emplace(name);
        }

        const char* separator = std::strchr(name, kSeparator);
        const std::string missing(name, static_cast<size_t>(separator - name));
        log_line(std::string("SaveXML: save contains data from '") + missing + "' (not loaded): '" + name +
                 "' skipped, it is dropped on the next save");
        return 0;
    }
    return s_lookup_original.load()(name, context);
}

} // namespace x4n
