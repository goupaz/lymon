#include <hacker.h>
#include <hashmap.h>

hash_map* init_hash(hash_map mi) {
    hash_map*   hm = calloc(1, sizeof(hash_map));
    hash_entry* he = calloc(mi.capacity, sizeof(hash_entry));
    memcpy(hm, &mi, sizeof(mi));
    hm->hash_table = he;
    return hm;
}

// Map hash into the range [0,N) more efficiently than (hash % N):
// https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
static inline uint32_t reduce(uint32_t x, uint32_t N) {
        return ((uint64_t) x * (uint64_t) N) >> 32;
}

void dealloc_hash(hash_map* ih) {
    for(int ii = 0; ii < ih->capacity; ii ++ ) {
        struct hash_entry* node = &ih->hash_table[ii];
        if(node->used) {
            if(ih->hash_dealloc)
                ih->hash_dealloc(node);
            node->used = false;
        }
    }
}

bool hash_remove(hash_map* mi, void* key) {
    size_t start_index = reduce(mi->hash_fn(key), mi->capacity);
    for(int lwp = 0; lwp <= mi->max_distance; lwp++) {
        int index = ((start_index + lwp) % mi->capacity);
        hash_entry* entry = &mi->hash_table[index];
        if(!entry->used) {
            fprintf(stderr, "key not found\n");
            return false;
        }
        if(!(mi->hash_cmp(entry->key, key))) 
            continue;
        for(; lwp < mi->capacity; lwp++) {
            int next_idx = (start_index + lwp + 1) % mi->capacity;
            hash_entry *next_entry = &mi->hash_table[next_idx];
            if(!next_entry->used || next_entry->distance == 0) {
                entry->used = false;
                mi->size-=1;
                mi->hash_dealloc(entry);
                return true;
            }
            *entry = *next_entry;
            entry->distance-=1;
            entry = next_entry;
        }
    }
    return false;
}

void resize_hash(hash_map* ih) {
    if(ih->size >= LOAD_FACTOR(ih->capacity)) {
        ih->size = 0;
        size_t oldcapacity = ih->capacity;
        ih->capacity = oldcapacity * 2;
        struct hash_entry* oldentries = ih->hash_table;
        ih->hash_table = calloc(ih->capacity, sizeof(hash_entry));
        for(int ii = 0; ii < oldcapacity; ii++) {
            struct hash_entry* node = &oldentries[ii];
            if(node->used)
                hash_insert(&((hash_entry){.key = node->key, .val = node->val}), ih);
        }
        free(oldentries);
    }
}

void hash_insert(hash_entry* entry, hash_map* mi) {
    resize_hash(mi);
    uint64_t insert_dist = mi->insert_dist; 
    uint64_t max_distance = mi->max_distance;
    size_t index = reduce(mi->hash_fn(entry->key), mi->capacity);
    struct hash_entry* node = &mi->hash_table[index];
    mi->size+=1;
    while(node->used && node->key != entry->key) {
        uint64_t prob_dist = node->distance; 
        max_distance = MAX(max_distance, insert_dist);
        if(prob_dist < insert_dist) {
            hash_entry swapped_node = *node;
            node->key = entry->key;
            node->val = entry->val;
            node->distance = insert_dist;
            mi->insert_dist = swapped_node.distance;
            hash_insert(&swapped_node, mi); 
        } else {
            index += (index == mi->capacity) ? (index % mi->capacity): 1;
            node = &mi->hash_table[index];
            insert_dist+=1;
            continue;
        }
    }
    mi->max_distance = MAX(max_distance, insert_dist);
    // insert entry to hash table
    mi->hash_table[index].val = entry->val;
    mi->hash_table[index].key = entry->key;
    mi->hash_table[index].used = true;
    mi->hash_table[index].distance = insert_dist;
}

hash_entry* hash_get(hash_entry* entry, hash_map* mi) {
    size_t start_index  = reduce(mi->hash_fn(entry->key), mi->capacity);
    for(int ii = 0; ii <= mi->max_distance; ii++) {
        int index = (start_index + ii) % mi->capacity;
        hash_entry* node = &mi->hash_table[index];
        if(!node->used) return NULL;
        if(node->key == entry->key || mi->hash_cmp(node->key, entry->key)) return node;
    }
    return NULL;
}
