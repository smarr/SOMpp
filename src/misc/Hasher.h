#pragma once

#include <fstream>
#include <string>

class ByteCodeHasher {
public:
    static size_t HashString(const std::string& str);
    static std::string GetFile(const std::string& pathWithFileName);
};