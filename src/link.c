#include <link.h>

List* addNodeTail(List* list, void* ptr) {
    struct Node* node = malloc(sizeof(Node));
    node->ptr = ptr;
    if(list->head == NULL) {
        list->tail = list->head = node;
        node->prev = NULL;
        node->next = NULL;
    } else {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
        node->next = NULL;
    }
    list->count++;
    return list;
}

List* addNodeHead(List* list, void* ptr) {
    struct Node* node = malloc(sizeof(Node));
    node->ptr = ptr;
    if(list->head == NULL) {
        list->tail = list->head = node;
        node->prev = NULL;
        node->next = NULL;
    } else {
        node->prev = NULL;
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->count++;
    return list;
}

Node* searchByKey(List* list, void* ptr) { 
    Node* node = list->head;
    while(node != NULL) {
        if(list->cmp(ptr, node->ptr)){
            return node;
        }
        node = node->next;
    }
    return NULL;
}

bool cmpPubSub(void* src, void* dst) {
    return src == dst;
}

void deleteNode(List* list, Node* node) {
    if(node->next != NULL)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;
    if(node->prev != NULL)
        node->prev->next = node->next;
    else
        list->head = node->next;
    if(list->dynamic)
        free(node->ptr);
    free(node);
    list->count--;
}

void deallocate(List* list) {
    Node* start, *temp; 
    start = list->head;
    while(start != NULL) {
        temp = start->next; 
        if(list->dynamic) free(start->ptr);
        free(start);
        start = temp;
    }
    list->head = list->tail = 0;
    list->count = 0;
}

void totaldealloc(List* list) {
    deallocate(list);
    free(list);
}

ListIter* listIterNewNode(Node* node, listDirection direction) {
    ListIter* iterator = malloc(sizeof(ListIter));
    iterator->next = node;
    iterator->direction = direction;
    return iterator;
}

ListIter* listIterator(List* list, listDirection direction) {
    Node* node = (direction == HEAD) ? list->head : list->tail;
    return listIterNewNode(node, direction);
}

Node* listIterNext(ListIter* iter) {
    Node* curr = iter->next;
    if(curr)
        iter->next = (iter->direction == HEAD) ? curr->next : curr->prev;
    return curr;
}

List* createList(bool dyn, cmpFn cmp) {
    struct List* list = malloc(sizeof(List));
    list->cmp = (cmp) ? cmp : cmpPubSub;
    list->head = list->tail = NULL;
    list->count = 0;
    list->dynamic = dyn;
    return list;
}
