#ifndef MURMUR3HASH_H
#define MURMUR3HASH_H

#include <cstdint>
#include <cstddef>

class Murmur3Hash {
public:
    // Computes a 32-bit Murmur3 hash of the input key.
    static uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed);

private:
    static inline uint32_t murmur_32_scramble(uint32_t k);
};

#endif // MURMUR3HASH_H
