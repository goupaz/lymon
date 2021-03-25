#include <lym_epoll.h>
#include <pubsub.h>
#include <common.h>
#include <time.h>
#include <stdarg.h>
#include <socket.h>
#include <signal.h>
#include <unistd.h>
#define SIGINT_MSG "SIGINT received.\n"
struct serverObj srvObj;

static const commands const cmds[] = { 
    {.command = "publish", .fn = publish, .minargs = 3}, 
    {.command = "subscribe", .fn = subscribe, .minargs = -2},
    {.command = "unsubscribe", .fn = unsubscribe, .minargs = -2},
    {.command = "set", .fn = setCmd, .minargs = 3},
    {.command = "get", .fn = getCmd, .minargs = 2},
    {.command = "del", .fn = delCmd, .minargs = 2},
};

void sig_int_handler(int signum, siginfo_t *info, void *ptr) {
    write(STDERR_FILENO, SIGINT_MSG, sizeof(SIGINT_MSG));
}

void catch_sigterm() {
    static struct sigaction _sigact;
    memset(&_sigact, 0, sizeof(_sigact));
    _sigact.sa_sigaction = sig_int_handler;
    _sigact.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &_sigact, NULL);
}

int set_non_block(int sockfd) {
    int flags, s;
    flags = fcntl(sockfd, F_GETFL, 0);
    if(flags == -1) {
        fprintf(stderr, "get F_GETFL flag failed: %m");
        return -1;
    }
    flags |= O_NONBLOCK;
    s = fcntl(sockfd, F_SETFL, flags);
    if(s == -1) {
        fprintf(stderr, "set F_SETFL flag failed: %m");
        return -1;
    }
    return 0;
}

int skt() {
    int status;
    int sockfd, new_fd;
    int on = 1;
    int bres = -1;
    char pbuff[NI_MAXSERV];
    struct addrinfo server;
    struct addrinfo *serverinfo;
    struct addrinfo *p;
    socklen_t addr_size;
    bzero(&server, sizeof server);
    server.ai_family = AF_UNSPEC;
    server.ai_socktype = SOCK_STREAM;
    server.ai_flags = AI_PASSIVE;
    snprintf(pbuff, sizeof(pbuff), "%d", srvObj.port);
    status = getaddrinfo(NULL, pbuff, &server, &serverinfo);
    if(status != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status)); 
        exit(EXIT_FAILURE);
    }
    for (p = serverinfo; p != NULL; p = p->ai_next ) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sockfd == -1) continue;
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &on, sizeof(on)) == -1) {
            close(sockfd);
            fprintf(stderr, "set TCP_DEFER_ACCEPT failed: %m");
        }
        if(set_non_block(sockfd) == -1) {
            fprintf(stderr, "non blocking failed\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        if((bind(sockfd, p->ai_addr, p->ai_addrlen)) == 0) {
            if(listen(sockfd, BACKLOG) == -1){
                close(sockfd);
                fprintf(stderr, "listen failed \n");
                exit(EXIT_FAILURE);
            }
            break;
        } else {
            lympanic("bind failed %s\n", strerror(errno));
        }
    }
    freeaddrinfo(serverinfo);
    return sockfd;
}

static const char *LF = "\n";

void sendBuffer(int fd, char* msg) {
    send(fd, msg, strlen(msg), 0);
    send(fd, LF, strlen(LF), 0);
}

void sendBuffFmt(int fd, char* msg, char* buff) {
    char *message = malloc (strlen(msg) + strlen(buff) + 1);
    sprintf(message, msg, buff); 
    sendBuffer(fd, message);
    free(message);
}

void sendBuffError(int fd, char* msg) {
    sendBuffFmt(fd, "ERROR: %s ", msg);
}

void createClient(int fd) {
    hash_map* ph_pubsub = init_hash(((hash_map)
                {.capacity = 16, 
                .hash_fn  = pbObjHashFn, 
                .hash_cmp = pbObjHashCmp,
                .hash_dealloc = pbObjHashDealloc}));
    clientObj *cl = calloc(1, sizeof(clientObj));
    cl->fd = fd; // client fd
    cl->pubsub = ph_pubsub;
    cl->csize = 1024;
    cl->cbytes = 0;
    cl->cbuff = malloc(cl->csize);
    addNodeTail(srvObj.clients, cl);
}

