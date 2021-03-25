#include <hashmap.h>
#include <link.h>
#ifdef CONCURRENT 
#else
#define LOCK_PREFIX ""
#endif
#define REFCOUNT_CHECK_LT_ZERO              \
        "js 111f\n\t"                       \
        "jmp 112f\n"                        \
        "111: call %P1\n\t"                 \
        "112:nop\n"

#define REFCOUNT_CHECK_LE_ZERO				\
	"jz 111f\n\t"					        \
	REFCOUNT_CHECK_LT_ZERO

typedef struct serverObj {
    List* clients;
    hash_map* pubsub;
    hash_map* db;
    hash_map* expires;
    int port;
    int backlog;
} serverObj;

typedef struct {
    int counter;
} refcount_t;

// TODO(talishboy): refcount_t will be atomic context

typedef struct sObj {
    refcount_t refcnt;
    void *ptr;
} sObj;

typedef struct clientObj {
    int fd;
    List* channels;
    hash_map* pubsub; 
    size_t csize; // client buffer size
    char* cbuff; // client buffer
    size_t cbytes; // current received bytes cnt
    sObj** args;
    int argc;
} clientObj;

typedef struct commands {
    char *command;
    void (*fn)(clientObj* cl);
    int minargs;
} commands;

static inline void overflow_panic() {
    fprintf(stderr, "Exception: Reference-counter overflow!\n");
    abort();
}

static inline void refcount_dec(sObj* sobj) {
    if(sobj->refcnt.counter == 1) {
        free(sobj->ptr);
        free(sobj);
    } else {	
        asm volatile("decl %0\n\t"
            REFCOUNT_CHECK_LE_ZERO
            :"+m" (sobj->refcnt.counter)
            : "i" (overflow_panic) : "cc", "cx");
    }
}

static inline void refcount_inc(sObj* sobj) {
	asm volatile("lock incl %0\n\t"
        REFCOUNT_CHECK_LT_ZERO
        :"+m" (sobj->refcnt.counter)
		: "i" (overflow_panic) : "cc", "cx");
}

void createClient(int clientfd);
extern struct serverObj srvObj;
void incrRefCnt(sObj* sobj);
void decRefCnt(sObj* sobj);
void sendBuffer(int fd, char* msg);
void sendBuffError(int fd, char* msg);
void lympanic( const char* format, ... );
