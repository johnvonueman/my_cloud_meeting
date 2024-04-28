#include "unp.h"
#include "mypthread.h"
#define SENDTHREADSIZE 5
void process_main(int i,int fd){

    void *accept_fd(void *);
    void *sendfunc(void *);
    

    pthread_t pfd1;
    int *ptr=(int *)malloc(4);
    *ptr=fd;
    Pthread_create(&pfd1,nullptr,accept_fd,ptr);

    for(int i=0;i<SENDTHREADSIZE;i++){
      Pthread_create(&pfd1,nullptr,sendfunc,nullptr);
    }



}


void *accept_fd(void * arg){

    Pthread_detach(pthread_self());//线程分离
    int fd= *(int *)arg;
    int tfd=-1;
    free(arg);

    while(1){
        int n,c;
        if((n=read_fd(fd,&c,1,&tfd))<=0){
            err_quit("read_fd error");
        }
        if(tfd<0){
            err_quit("no fd from read_fd");
        }
        if(c=='C'){
            //创建会议
        }else if (c=='J')
        {
            //加入会议
        }
    }
    
    return nullptr;

}