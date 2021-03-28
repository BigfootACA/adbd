#include<cutils/sockets.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<stddef.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<sys/select.h>
#include<sys/types.h>
#include "socket_local.h"
#define LISTEN_BACKLOG 4
int socket_make_sockaddr_un(const char*name,int namespaceId,struct sockaddr_un*p_addr,socklen_t*alen){
	memset(p_addr,0,sizeof(*p_addr));
	size_t namelen;
	switch(namespaceId){
		case ANDROID_SOCKET_NAMESPACE_ABSTRACT:
			namelen=strlen(name);
			if((namelen+1)>sizeof(p_addr->sun_path))goto error;
			p_addr->sun_path[0]=0;
			memcpy(p_addr->sun_path+1,name,namelen);
		break;
		case ANDROID_SOCKET_NAMESPACE_RESERVED:
			namelen=strlen(name)+strlen(ANDROID_RESERVED_SOCKET_PREFIX);
			if(namelen>sizeof(*p_addr)-offsetof(struct sockaddr_un,sun_path)-1)goto error;
			strcpy(p_addr->sun_path,ANDROID_RESERVED_SOCKET_PREFIX);
			strcat(p_addr->sun_path,name);
		break;
		case ANDROID_SOCKET_NAMESPACE_FILESYSTEM:
			namelen=strlen(name);
			if(namelen>sizeof(*p_addr)- offsetof(struct sockaddr_un,sun_path)-1)goto error;
			strcpy(p_addr->sun_path,name);
		break;
		default:return -1;
	}
	p_addr->sun_family=AF_LOCAL;
   *alen=namelen+offsetof(struct sockaddr_un,sun_path)+1;
	return 0;
error:
	return -1;
}
int socket_local_client_connect(int fd,const char*name,int namespaceId,int type){
	struct sockaddr_un addr;
	socklen_t alen;
	int err;
	if((err=socket_make_sockaddr_un(name,namespaceId,&addr,&alen))<0)goto error;
	if(connect(fd,(struct sockaddr*)&addr,alen)<0)goto error;
	return fd;
error:
	return -1;
}
int socket_local_client(const char*name,int namespaceId,int type){
	int s;
	if((s=socket(AF_LOCAL,type,0))<0)return -1;
	if(socket_local_client_connect(s,name,namespaceId,type)<0){
		close(s);
		return -1;
	}
	return s;
}
