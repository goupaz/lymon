#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <server.h>
#define BACKLOG 512 
void create_epoll(int sockfd);
int set_non_block(int sockfd);
