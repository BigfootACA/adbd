#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include"sysdeps.h"
#include<sys/types.h>
#define TRACE_TAG TRACE_TRANSPORT
#include"adb.h"
static int remote_read(apacket*p,atransport*t){
	if(readx(t->sfd,&p->msg,sizeof(amessage)))return -1;
	if(check_header(p))return -1;
	if(readx(t->sfd,p->data,p->msg.data_length))return -1;
	if(check_data(p))return -1;
	return 0;
}
static int remote_write(apacket *p,atransport *t){
	int length=p->msg.data_length;
	if(writex(t->sfd,&p->msg,sizeof(amessage)+length))return -1;
	return 0;
}
int local_connect(int port){return local_connect_arbitrary_ports(port-1,port);}
int local_connect_arbitrary_ports(int console_port,int adb_port){
	char buf[64];
	int fd=-1;
	if(fd<0)fd=socket_loopback_client(adb_port,SOCK_STREAM);
	if(fd>=0){
		close_on_exec(fd);
		disable_tcp_nagle(fd);
		snprintf(buf,sizeof buf,"%s%d",LOCAL_CLIENT_PREFIX,console_port);
		register_socket_transport(fd,buf,adb_port,1);
		return 0;
	}
	return -1;
}
static void*client_socket_thread(void*x){return 0;}
static void*server_socket_thread(void*arg){
	int serverfd,fd;
	struct sockaddr addr;
	socklen_t alen;
	int port=(int)arg;
	serverfd=-1;
	for(;;){
		if(serverfd==-1){
			if((serverfd=socket_inaddr_any_server(port,SOCK_STREAM))<0){
				adb_sleep_ms(1000);
				continue;
			}
			close_on_exec(serverfd);
		}
		alen=sizeof(addr);
		if((fd=adb_socket_accept(serverfd,&addr,&alen))>=0){
			close_on_exec(fd);
			disable_tcp_nagle(fd);
			register_socket_transport(fd,"host",port,1);
		}
	}
	return 0;
}
void local_init(int port){
	adb_thread_t thr;
	void*(*func)(void*);
	func=HOST?client_socket_thread:server_socket_thread;
	if(adb_thread_create(&thr,func,(void*)port))fatal_errno("adbd: cannot create local socket %s thread",HOST?"client":"server");
}
static void remote_kick(atransport*t){
	int fd=t->sfd;
	t->sfd=-1;
	adb_shutdown(fd);
	adb_close(fd);
}
static void remote_close(atransport*t){adb_close(t->fd);}
int init_socket_transport(atransport*t,int s,int adb_port,int local){
	int fail=0;
	t->kick=remote_kick;
	t->close=remote_close;
	t->read_from_remote=remote_read;
	t->write_to_remote=remote_write;
	t->sfd=s;
	t->sync_token=1;
	t->connection_state=CS_OFFLINE;
	t->type=kTransportLocal;
	t->adb_port=0;
	return fail;
}
