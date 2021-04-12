#include<cutils/sockets.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<stddef.h>
#define LISTEN_BACKLOG 4
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
int socket_loopback_server(int port,int type){
	struct sockaddr_in addr;
	int s,n;
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
	if((s=socket(AF_INET,type,0))<0)return -1;
	n=1;
	setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&n,sizeof(n));
	if(bind(s,(struct sockaddr*) &addr,sizeof(addr)) < 0) {
		close(s);
		return -1;
	}
	if(type!=SOCK_STREAM)return s;
	if(listen(s,LISTEN_BACKLOG)<0){
		close(s);
		return -1;
	}
	return s;
}

