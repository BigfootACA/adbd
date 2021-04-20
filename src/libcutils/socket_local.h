#ifndef __SOCKET_LOCAL_H
#define __SOCKET_LOCAL_H
#define FILESYSTEM_SOCKET_PREFIX "/tmp/"
#define ANDROID_RESERVED_SOCKET_PREFIX "/run/"
#include<sys/socket.h>
extern int socket_make_sockaddr_un(const char*name,int namespaceId,struct sockaddr_un*p_addr,socklen_t*alen);
#endif
