#include<sys/socket.h>
#include "unp.h"
int main(){
   int cfd=socket(AF_INET,SOCK_STREAM,0);
   sockaddr_in  saddr;
   saddr.sin_family=AF_INET;
   saddr.sin_port=htons(8888);
   saddr.sin_addr.s_addr=htonl(INADDR_ANY);
   connect(cfd,(sockaddr *)&saddr,sizeof(saddr));

}