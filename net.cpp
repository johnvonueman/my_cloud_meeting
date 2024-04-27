

#include "unp.h"
#define MAXSOCKADDR 128
static char errorMsg[] = "server error"; 
void Setsockopt(int fd, int level, int optname, const void * optval, socklen_t optlen)
{
    if(setsockopt(fd, level, optname, optval, optlen) < 0)
    {
        err_msg("setsockopt error");
    }
}

char *Sock_ntop(char *str,int size,const sockaddr *sa,socklen_t salen){

    switch(sa->sa_family){

      case AF_INET:
      {
        sockaddr_in *sin=(sockaddr_in *)sa;
        if(inet_ntop(AF_INET,&sin->sin_addr,str,size)==nullptr){
            err_msg("inet_ntop error");
        }
        if(sin->sin_port>0){

            snprintf(str+strlen(str),size-strlen(str),":%d",sin->sin_port);
        }

         return str;

      }

      case AF_INET6:
      {
        sockaddr_in6 *sin6=(sockaddr_in6 *)sa;
        if(inet_ntop(AF_INET,&sin6->sin6_addr,str,size)==nullptr){
            err_msg("inet_ntop error");
        }
        if(sin6->sin6_port>0){

            snprintf(str+strlen(str),size-strlen(str),":%d",sin6->sin6_port);
        }

         return str;

      }
      default:
          
          return errorMsg;

    }
      return NULL;

}

void Close(int fd){

    if(close(fd)<0){
        err_msg("close error");
    }

}

void Listen(int fd,int backlog){
   if(listen(fd,backlog)<0){
    err_quit("listen error");
   }
}


int Tcp_listen(const char * host,const char * service,socklen_t *addrlen){

   int listenfd,n;
   int on=1;
   struct addrinfo hints,*res,*ressave;
   bzero(&hints,sizeof(struct addrinfo));
   hints.ai_flags=AI_PASSIVE;
   hints.ai_family=AF_UNSPEC;
   hints.ai_socktype=SOCK_STREAM;
  
   char addr[MAXSOCKADDR];


   if((n=getaddrinfo(host,service,&hints,&res))>0){

     err_quit("tcp listen error for %s %s:%s",host,service,gai_strerror(n));
   }

   ressave=res;

   do{
      listenfd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
      if(listenfd<0){
        continue;
      }
      Setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
      
      if(bind(listenfd,res->ai_addr,res->ai_addrlen)==0){
        printf("server address:%s\n",Sock_ntop(addr,MAXSOCKADDR,res->ai_addr,res->ai_addrlen));
        break;//绑定成功
      }
      Close(listenfd);

   }while((res=res->ai_next)!=nullptr);
   freeaddrinfo(ressave);

   if(res==nullptr){
    err_quit("tcp listen error for %s:%s",host,service);

   }

   //设置监听上限
   Listen(listenfd,LISTENQ);

   if(addrlen){
    *addrlen=res->ai_addrlen;
   }

   return listenfd;

}