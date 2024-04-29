#ifndef MSG_H
#define MSG_H
#include "netheader.h"
#include <queue>
#define MAXSIZE 10000
enum STATUS{
 CLOSE=0,
 ON=1,
} ;

struct MSG{

    char *ptr;
    int len;
    int targetfd;
    uint32_t ip;
    MSG_TYPE msgtype;  //消息类型
    Image_Format imgFomt;//图片格式
    
    MSG(){

    }
    MSG(MSG_TYPE msg_type,char *msg,int length,int fd){
        msgtype=msg_type;
        ptr=msg;
        len=length;
        targetfd=fd;
    }
    

};

struct SENDQUEUE{
private:
    pthread_mutex_t lock;
    pthread_cond_t cond;
    std::queue<MSG> send_queue;

public:
    SENDQUEUE(){
        lock=PTHREAD_MUTEX_INITIALIZER;
        cond=PTHREAD_COND_INITIALIZER;
    }

    void push_msg(MSG msg){
     pthread_mutex_lock(&lock);
     while(send_queue.size()>=MAXSIZE){
        pthread_cond_wait(&cond,&lock);//等待cond满足之后，同时释放lock
     }
     send_queue.push(msg);
     pthread_mutex_unlock(&lock);
     pthread_cond_signal(&cond);
      
    }

    MSG pop_msg(){
     pthread_mutex_lock(&lock);
     
     while(send_queue.empty()){
        pthread_cond_wait(&cond,&lock);
     }
     MSG msg=send_queue.front();
     send_queue.pop();
     pthread_mutex_unlock(&lock);
     pthread_cond_signal(&cond);
     return msg;

    }

    void clear(){

       while(!send_queue.empty()){
        send_queue.pop();
       }

    }

};

#endif