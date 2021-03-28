#include <cutils/sockets.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "socket_local.h"
#define LISTEN_BACKLOG 4
int socket_local_server_bind(int s,const char *name,int namespaceId){
	struct sockaddr_un addr;
	socklen_t alen;
	int n,err;
	if((err=socket_make_sockaddr_un(name,namespaceId,&addr,&alen))<0)return -1;
	unlink(addr.sun_path);
	n=1;
	setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&n,sizeof(n));
	if(bind(s,(struct sockaddr*)&addr,alen)<0)return -1;
	return s;
}
int socket_local_server(const char*name,int namespace,int type){
	int err,s;
	if((s=socket(AF_LOCAL,type,0))<0)return -1;
	if((err=socket_local_server_bind(s,name,namespace))<0){
		close(s);
		return -1;
	}
	if(type!=SOCK_STREAM)return s;
	int ret;
	if((ret=listen(s,LISTEN_BACKLOG))<0){
		close(s);
		return -1;
	}
	return s;
}
