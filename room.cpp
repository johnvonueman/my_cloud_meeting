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

void fd_close(int fd,int pipefd){
    
     
    if(user_pool->onwer==fd){
       //关闭 room
       user_pool->clear_pool();
       //写 E给父进程
       printf("room close\n");

       if(writen(pipefd,"E",1)<0){
        err_msg("write error");
       }
       else{

        //从room里删除

        uint32_t ip;
        pthread_mutex_lock(&user_pool->lock);
 
        ip=user_pool->fd2ip[fd];
        user_pool->num--;
        FD_CLR(fd,&user_pool->fdset);
        user_pool->status[fd]=CLOSE;
        if(fd==maxfd) maxfd--;
        pthread_mutex_unlock(&user_pool->lock);
        

        char cmd='Q';
        if(writen(pipefd,&cmd,1)<1){
            err_msg("writen error");
        }

        MSG msg;
        memset(&msg,0,sizeof(msg));
        msg.ip=ip;
        msg.msgtype=EXIT_MEETING;
        msg.targetfd=-1;

        Close(fd);
        sendQueue.push_msg(msg);
       }
    }
}



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

    for(;;){
       fd_set rset=user_pool->fdset;
       int nsel;
       struct timeval time;
       memset(&time,0,sizeof(time));
       while((nsel=select(maxfd+1,&rset,nullptr,nullptr,&time))==0){
             rset=user_pool->fdset;
       }
       for(int i=0;i<=maxfd;i++){
        if(FD_ISSET(i,&rset)){
           char head[15]={0};
           int ret=Readn(i,head,11);
           if(ret<=0){
            printf("peer fd closed");
            //关闭i，fd
            fd_close(i,fd);
           }
           else if(ret==11){
            if(head[0]=='$'){
                //获取消息种类
                MSG_TYPE msgtype;
                memcpy(&msgtype,head+1,2);
                msgtype=(MSG_TYPE)ntohs(msgtype);


                MSG msg;
                memset(&msg,0,sizeof(msg));
                
                msg.targetfd=i;
                memcpy(&msg.ip,head+3,4);
                
                int msglen;
                memcpy(&msglen,head+7,4);
                msg.len=ntohl(msglen);

                if(msgtype==IMG_SEND||msgtype==AUDIO_SEND||msgtype==TEXT_SEND){

                  msg.msgtype=IMG_SEND?IMG_RECV:(AUDIO_SEND?AUDIO_RECV:TEXT_RECV);
                  msg.ptr=(char *)malloc(msg.len);
                  msg.ip=user_pool->fd2ip[i];

                  
                  if((ret=Readn(i,msg.ptr,msg.len))<msg.len){
                    err_msg("3 msg format error");
                  }
                  else{

                    char tail;
                    Readn(i,&tail,1);

                    if(tail!='#'){
                        err_msg("4 msg format error");
                    }
                    else{
                        sendQueue.push_msg(msg);
                    }
 

                  }

                }
                if(msgtype==CLOSE_CAMERA){
                    char tail;
                    Readn(i,&tail,1);

                    if(tail=='#'&&msg.len==0){
                        msg.msgtype=CLOSE_CAMERA;
                        sendQueue.push_msg(msg);
                    }
                    else{
                        err_msg("camera data error");
                    }
                }

                
            }
            else{
                err_msg("head error");
            }
           }
           else{
            err_msg("2 msg error");
           }
           if(--nsel<=0){
            break;
           }

        }
       }
 


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


      if(msg.msgtype==CREATE_MEETING_RESPONSE||msg.msgtype==PARTNER_JOIN2){
        len+=4;
      }
      else if(msg.msgtype==TEXT_RECV||msg.msgtype==PARTNER_EXIT||msg.msgtype==PARTNER_JOIN||msg.msgtype==IMG_RECV||msg.msgtype==AUDIO_RECV||msg.msgtype==CLOSE_CAMERA){
        memcpy(sendbuf+len,&msg.ip,sizeof(msg.ip));
        len+=4;
      }

      int datasize=htonl(msg.len);
      memcpy(sendbuf+len,&datasize,sizeof(int));
      len+=4;

      memcpy(sendbuf+len,msg.ptr,msg.len);
      len+=msg.len;

      sendbuf[len++]='#';


      pthread_mutex_lock(&user_pool->lock);
      
      if(msg.msgtype==CREATE_MEETING_RESPONSE){
        if(write(msg.targetfd,sendbuf,sizeof(sendbuf))<0){
            err_msg("write error");
        }
      }
      else if(msg.msgtype==AUDIO_RECV||msg.msgtype==IMG_RECV||msg.msgtype==AUDIO_RECV||msg.msgtype==TEXT_RECV||msg.msgtype==CLOSE_CAMERA||msg.msgtype==PARTNER_JOIN){

        for(int i=0;i<=maxfd;i++){
            if(user_pool->status[i]==ON&&msg.targetfd!=i){

               if(write(i,sendbuf,sizeof(sendbuf))<0){
 
                 err_msg("write error");

               }

            }
        }

      }
      else if (msg.msgtype==PARTNER_JOIN2)
      {
        if(user_pool->status[msg.targetfd]==ON){
            if(write(msg.targetfd,sendbuf,sizeof(sendbuf))<0){
                err_msg("write error");
            }
        }
      }
      
      pthread_mutex_unlock(&user_pool->lock);

      if(msg.ptr){
        free(msg.ptr);
        msg.ptr=nullptr;
      }

      

    }
    free(sendbuf);
    return nullptr;
}