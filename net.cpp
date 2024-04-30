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

void Socketpair(int family, int type, int protocol, int * sockfd)
{
    if(socketpair(family, type, protocol, sockfd) < 0)
    {
        err_quit("socketpair error");
    }
}

//read_fd(fd,&c,1,&tfd)
ssize_t read_fd(int fd,void *ptr,size_t nbytes,int *recvfd){
   
   struct msghdr msg;
   struct iovec iov[1];
   ssize_t n;

   union{

     struct cmsghdr cm;
     char contorl[CMSG_SPACE(sizeof(int))];

   }control_un;

   struct cmsghdr *cmptr;
   msg.msg_control=control_un.contorl;
   msg.msg_controllen=sizeof(control_un.contorl);

   msg.msg_name=nullptr;
   msg.msg_namelen=0;
   
   iov[0].iov_base=ptr;
   iov[0].iov_len=nbytes;

   msg.msg_iov=iov;
   msg.msg_iovlen=1;

   if((n==recvmsg(fd,&msg, MSG_WAITALL))<0){//没有接收到 msghdr 中指定的 msg_iovlen 和 iov_len 字段所描述的总字节数，recvmsg 将不会返回
      return n;
   }
   //CMSG_FIRSTHDR用来获取msg中第一个控制消息的指针
   //cmsg_level字段表示控制消息所属的协议级别。在这里，它应该等于SOL_SOCKET，这表示消息是由套接字层生成的。
   //cmsg_type字段表示控制消息的具体类型。SCM_RIGHTS是一种特殊的控制消息类型，用于在进程间传递文件描述符。
   if((cmptr=CMSG_FIRSTHDR(&msg))!=nullptr&&(cmptr->cmsg_len==CMSG_LEN(sizeof(int)))){
    if(cmptr->cmsg_level!=SOL_SOCKET){
      err_msg("control level!=SOL_SOCKET");
    }
    if(cmptr->cmsg_type!=SCM_RIGHTS){
      err_msg("control type!=SCM_RIGHTS");
    }
    *recvfd= *((int *)CMSG_DATA(cmptr));
   }
   else{
    *recvfd=-1;
   }
   return n;
} 

ssize_t write_fd(int fd, void *ptr, size_t nbytes, int sendfd){
  struct msghdr msg;
  struct iovec iov[1];
  
  union{
    struct cmsghdr cm;
    char control[CMSG_SPACE(sizeof(int))];
  }control_un;

  struct cmsghdr *cmptr;

  msg.msg_control=control_un.control;
  msg.msg_controllen=sizeof(control_un.control);

  cmptr=CMSG_FIRSTHDR(&msg);
  cmptr->cmsg_len=CMSG_LEN(sizeof(int));
  cmptr->cmsg_level=SOL_SOCKET;
  cmptr->cmsg_type=SCM_RIGHTS;

  *((int *)CMSG_DATA(cmptr))=sendfd;


  msg.msg_name=nullptr;
  msg.msg_namelen=0;

  iov[0].iov_base=ptr;
  iov[0].iov_len=nbytes;


  msg.msg_iov=iov;
  msg.msg_iovlen=1;

  return (sendmsg(fd,&msg,0));
}

uint32_t getpeerip(int fd){

  struct sockaddr_in peeraddr;
  socklen_t len;
  if(getpeername(fd,(sockaddr *)&peeraddr,&len)<0){
    err_msg("getpeername error");
  }
  
  return peeraddr.sin_addr.s_addr;


}

ssize_t Readn(int fd,void *buf,size_t size){

     ssize_t lefttoread=size,hasread=0;
     char *ptr=(char *)buf;
     while(lefttoread>0){

      if((hasread=read(fd,ptr,size))<0){
        
         if(errno == EINTR)//errno是一个全局变量，代表的是系统调用的错误类型，EINTR代表中断
             {
                hasread = 0;
             }
         else{
          return -1;
         }
      }
      else if(hasread==0){
        break;//读完
      }
      lefttoread=size-hasread;
      ptr+=hasread;

     }

     return size-lefttoread;

}

ssize_t writen(int fd,const void *buf,size_t size){

    ssize_t lefttowrite=size,haswrite=0;
    char *ptr=(char *)buf;
    while(lefttowrite>0){
       if((haswrite=write(fd,ptr,size))<0){
         if(errno==EINTR){
          return 0;
         }
         else{
          return -1;
         }
       }
       else if(haswrite==0){
        break;
       }
       lefttowrite-=haswrite;
       ptr+=haswrite;

    }
    return size-lefttowrite;//不一样

}

int Accept(int listenfd, SA * addr, socklen_t *addrlen){

    int n;

    for(;;){

       if((n=accept(listenfd,addr,addrlen)<0)){
        if(errno==EINTR){
          continue;
        }
        else{
          err_msg("accept error");
        }
       }
       else{
        return n;
       }


    }
}