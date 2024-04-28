#include "unp.h"
#include "mypthread.h"


void Pthread_create(pthread_t *tid,const pthread_attr_t *attr,void *(*func) (void *) ,void *arg){

   if(pthread_create(tid,attr,func,arg)!=0){
     err_msg("pthread create error\n");
   }

}
void Pthread_detach(pthread_t tid){

   if(pthread_detach(tid)!=0){

      err_msg("detach error");
   }

}