#include "save_xml.h"

#include <x4_manual_types.h>

#include <cstddef>
#include <cstring>

namespace x4n {

std::mutex                            SaveXml::s_mutex;
std::map<std::string, int32_t>        SaveXml::s_numbers;
std::map<int32_t, std::string>        SaveXml::s_names;
std::atomic<SaveXml::FormatXMLNameFn> SaveXml::s_format_original{nullptr};
std::atomic<SaveXml::LookupNameFn>    SaveXml::s_lookup_original{nullptr};
std::atomic<bool>                     SaveXml::s_any{false};

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

/// The game's number for a name, or 0; and the highest number the game uses.
static int32_t game_number(SaveXmlHandle table, const char* name, int32_t* highest) {
    int32_t found = 0;
    for (size_t i = 0; i < table.count(); ++i) {
        const int32_t number = table.number(i);
        const char* entry_name = table.name(i);
        if (number > *highest) *highest = number;
        if (!found && entry_name && std::strcmp(entry_name, name) == 0) found = number;
    }
    return found;
}

void SaveXml::attach(FormatXMLNameFn format, LookupNameFn lookup) {
    s_format_original = format;
    s_lookup_original = lookup;
}

void SaveXml::detach() {
    s_format_original = nullptr;
    s_lookup_original = nullptr;
}

bool SaveXml::attached() { return s_format_original && s_lookup_original; }

int32_t SaveXml::register_name(const void* ns, const char* name) {
    if (!attached() || !ns || !name || !*name) return 0;

    std::lock_guard<std::mutex> lock(s_mutex);
    const auto added = s_numbers.find(name);
    if (added != s_numbers.end()) return added->second;

    int32_t highest = 0;
    const int32_t known = game_number(SaveXmlHandle(ns), name, &highest);
    if (known) return known;

    int32_t next = highest + 1;
    if (!s_names.empty() && s_names.rbegin()->first >= next) next = s_names.rbegin()->first + 1;
    s_numbers.emplace(name, next);
    s_names.emplace(next, name);
    s_any = true;
    return next;
}

const char* SaveXml::format_xml_name(const int32_t* id) {
    if (id && s_any) {
        std::lock_guard<std::mutex> lock(s_mutex);
        const auto added = s_names.find(*id);
        if (added != s_names.end()) return added->second.c_str();
    }
    return s_format_original.load()(id);
}

int32_t SaveXml::lookup_name(const char* name, const void* context) {
    if (name && s_any) {
        std::lock_guard<std::mutex> lock(s_mutex);
        const auto added = s_numbers.find(name);
        if (added != s_numbers.end()) return added->second;
    }
    return s_lookup_original.load()(name, context);
}

} // namespace x4n
