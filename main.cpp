#include <iostream>
#include <sys/socket.h>
#include "mypthread.h"
#include "unp.h"
using namespace std;
socklen_t addrlen;
int listenfd;
Room *room;

int make_process(int i,int listenfd){//并没有用到listenfd
void process_main(int, int);
int pipefd[2];
Socketpair(AF_LOCAL,SOCK_STREAM,0,pipefd);
pid_t pid;
if((pid=fork())>0){
   Close(pipefd[1]);
   room->pptr[i].child_id=pid; 
   room->pptr[i].child_pipefd=pipefd[0];//?这个对吗
   room->pptr[i].child_status=0;
   room->pptr[i].total=0;
   return pid;
}
   Close(listenfd);
   Close(pipefd[0]);
   process_main(i,pipefd[1]);
   return 0;
}






int main(int argc,char **argv){

   if(argc==4){
     //tcp连接，
     listenfd=Tcp_listen(nullptr,argv[1],&addrlen);
   }

   else if(argc==5){
    //tcp连接，提供ip，port
    
    listenfd=Tcp_listen(argv[1],argv[2],&addrlen);
    
   }
   else{

      err_quit("usage:./app [host] [port] [threads] [processes]");

   }

   int nthreads=atoi(argv[argc-2]);
   int nprocesses=atoi(argv[argc-1]);
   
   
   //创建房间
   room=new Room(nprocesses);
   
   Thread *tptr=(Thread*)calloc(nthreads,sizeof(Thread));
   
   fd_set res,master;
   FD_ZERO(&master);
   int maxfd=0;
   //进程池创建
   for(int i=0;i<nprocesses;i++){

      // 创建线程
      make_process(i,listenfd);
      FD_SET(room->pptr[i].child_pipefd,&master);//加入监听集合
      maxfd=max(maxfd,room->pptr[i].child_pipefd);
   
   }



   printf("创建成功，线程数%d,进程数%d",nthreads,nprocesses);




}