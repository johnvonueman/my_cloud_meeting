#include "unp.h"
#include "msg.h"
#include "netheader.h"
#include "mypthread.h"
extern socklen_t addrlen;
extern Room *room;
extern int nprocesses;
extern int listenfd;
pthread_mutex_t mlock=PTHREAD_MUTEX_INITIALIZER;
void *thread_main(void *arg){
    //printf("here\n");
    void dowithuser(int connfd);
    
    int i=*(int *)arg;
    free(arg);
    int connfd;
    pthread_detach(pthread_self());
    
    printf("thread %d starting\n", i);

    struct sockaddr_in *cliaddr=(sockaddr_in *)Calloc(1,addrlen);
    socklen_t clilen;
    char buf[MAXSOCKADDR];
    

    for(;;){

        clilen=addrlen;

        pthread_mutex_lock(&mlock);
        connfd=Accept(listenfd,(sockaddr*)cliaddr,&clilen);
        pthread_mutex_unlock(&mlock);

        printf("connect from %s\n",Sock_ntop(buf,MAXSOCKADDR,(sockaddr*)cliaddr,clilen));
         
        dowithuser(connfd);

    }
    return nullptr;
}

void dowithuser(int connfd){
     void writetofd(int fd,MSG msg);
     int ret;
     char head[15]={0};
     for(;;){
       
       //printf("%d",connfd);
       ret=Readn(connfd,head,11);
       //printf("%d",ret);
       
    //    for(int k=0;k<11;k++){
    //      printf("%c",head[1]);
    //    }
       if(ret<=0){
        printf("%d close\n", connfd);
        Close(connfd);
        return ;
       }
       else if(ret<11){
        err_msg("len too short");
       }
       else if(head[0]!='$'){
        err_msg("data format error\n");
       }
       else{
 
        MSG_TYPE msgtype;
        memcpy(&msgtype,head+1,2);
        msgtype=(MSG_TYPE)ntohs(msgtype);


        uint32_t ip;
        memcpy(&ip,head+3,4);
        ip=(uint32_t)ntohl(ip);


        int datasize;
        memcpy(&datasize,head+7,4);
        datasize=(int)ntohl(datasize);
        
        
       
        if(msgtype==CREATE_MEETING){
          char tail;
          Readn(connfd,&tail,1);
          if(tail=='#'&&datasize==0){
             char *c=(char *)&ip;
             printf("ip %d %d %d %d creat meeting\n",c[3],c[2],c[1],c[0]);
             if(room->nval<=0){
                MSG msg;
                memset(&msg,0,sizeof(msg));
                msg.msgtype=CREATE_MEETING_RESPONSE;

                int roomno=0;
                msg.ptr=(char *)malloc(sizeof(int));
                memcpy(msg.ptr,&roomno,sizeof(int));

                msg.len=sizeof(int);

                writetofd(connfd,msg);

             }
             else{
                int i;
                pthread_mutex_lock(&room->lock);
                
                for(i=0;i<nprocesses;i++){
                    if(room->pptr[i].child_status==0) break;
                }

                if(i==nprocesses){
                    MSG msg;
                    memset(&msg,0,sizeof(msg));
                    msg.msgtype=CREATE_MEETING_RESPONSE;
                    int roomno=0;
                    msg.ptr=(char *)malloc(sizeof(int));
                    memcpy(msg.ptr,&roomno,sizeof(int));
                    msg.len=sizeof(int);
                    writetofd(connfd,msg);
                }
                
                else{


                    char cmd='C';
                    //printf("%d",room->pptr[i].child_pipefd);
                    if(write_fd(room->pptr[i].child_pipefd,&cmd,1,connfd)<0){

                        err_msg("write_fd error\n");
                    }
                    else{
                        
                        Close(connfd);
                       // printf("aaaaaaaaaaaa\n");
                        printf("room %d is empty\n",room->pptr[i].child_id);
                        
                        room->nval--;
                        room->pptr[i].total++;
                        room->pptr[i].child_status=1;
                        pthread_mutex_unlock(&room->lock);
                        return ;
                    }
 

                }

                pthread_mutex_unlock(&room->lock);
             }
          }
          else{
            err_msg("1 data format error\n");
          }


        }
        else if(msgtype==JOIN_MEETING){
          uint32_t msgsize,roomno;
          memcpy(&msgsize,head+7,4);
          msgsize=ntohl(msgsize);
    
          int r=Readn(connfd,head,msgsize+1);
          if(r<msgsize+1){
            err_msg("data too short\n");
          }

          else{
                
                if(head[msgsize]=='#'){
                  
                  memcpy(&roomno,head,msgsize);
                  roomno=ntohl(roomno);


                  bool ok=false;
                  int i;
                  for(i=0;i<nprocesses;i++){

                    if(room->pptr[i].child_status==1&&room->pptr[i].child_id==roomno){
                           
                           ok=true;
                           break;

                    }
                  }

                  MSG msg;
                  memset(&msg,0,sizeof(msg));
                  msg.msgtype=JOIN_MEETING_RESPONSE;
                  msg.len=sizeof(uint32_t);

                  if(ok){
                      if(room->pptr[i].total>=1024){
                          msg.ptr=(char *)malloc(sizeof(uint32_t));
                          uint32_t full=-1;
                          memcpy(msg.ptr,&full,sizeof(uint32_t));
                          writetofd(connfd,msg);
                      }
                      else{

                          pthread_mutex_lock(&room->lock);
                          char cmd='J';

                          if(write_fd(room->pptr[i].child_pipefd,&cmd,1,connfd)<0){
                            err_msg("write_fd err");
                          }

                          else{

                              msg.ptr=(char *)malloc(sizeof(uint32_t));
                              memcpy(msg.ptr,&roomno,sizeof(uint32_t));
                              writetofd(connfd,msg);
                              room->pptr[i].total++;
                              pthread_mutex_unlock(&room->lock);
                              Close(connfd);
                              return ;


                          }

                          pthread_mutex_unlock(&room->lock);


                      }


                  }
                  else{
                      msg.ptr = (char *)malloc(msg.len);
                      uint32_t fail = 0;
                      memcpy(msg.ptr, &fail, sizeof(uint32_t));
                      writetofd(connfd, msg);

                  }
                  
                  


                }
                else{
                    printf("format error\n");
                    
                }





          }

 

        }
        else{
            printf("data format error\n");
        }
       }
     }

}

void writetofd(int fd,MSG msg){
    char *buf=(char *)malloc(100);
    memset(buf,0,100);
    int bytestowrite=0;
    buf[bytestowrite++]='$';

    MSG_TYPE msgtype=(MSG_TYPE)htons(msg.msgtype);
    memcpy(buf+bytestowrite,&msgtype,sizeof(uint16_t));
    bytestowrite+=2;

    bytestowrite+=4;//skip ip

    uint32_t len=htonl(msg.len);
    memcpy(buf+bytestowrite,&len,sizeof(uint32_t));
    bytestowrite+=4;


    memcpy(buf+bytestowrite,msg.ptr,msg.len);
    bytestowrite+=msg.len;


    buf[bytestowrite++]='#';
    
   // printf("%s\n",buf);

    if(writen(fd,buf,bytestowrite)<bytestowrite){
        printf("write fail\n");
    }

    if(msg.ptr){
        free(msg.ptr);
        msg.ptr=nullptr;
    }

    free(buf);
    
}