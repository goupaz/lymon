size_t DbObjHashFn(void *ptr);
bool DbObjHashCmp(void *hk, void* ck);
void DbObjHashDealloc(hash_entry* node);
void ExprObjHashDealloc(hash_entry* node);
void setCmd(clientObj* cobj);
void getCmd(clientObj* cobj);
void delCmd(clientObj* cobj);
