#ifndef ART_KEY_H
#define ART_KEY_H

#include <stdint.h>
#include <cstring>
#include <memory>
#include <assert.h>
#include <malloc.h>
#include "cacheops.h"
typedef struct Key {
    uint64_t value;
    size_t key_len;
    uint8_t fkey[];

    static inline Key *make_leaf(char *key, size_t key_len, uint64_t value);

    static inline Key *make_leaf(uint64_t key, size_t key_len, uint64_t value);

    inline size_t getKeyLen() const;
} Key;

inline Key *Key::make_leaf(char *key, size_t key_len, uint64_t value)
{
    void *aligned_alloc = memalign(64, sizeof(Key) + key_len);
    Key *k = reinterpret_cast<Key *> (aligned_alloc);

    k->value = value;
    k->key_len = key_len;
    memcpy(k->fkey, key, key_len);
#ifdef BUGFIX
    PMCHECK::clflush((char*)k, sizeof(Key) + key_len, false, true);
#endif
    return k;
}

inline Key *Key::make_leaf(uint64_t key, size_t key_len, uint64_t value)
{
    void *aligned_alloc = memalign(64, sizeof(Key) + key_len);
    Key *k = reinterpret_cast<Key *> (aligned_alloc);

    k->value = value;
    k->key_len = key_len;
    reinterpret_cast<uint64_t *> (&k->fkey[0])[0] = __builtin_bswap64(key);
#ifdef BUGFIX
    PMCHECK::clflush((char*)k, sizeof(Key) + key_len, false, true);
#endif
    return k;
}

inline size_t Key::getKeyLen() const { return key_len; }

#endif // ART_KEY_H