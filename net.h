#include "unp.h"
#define MAXSOCKADDR 128

void Listen(int fd,int backlog);

int Tcp_listen(const char * host,const char * service,socklen_t *addrlen);

uint32_t getpeerip(int fd);

//size_t writen(int fd,void *buf,size_t size);