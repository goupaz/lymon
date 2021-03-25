#include <server.h>
#include <errno.h>
#include <sys/time.h>
#include <limits.h>
#define SEC_MS_UNIT(x) (x*1000)
#define MS_SEC_UNIT(x) ((x + 500) / 1000);

long long msTime() {
    struct timeval te; 
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

size_t DbObjHashFn(void *ptr) {
    const char* key = ptr;
    return murmurhash(key, strlen(key), 0);
}

bool DbObjHashCmp(void *hk, void* ck) {
   const char* ck_key = ck;
   const char* hk_key = hk;
   return (strcmp(ck_key, hk_key) == 0);
}

void DbObjHashDealloc(hash_entry* node) {
    if(node->key) free(node->key);
    if(getVal(node->val)) refcount_dec(getVal(node->val));
}

void ExprObjHashDealloc(hash_entry* node) {
    if(node->key) free(node->key);
    if(getVal(node->val)) free(getVal(node->val));
}

long long parseStrToInt(clientObj* cobj, char* str) {
    char* sEnd;
    long long result = strtol(str, &sEnd, 10);
    errno = 0;
    if((sEnd == str) || (result == LONG_MAX || result == LONG_MIN) && errno == ERANGE)
        sendBuffError(cobj->fd, "time value cannot be parsed");
        return -1;
    return result;
}

void setKey(clientObj* cobj) {
    char* key = strdup(cobj->args[1]->ptr);
    sObj* val = cobj->args[2];
    hash_entry* entry = hash_get(&((hash_entry){.key = key}), srvObj.db);
    refcount_inc(val);
    if(entry)
        setVal(entry->val, val);
    else
        hash_insert(&((hash_entry){.key = key, .val.v = val}), srvObj.db);
}

void setCmd(clientObj* cobj) {
    char *flag;
    long long inputTime = 0;
    bool expire = false;
    if(cobj->argc >= 5) {
        flag = cobj->args[3]->ptr;
        if((flag[0] == 'E' || flag[0] == 'e') &&
           (flag[1] == 'x' || flag[1] == 'X')) {
            inputTime = parseStrToInt(cobj, cobj->args[4]->ptr);
            if(inputTime == -1) return;
            expire = true;
        }
    }
    setKey(cobj);
    if(expire) {
        hash_entry* entry = hash_get(&((hash_entry){.key = cobj->args[1]->ptr}), srvObj.db);
        long long timeval = msTime() + SEC_MS_UNIT(inputTime);
        hash_insert(&((hash_entry){.key = entry->key, .val.uintv = timeval}), srvObj.expires); 
    }
}

void getCmd(clientObj* cobj) {
    char* key = cobj->args[1]->ptr;
    char* val;
    hash_entry* entry = hash_get(&((hash_entry){.key = key}), srvObj.db);
    if(entry) {
        val = ((sObj*)getVal(entry->val))->ptr;
        sendBuffer(cobj->fd, val); 
    }
}

void delCmd(clientObj* cobj) {
    char* key = cobj->args[1]->ptr;
    hash_entry* entry = hash_get(&((hash_entry){.key = key}), srvObj.db);
    if(entry) {
        hash_remove(srvObj.db, key);
    }
}