void createServer(void) {
    srvObj.clients = createList(true, NULL);
    hash_map* ph_pubsub = init_hash(((hash_map)
                {.capacity = 16, 
                .hash_fn  = pbObjHashFn, 
                .hash_cmp = pbObjHashCmp,
                .hash_dealloc = pbObjHashDealloc}));
    hash_map* ph_db = init_hash(((hash_map)
                {.capacity = 16, 
                .hash_fn  = DbObjHashFn, 
                .hash_cmp = DbObjHashCmp,
                .hash_dealloc = DbObjHashDealloc}));
    hash_map* ph_expr = init_hash(((hash_map)
                {.capacity = 16, 
                .hash_fn  = DbObjHashFn, 
                .hash_cmp = DbObjHashCmp,
                .hash_dealloc = ExprObjHashDealloc}));
    srvObj.pubsub = ph_pubsub;
    srvObj.db = ph_db;
    srvObj.port = 1337;
    srvObj.backlog = 1024;
    srvObj.expires = ph_expr;
}

void clean(clientObj* cl) {
    printf("Cleaning resources for client %d\n", cl->fd);
    /* cleaning hash table and remove client from list */
    dealloc_hash(cl->pubsub);
    free(cl->pubsub->hash_table);
    free(cl->pubsub);
    free(cl->cbuff);
    Node* clnode = searchByKey(srvObj.clients, cl);
    deleteNode(srvObj.clients, clnode);
}

void lympanic( const char* format, ... ) {
    va_list args;
    fprintf( stderr, "Panic: " );
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    fprintf( stderr, "\n" );
    abort();
}

void dealloc_args(clientObj* cl) {
    /* cleaning arguments */
    if(cl->argc > 0) {
        for(int ii = 0; ii < cl->argc; ii++) 
            refcount_dec(cl->args[ii]);
        free(cl->args);
        cl->args = NULL;
        cl->argc = 0;
    }
}

clientObj* get_clObj(int fd) {
    Node* node = NULL;
    ListIter *it = listIterator(srvObj.clients, HEAD);
    clientObj* cl = NULL;
    while(node = listIterNext(it)) {
        cl = node->ptr;
        if(cl->fd == fd) break;
    }
    free(it);
    return cl;
}

const commands* lookupCommand(clientObj* cl, int argc) {
    char* buff = cl->args[0]->ptr;
    int fd = cl->fd;
    int ii;
    char *errmsg = NULL;
    size_t entry_cnt = (sizeof(cmds) / sizeof(struct commands));
    for(ii = 0; ii < entry_cnt; ii ++) {
        if(strcmp(buff, cmds[ii].command) != 0) {
            continue;
        }
        if(-cmds[ii].minargs > argc || cmds[ii].minargs > argc) {
            sendBuffer(fd, "Error: Number of cmd args doesn't match\n");
            return NULL;
        }
        return &cmds[ii];
    }
    sendBuffFmt(fd, "Error: %s command not found", buff);
    return NULL;
}

#define next() { cnt++; buff++; }

int calCnt(clientObj* clObj, char* buff) {
    int cnt = 0;
    bool quotes = false;
    if(*buff == '"') {
        quotes = true;
        next();
        while(quotes) {
            if(buff > clObj->cbuff + clObj->cbytes) break;
            if(*buff == '"' && quotes) quotes = false;
            next();
        }
    }else {
        while(!isspace(*buff) && *buff != '\xa') {
            next();
        }
    }
    return cnt;
}

int parseCmd(clientObj* clObj) {
    int nargs = 0, cnt = 0;
    int argcnt = 1;
    char *buff = clObj->cbuff;
    while(*buff != '\xa') {
        if(isspace(*buff)) { 
            buff++;
            continue;
        }
        if(isalnum(*buff) || *buff == '"') {
            if(clObj->args) {
                argcnt+=1;
                clObj->args = (sObj**)realloc(clObj->args, argcnt * sizeof(sObj*));
            } else {
                clObj->args = (sObj**)calloc(argcnt, sizeof(sObj*));
            }
            cnt = calCnt(clObj, buff);
            char *newObj = calloc(1, cnt + 1);
            memcpy(newObj, buff, cnt);
            buff = buff + cnt;
            clObj->args[nargs] = calloc(1, sizeof(sObj));
            clObj->args[nargs]->ptr = newObj;
            clObj->args[nargs]->refcnt.counter++;
            clObj->argc++;
            nargs++;
            continue;
        } else {
            fprintf(stderr, "Invalid symbol in command line\n");
            return 0;
        }
    }
    return nargs;
}

