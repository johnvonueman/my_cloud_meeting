#include <iostream>
#include <sys/socket.h>
#include "mypthread.h"
#include "unp.h"
using namespace std;
socklen_t addrlen;
int listenfd;
Thread *tptr;
Room *room;
int nprocesses,navail;
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


void thread_make(int i){
    void *thread_main(void *);
    int *arg=(int *)calloc(1,sizeof(int));
    *arg=i;
    //创建线程thread_main()
    //pthread_create(&tptr[i].thread_tid,nullptr,thread_main,arg);
    //  printf("0\n");
    //printf("%d\n",i);
    //tptr[i].thread_tid=1;
    //printf("%ld\n",tptr[i].thread_tid=1);
    Pthread_create(&tptr[i].thread_tid, NULL, thread_main, arg);    
}



int main(int argc,char **argv){
   void sig_child(int signo);
   Signal(SIGCHLD, sig_child);
   fd_set rset,master;
   FD_ZERO(&master);

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
   nprocesses=atoi(argv[argc-1]);
   
   
   //创建房间
   room=new Room(nprocesses);
   
   tptr=(Thread*)calloc(nthreads,sizeof(Thread));
   
   FD_ZERO(&master);
   int maxfd=0;
   //进程池创建
   
   
   for(int i=0;i<nprocesses;i++){

      // 创建线程
      make_process(i,listenfd);
      FD_SET(room->pptr[i].child_pipefd,&master);//加入监听集合
      maxfd=max(maxfd,room->pptr[i].child_pipefd);
   
   }
   
   //创建线程池
   
   for(int i=0;i<nthreads;i++){
      
      thread_make(i);
      
   }

   
   printf("创建成功，线程数%d,进程数%d",nthreads,nprocesses);

    for(;;){
     
       rset=master;
       int nsel;
       if((nsel=select(maxfd+1,&rset,nullptr,nullptr,nullptr))==0){
         continue;
       }
       
       for(int i=0;i<nprocesses;i++){

         if(FD_ISSET(room->pptr[i].child_pipefd,&rset)){

           char ch;
           int n;
           if((n=Readn(room->pptr[i].child_pipefd,&ch,1))<=0){
            err_quit("child %d terminal unexpectly\n",i);
           }
           
           if(ch=='E'){
              pthread_mutex_lock(&room->lock);
              
              room->pptr[i].child_status=0;
              room->nval++;
              printf("room %d is now free\n",room->pptr[i].child_id);

              pthread_mutex_unlock(&room->lock);


           }
           else if(ch=='Q'){

              pthread_mutex_lock(&room->lock);
             
              room->pptr[i].total--;

              pthread_mutex_unlock(&room->lock);

           }
           else{

              err_msg("read from %d error\n",room->pptr[i].child_pipefd);
              continue;

           }
         }

       }
      if(--nsel==0) break;
    }

    return 0;
}