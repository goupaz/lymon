#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
typedef enum listDirection {
    HEAD,
    TAIL
} listDirection;

typedef bool (*cmpFn)(void* src, void* dst);

typedef struct List {
    struct Node* tail;
    struct Node* head;
    size_t       count;
    cmpFn        cmp;
    bool         dynamic;
} List;

typedef struct Node {
    void*         ptr;
    struct Node*  next;
    struct Node * prev;
} Node;

typedef struct ListIter {
    Node* next;
    listDirection direction;
} ListIter;

#define list_count(list) ((list)->count)
#define list_head(list) ((list)->head)
#define list_tail(list) ((list)->tail)
#define node_next(node) ((node)->next)
#define node_prev(node) ((node)->prev)
void init_ld();
List* createList(bool dyn, cmpFn cmp);
void totaldealloc(List* list);
List* addNodeHead(List* list, void* ptr); 
List* addNodeTail(List* list, void* ptr);
Node* searchByKey(List* list, void* ptr);
void deleteNode(List* list, Node* node);
Node* listIterNext(ListIter* iter);
ListIter* listIterator(List* list, listDirection direction);
