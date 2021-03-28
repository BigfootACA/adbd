#ifndef __CUTILS_SOCKETS_H
#define __CUTILS_SOCKETS_H
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#define ANDROID_SOCKET_ENV_PREFIX "SOCKET_"
#define ANDROID_SOCKET_DIR        "/run"
static inline int android_get_control_socket(const char*name){
	char key[64]=ANDROID_SOCKET_ENV_PREFIX;
	const char*val;
	int fd;
	strncpy(key+sizeof(ANDROID_SOCKET_ENV_PREFIX)-1,name,sizeof(key)-sizeof(ANDROID_SOCKET_ENV_PREFIX));
	key[sizeof(key)-1]='\0';
	if(!(val=getenv(key)))return -1;
	errno=0;
	fd=strtol(val,NULL,10);
	if(errno)return -1;
	return fd;
}
#define ANDROID_SOCKET_NAMESPACE_ABSTRACT 0
#define ANDROID_SOCKET_NAMESPACE_RESERVED 1
#define ANDROID_SOCKET_NAMESPACE_FILESYSTEM 2
extern int socket_loopback_client(int port,int type);
extern int socket_network_client(const char*host,int port,int type);
extern int socket_loopback_server(int port,int type);
extern int socket_local_server(const char*name,int namespaceId,int type);
extern int socket_local_server_bind(int s,const char*name,int namespaceId);
extern int socket_local_client_connect(int fd,const char*name,int namespaceId,int type);
extern int socket_local_client(const char*name,int namespaceId,int type);
extern int socket_inaddr_any_server(int port,int type);
extern bool socket_peer_is_trusted(int fd);
#endif
