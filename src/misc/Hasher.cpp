#include "Hasher.h"

#include <cstddef>
#include <functional>
#include <string>

using namespace std;

/* Hash a string with standard cpp hash function */
size_t ByteCodeHasher::HashString(const std::string& str) {
    const std::hash<std::string> hasher;
    return hasher(str);
}