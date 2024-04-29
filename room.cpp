#include "unp.h"
#include "msg.h"
#include "mypthread.h"
#include "net.h"
#include <unordered_map>
#define SENDTHREADSIZE 5
#define MB 1024*1024
SENDQUEUE sendQueue;
STATUS volatile roomstatus=ON;
static volatile int maxfd;
class Pool{
public:
    Pool(){
        lock=PTHREAD_MUTEX_INITIALIZER;
        onwer=0;
        num=0;
        FD_ZERO(&fdset);
        memset(status,0,sizeof(status));
    }

    void clear_pool(){
         pthread_mutex_lock(&lock);
         
         
         for(int i=0;i<=maxfd;i++){

             if(status[i]==ON){
                Close(i);
             }

         }
         roomstatus=CLOSE;
         onwer=0;
         num=0;
         memset(status,0,sizeof(status));
         FD_ZERO(&fdset);
         fd2ip.clear();
         //发送队列置空
         sendQueue.clear();
         pthread_mutex_unlock(&lock);
    
    }

public:
  int onwer;
  int num;
  pthread_mutex_t lock;
  int status[1024+10];
  fd_set fdset;
  std::unordered_map<int,uint32_t> fd2ip;

};

Pool *user_pool=new Pool();

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
            printf("c=%c\n",c);
            err_quit("no fd from read_fd");
        }
        if(c=='C'){
            //创建会议
            pthread_mutex_lock(&user_pool->lock);
            user_pool->num++;
            user_pool->onwer=tfd;
            user_pool->status[tfd]=1;
            user_pool->fd2ip[tfd]=getpeerip(tfd);
            FD_SET(tfd,&user_pool->fdset);
            roomstatus=ON;
            maxfd=maxfd>tfd?maxfd:tfd;
            pthread_mutex_unlock(&user_pool->lock);

            MSG msg;
            msg.msgtype=CREATE_MEETING_RESPONSE;
            msg.targetfd=tfd;
            int ROOMNO=htonl(getpid());
            msg.ptr=(char *)malloc(sizeof(int));
            memcpy(msg.ptr,&ROOMNO,sizeof(int));
            msg.len=sizeof(int);
            sendQueue.push_msg(msg);

        }else if (c=='J')
        {
            //加入会议
            pthread_mutex_lock(&user_pool->lock);
            if(roomstatus==CLOSE){
                Close(tfd);
                pthread_mutex_unlock(&user_pool->lock);
                continue;
            }
            else {

                user_pool->num++;
                user_pool->status[tfd]=1;
                user_pool->fd2ip[tfd]=getpeerip(tfd);
                FD_SET(tfd,&user_pool->fdset);
                maxfd=maxfd>tfd?maxfd:tfd;
                pthread_mutex_unlock(&user_pool->lock);

                MSG msg;
                memset(&msg,0,sizeof(MSG));
                msg.msgtype=PARTNER_JOIN;
                msg.ip=user_pool->fd2ip[tfd];
                msg.ptr=nullptr;
                msg.len=0;
                msg.targetfd=tfd;
                sendQueue.push_msg(msg);


                MSG msg1;
                memset(&msg1,0,sizeof(MSG));
                msg1.msgtype=PARTNER_JOIN2;
                msg1.targetfd=tfd;
                int size=user_pool->num*sizeof(uint32_t);
                msg1.ptr=(char *)malloc(size);
                
                int pos=0;
                for(int i=0;i<=maxfd;i++){

                   if(user_pool->status[i]==ON&&i!=tfd){
                    memcpy(msg1.ptr+pos,&user_pool->fd2ip[i],sizeof(uint32_t));
                    pos+=sizeof(uint32_t);
                    msg1.len+=sizeof(uint32_t);
                   }

                }
                sendQueue.push_msg(msg1);
                printf("join meeting: %d\n", msg.ip);

            }

        }
    }
    
    return nullptr;

}

void *sendfunc(void *arg){
    Pthread_detach(pthread_self());
    char *sendbuf=(char *)malloc(4*MB);

    for(;;){
      memset(sendbuf,0,sizeof(sendbuf));
      MSG msg=sendQueue.pop_msg();
      int len=0;


      sendbuf[len++]='$';

      short type=htons(msg.msgtype);
      memcpy(sendbuf+len,&type,sizeof(type));
      len+=2;
 



    }

}