#include <iostream>
#include <sys/socket.h>

#include "unp.h"
using namespace std;
socklen_t addrlen;
int listenfd;
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

   int nthreads=atoi(argv[3]);
   int nprocesses=atoi(argv[4]);

   

}