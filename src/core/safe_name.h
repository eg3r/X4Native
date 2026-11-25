// ---------------------------------------------------------------------------
// safe_name.h — check for file names that extensions supply
//
// Extensions name their own log files ("logfile" in x4native.json,
// x4n::log::set_log_file(), x4n::log::to_file()). Such a name is joined onto
// the extension's log folder, so it must not be able to leave that folder.
// ---------------------------------------------------------------------------
#pragma once

#include <cctype>
#include <string>
#include <string_view>

namespace x4n {

// Returns nullptr when `name` is a relative path that stays below the folder
// it is joined onto, otherwise a short reason for the log. Sub-folders are
// allowed ("events/2026.log"). Rejected:
//   - an empty name
//   - anything rooted: "\x" and "/x" (the root of the current drive),
//     "\\server\share\x", "\\?\C:\x"
//   - ':' anywhere: a drive ("C:\x", and "D:x", which is relative to drive
//     D's current folder) or an alternate data stream ("x.log:stream")
//   - a ".." component
//   - a component that Windows maps to a device, with or without an
//     extension ("nul", "con.log", "COM1.txt")
//   - a component that ends in '.' or ' ': Windows drops those characters,
//     so the name on disk would not be the name that was checked
inline const char* unsafe_relative_name_reason(std::string_view name) {
    if (name.empty()) return "empty filename";
    if (name.front() == '\\' || name.front() == '/') return "rooted path";
    if (name.find(':') != std::string_view::npos) return "':' in path";

    size_t start = 0;
    while (start <= name.size()) {
        size_t end = name.find_first_of("\\/", start);
        if (end == std::string_view::npos) end = name.size();
        std::string_view part = name.substr(start, end - start);
        start = end + 1;

        if (part.empty() || part == ".") continue;
        if (part == "..") return "'..' in path";
        if (part.back() == '.' || part.back() == ' ') return "name ending in '.' or ' '";

        // Device names count up to the first '.', ignoring spaces before it
        // and letter case: "NUL", "nul.txt" and "Nul .log" are all the device.
        std::string stem(part.substr(0, part.find('.')));
        while (!stem.empty() && stem.back() == ' ') stem.pop_back();
        for (char& c : stem) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        bool numbered = stem.size() == 4 && (stem.starts_with("com") || stem.starts_with("lpt")) &&
                        stem[3] >= '0' && stem[3] <= '9';
        if (numbered || stem == "con" || stem == "prn" || stem == "aux" || stem == "nul" ||
            stem == "conin$" || stem == "conout$")
            return "device name";
    }
    return nullptr;
}

} // namespace x4n
