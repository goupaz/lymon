#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <murmurhash.h>
#include <stdbool.h>
#define LOAD_FACTOR(x) (((x) * 775) >> 10)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
typedef struct hash_entry {
    void* key;
    union {
        void* v;
        uint64_t uintv;
        int64_t intv;
    } val;
    uint64_t distance;
    bool used;
} hash_entry;

typedef struct hash_map { 
    uint64_t    max_distance;
    size_t      capacity;
    size_t      size;
    uint64_t    insert_dist;
    size_t      (*hash_fn)(void* key);
    bool        (*hash_cmp)(void* hk, void*ck);
    void        (*hash_dealloc)(hash_entry* node);
    hash_entry* hash_table;
} hash_map;

hash_map* init_hash(hash_map map_info);
void hash_insert(hash_entry* entry, hash_map* map_info);
hash_entry* hash_get(hash_entry* entry, hash_map* map_info);
void dealloc_hash(hash_map* ih);
bool hash_remove(hash_map* mi, void* key);
#define setVal(he, val) he.v = val
#define setSint(he, val) he.intv = val
#define setUint(he, val) he.uintv = val
#define getVal(he) he.v
