#include "unp.h"


struct Process{

    pid_t child_id;
    int child_pipefd;
    int child_status;
    int total;

};

struct Room{

   Process *pptr;
   int nval;
   pthread_mutex_t lock;

   Room(int n){

      nval=n;
      pptr=  (Process *)malloc(sizeof(Process)*n);
      lock=PTHREAD_MUTEX_INITIALIZER;
   };

};

struct Thread
{
   pthread_t thread_tid;
};

void Pthread_create(pthread_t *tid,const pthread_attr_t *attr,void *(*func) (void *) ,void *arg);

void Pthread_detach(pthread_t tid);