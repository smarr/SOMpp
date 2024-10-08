#pragma once

#include <string>

inline bool ReplacePattern(std::string& str, const char* pattern,
                           const char* replacement) {
    size_t const pos = str.find(pattern);
    if (pos == std::string::npos) {
        return false;
    }

    str.replace(pos, strlen(pattern), replacement);
    return true;
}

inline bool ReplacePattern(std::string& str, const char* pattern,
                           std::string& replacement) {
    size_t const pos = str.find(pattern);
    if (pos == std::string::npos) {
        return false;
    }

    str.replace(pos, strlen(pattern), replacement);
    return true;
}
