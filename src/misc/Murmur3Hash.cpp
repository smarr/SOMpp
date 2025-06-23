#include "Murmur3Hash.h"
#include <cstdint>
#include <cstring>

// static member function defined as part of Murmur3Hash
uint32_t Murmur3Hash::murmur3_32(const uint8_t* key, size_t len, uint32_t seed) {
    uint32_t h = seed;
    uint32_t k = 0;

    for (size_t i = len >> 2U; i != 0U; i--) {
        std::memcpy(&k, key, sizeof(uint32_t));
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13U) | (h >> 19U);
        h = h * 5 + 0xe6546b64;
    }

    k = 0;
    for (size_t i = len & 3U; i != 0U; i--) {
        k <<= 8U;
        k |= key[i - 1];
    }

    h ^= murmur_32_scramble(k);
    h ^= len;
    h ^= h >> 16U;
    h *= 0x85ebca6b;
    h ^= h >> 13U;
    h *= 0xc2b2ae35;
    h ^= h >> 16U;

    return h;
}

// static inline method — OK to leave outside class as it’s inline
inline uint32_t Murmur3Hash::murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15U) | (k >> 17U);
    k *= 0x1b873593;
    return k;
}