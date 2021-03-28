#ifndef _ADB_SYSDEPS_H
#define _ADB_SYSDEPS_H
#include "fdevent.h"
#include<cutils/sockets.h>
#include<cutils/properties.h>
#include<cutils/misc.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<pthread.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdarg.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<string.h>
#include<unistd.h>
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp)({typeof(exp)_rc;do{_rc=(exp);}while(_rc==-1&&errno==EINTR);_rc;})
#endif
#define OS_PATH_SEPARATOR '/'
#define OS_PATH_SEPARATOR_STR "/"
#define ENV_PATH_SEPARATOR_STR ":"
typedef pthread_mutex_t adb_mutex_t;
#define ADB_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define adb_mutex_init pthread_mutex_init
#define adb_mutex_lock pthread_mutex_lock
#define adb_mutex_unlock pthread_mutex_unlock
#define adb_mutex_destroy pthread_mutex_destroy
#define ADB_MUTEX_DEFINE(m) adb_mutex_t m=PTHREAD_MUTEX_INITIALIZER
#define adb_cond_t pthread_cond_t
#define adb_cond_init pthread_cond_init
#define adb_cond_wait pthread_cond_wait
#define adb_cond_broadcast pthread_cond_broadcast
#define adb_cond_signal pthread_cond_signal
#define adb_cond_destroy pthread_cond_destroy
#define ADB_MUTEX(x) extern adb_mutex_t x;
#include "mutex_list.h"
static __inline__ void close_on_exec(int fd){fcntl(fd,F_SETFD,FD_CLOEXEC);}
static __inline__ int unix_open(const char*path,int options,...){
	if((options&O_CREAT)==0)return TEMP_FAILURE_RETRY(open(path,options));
	else{
		int mode;
		va_list args;
		va_start(args,options);
		mode=va_arg(args,int);
		va_end(args);
		return TEMP_FAILURE_RETRY(open(path,options,mode));
	}
}
static __inline__ int adb_open_mode(const char*pathname,int options,int mode){return TEMP_FAILURE_RETRY(open(pathname,options,mode));}
static __inline__ int adb_open(const char*pathname,int options){
	int fd=TEMP_FAILURE_RETRY(open(pathname,options));
	if(fd<0)return -1;
	close_on_exec(fd);
	return fd;
}
#undef open
#define open ___xxx_open
static __inline__ int adb_shutdown(int fd){return shutdown(fd,SHUT_RDWR);}
#undef shutdown
#define shutdown ____xxx_shutdown
static __inline__ int adb_close(int fd){return close(fd);}
#undef close
#define close ____xxx_close
static __inline__ int adb_read(int fd,void*buf,size_t len){return TEMP_FAILURE_RETRY(read(fd,buf,len));}
#undef read
#define read ___xxx_read
static __inline__ int adb_write(int fd,const void*buf,size_t len){return TEMP_FAILURE_RETRY(write(fd,buf,len));}
#undef write
#define write ___xxx_write
static __inline__ int adb_lseek(int fd,int pos,int where){return lseek(fd,pos,where);}
#undef lseek
#define lseek ___xxx_lseek
static __inline__ int adb_unlink(const char*path){return unlink(path);}
#undef unlink
#define unlink ___xxx_unlink
static __inline__ int adb_creat(const char*path,int mode){
	int fd=TEMP_FAILURE_RETRY(creat(path,mode));
	if(fd<0)return -1;
	close_on_exec(fd);
	return fd;
}
#undef creat
#define creat ___xxx_creat
static __inline__ int adb_socket_accept(int serverfd,struct sockaddr*addr,socklen_t*addrlen){
	int fd;
	fd=TEMP_FAILURE_RETRY(accept(serverfd,addr,addrlen));
	if(fd>=0)close_on_exec(fd);
	return fd;
}
#undef accept
#define accept ___xxx_accept
#define unix_read adb_read
#define unix_write adb_write
#define unix_close adb_close
typedef pthread_t adb_thread_t;
typedef void*(*adb_thread_func_t)(void*arg);
static __inline__ int adb_thread_create(adb_thread_t*pthread,adb_thread_func_t start,void*arg){
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	return pthread_create(pthread,&attr,start,arg);
}
static __inline__ int adb_socket_setbufsize(int fd,int bufsize){int opt=bufsize;return setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&opt,sizeof(opt));}
static __inline__ void disable_tcp_nagle(int fd){int on=1;setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(void*)&on,sizeof(on));}
static __inline__ int unix_socketpair(int d,int type,int protocol,int sv[2]){return socketpair(d,type,protocol,sv);}
static __inline__ int adb_socketpair(int sv[2]){
	int rc;
	if((rc=unix_socketpair(AF_UNIX,SOCK_STREAM,0,sv))<0)return -1;
	close_on_exec(sv[0]);
	close_on_exec(sv[1]);
	return 0;
}
#undef socketpair
#define socketpair ___xxx_socketpair
static __inline__ void adb_sleep_ms(int mseconds){usleep(mseconds*1000);}
static __inline__ int adb_mkdir(const char*path,int mode){return mkdir(path,mode);}
#undef mkdir
#define mkdir ___xxx_mkdir
static __inline__ void adb_sysdeps_init(void){}
static __inline__ char*adb_dirstart(const char*path){return strchr(path,'/');}
static __inline__ char*adb_dirstop(const char*path){return strrchr(path,'/');}
static __inline__ int adb_is_absolute_host_path(const char*path){return path[0]=='/';}
static __inline__ char*adb_strtok_r(char*str,const char*delim,char**saveptr){return strtok_r(str,delim,saveptr);}
#undef strtok_r
#define strtok_r ___xxx_strtok_r
#endif