size_t getBuff(clientObj* clObj, int fd) {
    int cnt;
    for(;;) {
        if(clObj->cbytes >= clObj->csize) {
            clObj->csize *= 2;
            clObj->cbuff = realloc(clObj->cbuff, clObj->csize);
        }
        int lastcnt = clObj->csize - clObj->cbytes;
        cnt = read(fd, clObj->cbuff, lastcnt);
        if(cnt > 0) {
            clObj->cbytes += cnt;
            if(cnt == lastcnt)
                continue;
            else
                break;
        } else {
            break;
        }
    }
    return cnt;
}

uint64_t get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
}

void create_epoll(int sockfd) {
    uint64_t last_time = get_time();
    uint64_t elapsed;
    struct timeval tv;
    struct timespec ts;
    bool quit = false;
    int efd, curfds, n, nfds;
    struct epoll_event ev;
    struct epoll_event *events;
    efd = epoll_create(BACKLOG);
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        fprintf(stderr, "epoll_ctl error\n");
        return;
    }
    events = calloc(BACKLOG, sizeof ev);
    curfds = 1;
    for(;;) {
        nfds = epoll_wait(efd, events, BACKLOG, 1);
        if(nfds == -1) {
            fprintf(stderr, "epoll_wait error\n");
            free(events);
            break;
        }
        for(n = 0; n < nfds; n++) {
            if ((events[n].events & EPOLLERR)) {
                fprintf(stderr, "epoll error\n");
                exit(EXIT_FAILURE);
            } else if (events[n].data.fd == sockfd) {
                struct sockaddr_in client;
                int clientfd;
                socklen_t clilen = sizeof (struct sockaddr);
                clientfd = accept(sockfd, (struct sockaddr *)&client, &clilen);
                createClient(clientfd);
                if (clientfd == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    } else {
                        fprintf(stderr, "epoll accepted failed\n");
                        break;
                    }
                }
                set_non_block(sockfd);
                ev.data.fd = clientfd;
                ev.events = EPOLLIN;
                if(epoll_ctl(efd, EPOLL_CTL_ADD, clientfd, &ev) ==  -1){
                     fprintf(stderr, "epoll_ctrl failed\n");
                }
            } else {
                if(events[n].events & EPOLLIN) {
                    int cnt;
                    int fd = events[n].data.fd;
                    clientObj* cl = get_clObj(events[n].data.fd); 
                    cnt = getBuff(cl, events[n].data.fd);
                    struct epoll_event event = {0};
                    event.data.fd = fd;
                    event.events |= EPOLLIN | EPOLLOUT;
                    if(cnt == -1) {
                        if(errno != EAGAIN) fprintf(stderr, "read buffer error\n");
                        break;
                    } else if(cnt == 0) {
                        fprintf(stdout, "Connection closed %d\n", events[n].data.fd);
                        if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL) < 0) {
                            fprintf(stderr, "epoll_ctl failed: EPOLL_CTL_DEL");
                        }
                        clean(cl);
                        close(events[n].data.fd);
                        break;
                    } else if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event) < 0) {
                        fprintf(stderr, "epoll_ctl failed: EPOLL_CTL_MOD");
                    }
                    cl->cbuff[cnt] = '\0';
                    int argc = parseCmd(cl);
                    if(argc > 0) {
                        const commands* cmd = lookupCommand(cl, argc);
                        if(cmd)
                            cmd->fn(cl);
                        dealloc_args(cl);
                    }
                }
            }
        }
    }
    return;
}

void background() {
    pid_t pid;
    pid = fork();
    if(pid < 0)
        lympanic("fork failed: fork process\n");
    if(pid > 0)
        exit(EXIT_SUCCESS);
    if (setsid() < 0)
        exit(EXIT_FAILURE);
}

int main() {
    catch_sigterm();
    createServer(); // create server structure
    background();
    int sockfd = skt();
    create_epoll(sockfd);
}
