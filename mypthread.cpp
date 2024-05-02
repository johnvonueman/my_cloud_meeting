#include "unp.h"
#include "mypthread.h"
static int i=0;

//void Pthread_create(pthread_t *tid,const pthread_attr_t *attr,void *(*func) (void *) ,void *arg){
void  Pthread_create(pthread_t * tid, const pthread_attr_t * attr,THREAD_FUNC * func, void *arg){
  int n;
  if( (n = pthread_create(tid, attr, func, arg)) != 0)
  {
      errno = n;
      err_quit("pthread create error");
  }
 
}
void Pthread_detach(pthread_t tid){

   if(pthread_detach(tid)!=0){

      err_msg("detach error");
   }

}