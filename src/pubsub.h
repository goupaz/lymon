#include <murmurhash.h>
typedef struct pbObj {
    void* messagePtr;
} pbObj;

size_t pbObjHashFn(void *ptr);
bool pbObjHashCmp(void *hk, void* ck);
void pbObjHashDealloc(hash_entry* node);
void createChannel(hash_map* tbl, sObj* aobj, List* channel);
hash_entry* getChannel(hash_map* tbl, sObj* aobj);
void subscribe(clientObj* cobj);
void publish(clientObj* cobj);
void unsubscribe(clientObj* cobj);
