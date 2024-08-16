#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

template <typename T>
inline bool Contains(std::vector<T>& vec, T elem) {
    auto it = std::find(vec.begin(), vec.end(), elem);
    return it != vec.end();
}

template <typename T>
inline int64_t IndexOf(std::vector<T>& vec, T elem) {
    auto it = std::find(vec.begin(), vec.end(), elem);
    if (it != vec.end()) {
        return it - vec.begin();
    }
    return -1;
}
