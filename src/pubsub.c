#include <server.h>
#include <pubsub.h>
#include <sys/socket.h>
size_t pbObjHashFn(void *ptr) {
    const char* key = ((sObj*)ptr)->ptr;
    return murmurhash(key, strlen(key), 0);
}

bool pbObjHashCmp(void *hk, void* ck) {
   const char* ck_key = ((sObj*)ck)->ptr;
   const char* hk_key = ((sObj*)hk)->ptr;
   return (strcmp(ck_key, hk_key) == 0);
}

void pbObjHashDealloc(hash_entry* node) {
    if(node->key) refcount_dec(node->key);
    if(getVal(node->val)) free(getVal(node->val));
}

void publish(clientObj* us) {
    char *msg = us->args[2]->ptr;
    hash_entry* entry = hash_get(&((hash_entry){.key = us->args[1]}), srvObj.pubsub);
    List* clients = NULL;
    if(entry) {
        /*iterate over client list and broadcast message to clients*/
        clients = getVal(entry->val);
        Node* node = clients->head;
        clientObj* cl;
        while(node != NULL) {
            cl = node->ptr;
            sendBuffer(cl->fd, msg);
            node = node->next;
        }
    }
}

void unsubscribeCmd(clientObj* cobj, sObj* aobj) {
    hash_entry* entry = hash_get(&((hash_entry){.key = aobj}), srvObj.pubsub);
    Node* clnode;
    refcount_inc(aobj);
    if(entry) {
        hash_remove(cobj->pubsub, aobj);
        List* clients = getVal(entry->val);
        clnode = searchByKey(clients, cobj);
        deleteNode(clients, clnode);
        refcount_dec(aobj);
        if(list_count(clients) == 0)
            /* if there is no client anymore remove object in hash table */
            hash_remove(srvObj.pubsub, aobj);
    }
}

void unsubscribe(clientObj* cobj) {
    int i;
    for(i = 1; i < cobj->argc; i++){
        unsubscribeCmd(cobj, cobj->args[i]);
    }
}

void subscribeCmd(clientObj* cobj, sObj* aobj) {
    hash_entry* entry = hash_get(&((hash_entry){.key = aobj}), srvObj.pubsub);
    List* clients = NULL;
    refcount_inc(aobj);
    hash_insert(&((hash_entry){.key = aobj, .val.v = clients}), cobj->pubsub);
    if(entry) {
        clients = getVal(entry->val);
    } else {
        refcount_inc(aobj);
        clients = createList(false, NULL);
        hash_insert(&((hash_entry){.key = aobj, .val.v = clients}), srvObj.pubsub);
    }
    addNodeTail(clients, cobj);
}

void subscribe(clientObj* cobj) {
    int i;
    for(i = 1; i < cobj->argc; i++){
        subscribeCmd(cobj, cobj->args[i]);
    }
}
