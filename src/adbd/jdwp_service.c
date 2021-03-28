#include "sysdeps.h"
#define TRACE_TAG TRACE_JDWP
#include "adb.h"
#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#define MAX_OUT_FDS 4
#include<sys/socket.h>
#include<sys/un.h>
typedef struct JdwpProcess JdwpProcess;
struct JdwpProcess{
	JdwpProcess*next,*prev;
	int pid,socket;
	fdevent*fde;
	char in_buff[4];
	int in_len,out_fds[MAX_OUT_FDS],out_count;
};
static JdwpProcess _jdwp_list;
static int jdwp_process_list(char*buffer,int bufferlen){
	char*end=buffer+bufferlen;
	char*p=buffer;
	JdwpProcess*proc=_jdwp_list.next;
	for(;proc !=&_jdwp_list;proc=proc->next){
		int len;
		if(proc->pid<0)continue;
		len=snprintf(p,end-p,"%d\n",proc->pid);
		if(p+len>=end)break;
		p+=len;
	}
	p[0]=0;
	return(p - buffer);
}
static int jdwp_process_list_msg(char*buffer,int bufferlen){
	char head[5];
	int len=jdwp_process_list(buffer+4,bufferlen-4);
	snprintf(head,sizeof head,"%04x",len);
	memcpy(buffer,head,4);
	return len+4;
}
static void jdwp_process_list_updated(void);
static void
jdwp_process_free(JdwpProcess*proc){
	if(proc){
		int n;
		proc->prev->next=proc->next;
		proc->next->prev=proc->prev;
		if(proc->socket>=0){
			adb_shutdown(proc->socket);
			adb_close(proc->socket);
			proc->socket=-1;
		}
		if(proc->fde !=NULL){
			fdevent_destroy(proc->fde);
			proc->fde=NULL;
		}
		proc->pid=-1;
		for(n=0;n<proc->out_count;n++)adb_close(proc->out_fds[n]);
		proc->out_count=0;
		free(proc);
		jdwp_process_list_updated();
	}
}
static void jdwp_process_event(int,unsigned,void*);
static JdwpProcess*jdwp_process_alloc(int socket){
	JdwpProcess*proc;
	if((proc=calloc(1,sizeof(*proc)))==NULL){
		printf("not enough memory to create new JDWP process\n");
		return NULL;
	}
	proc->socket=socket;
	proc->pid=-1;
	proc->next=proc;
	proc->prev=proc;
	if((proc->fde=fdevent_create(socket,jdwp_process_event,proc))==NULL){
		printf("adbd: could not create fdevent for new JDWP process\n");
		free(proc);
		return NULL;
	}
	proc->fde->state|=FDE_DONT_CLOSE;
	proc->in_len=0;
	proc->out_count=0;
	proc->next=&_jdwp_list;
	proc->prev=proc->next->prev;
	proc->prev->next=proc;
	proc->next->prev=proc;
	fdevent_add(proc->fde,FDE_READ);
	return proc;
}
static void
jdwp_process_event(int socket,unsigned events,void*_proc){
	JdwpProcess*proc=_proc;
	if(events&FDE_READ){
		if(proc->pid<0){
			char*p=proc->in_buff+proc->in_len;
			int size=4-proc->in_len;
			char temp[5];
			while(size>0){
				int len=recv(socket,p,size,0);
				if(len<0){
					if(errno==EINTR)continue;
					if(errno==EAGAIN)return;
					printf("adbd: weird unknown JDWP process failure: %m\n");
					goto CloseProcess;
				}
				if(len==0){
					printf("adbd: weird end-of-stream from unknown JDWP process\n");
					goto CloseProcess;
				}
				p+=len;
				proc->in_len+=len;
				size -=len;
			}
			memcpy(temp,proc->in_buff,4);
			temp[4]=0;
			if(sscanf(temp,"%04x",&proc->pid)!=1)goto CloseProcess;
			printf("adbd: adding pid %d to jdwp process list\n",proc->pid);
			jdwp_process_list_updated();
		}else{
			char buf[32];
			for(;;){
				int len=recv(socket,buf,sizeof(buf),0);
				if(len<=0){
					if(len<0&&errno==EINTR)continue;
					if(len<0&&errno==EAGAIN)return;
					else{
						printf("adbd: terminating JDWP %d connection: %s\n",proc->pid,strerror(errno));
						break;
					}
				}else printf("adbd: ignoring unexpected JDWP %d control socket activity(%d bytes)\n",proc->pid,len);
			}
		CloseProcess:
			if(proc->pid>=0)printf("adbd: remove pid %d to jdwp process list\n",proc->pid);
			jdwp_process_free(proc);
			return;
		}
	}
	if(events&FDE_WRITE){
		if(proc->out_count>0){
			int fd=proc->out_fds[0],n,ret;
			struct cmsghdr*cmsg;
			struct msghdr msg;
			struct iovec iov;
			char dummy='!';
			char buffer[sizeof(struct cmsghdr)+sizeof(int)];
			int flags;
			iov.iov_base=&dummy;
			iov.iov_len=1;
			msg.msg_name=NULL;
			msg.msg_namelen=0;
			msg.msg_iov=&iov;
			msg.msg_iovlen=1;
			msg.msg_flags=0;
			msg.msg_control=buffer;
			msg.msg_controllen=sizeof(buffer);
			cmsg=CMSG_FIRSTHDR(&msg);
			cmsg->cmsg_len=msg.msg_controllen;
			cmsg->cmsg_level=SOL_SOCKET;
			cmsg->cmsg_type=SCM_RIGHTS;
			((int*)CMSG_DATA(cmsg))[0]=fd;
			if((flags=fcntl(proc->socket,F_GETFL,0))==-1){
				printf("adbd: failed to get cntl flags for socket %d: %m\n",proc->pid);
				goto CloseProcess;
			}
			if(fcntl(proc->socket,F_SETFL,flags&~O_NONBLOCK)==-1){
				printf("adbd: failed to remove O_NONBLOCK flag for socket %d: %m\n",proc->pid);
				goto CloseProcess;
			}
			for(;;){
				if((ret=sendmsg(proc->socket,&msg,0))>=0){adb_close(fd);break;}
				if(errno==EINTR)continue;
				printf("adbd: sending new file descriptor to JDWP %d failed: %m\n",proc->pid);
				goto CloseProcess;
			}
			printf("sent file descriptor %d to JDWP process %d\n",fd,proc->pid);
			for(n=1;n<proc->out_count;n++)proc->out_fds[n-1]=proc->out_fds[n];
			if(fcntl(proc->socket,F_SETFL,flags)==-1){
				printf("adbd: failed to set O_NONBLOCK flag for socket %d: %m\n",proc->pid);
				goto CloseProcess;
			}
			if(--proc->out_count==0)fdevent_del(proc->fde,FDE_WRITE);
		}
	}
}
int create_jdwp_connection_fd(int pid){
	JdwpProcess*proc=_jdwp_list.next;
	printf("looking for pid %d in JDWP process list\n",pid);
	for(;proc !=&_jdwp_list;proc=proc->next)if(proc->pid==pid)goto FoundIt;
	printf("search failed !!\n");
	return -1;
	FoundIt:{
		int fds[2];
		if(proc->out_count>=MAX_OUT_FDS){
			printf("adbd: too many pending JDWP connection for pid %d\n",pid);
			return -1;
		}
		if(adb_socketpair(fds)<0){
			printf("adbd: socket pair creation failed: %m\n");
			return -1;
		}
		proc->out_fds[proc->out_count]=fds[1];
		if(++proc->out_count==1)fdevent_add(proc->fde,FDE_WRITE);
		return fds[0];
	}
}
#define JDWP_CONTROL_NAME "\0jdwp-control"
#define JDWP_CONTROL_NAME_LEN (sizeof(JDWP_CONTROL_NAME)-1)
typedef struct{int listen_socket;fdevent*fde;}JdwpControl;
static void jdwp_control_event(int s,unsigned events,void*user);
static int jdwp_control_init(JdwpControl*control,const char*sockname,int socknamelen){
	struct sockaddr_un addr;
	socklen_t addrlen;
	int s,maxpath=sizeof(addr.sun_path),pathlen=socknamelen;
	if(pathlen>=maxpath){
		printf("adbd: vm debug control socket name too long(%d extra chars)\n",pathlen+1-maxpath);
		return -1;
	}
	memset(&addr,0,sizeof(addr));
	addr.sun_family=AF_UNIX;
	memcpy(addr.sun_path,sockname,socknamelen);
	if((s=socket(AF_UNIX,SOCK_STREAM,0))<0){
		printf("adbd: could not create vm debug control socket: %m\n");
		return -1;
	}
	addrlen=(pathlen+sizeof(addr.sun_family));
	if(bind(s,(struct sockaddr*)&addr,addrlen)<0){
		printf("adbd: could not bind vm debug control socket: %m\n");
		adb_close(s);
		return -1;
	}
	if(listen(s,4)<0){
		printf("adbd: listen failed in jdwp control socket: %m\n");
		adb_close(s);
		return -1;
	}
	control->listen_socket=s;
	control->fde=fdevent_create(s,jdwp_control_event,control);
	if(control->fde==NULL){
		printf("adbd: could not create fdevent for jdwp control socket\n");
		adb_close(s);
		return -1;
	}
	fdevent_add(control->fde,FDE_READ);
	close_on_exec(s);
	printf("adbd: jdwp control socket started\n");
	return 0;
}
static void jdwp_control_event(int s,unsigned events,void*_control){
	(void)s;
	JdwpControl*control=(JdwpControl*)_control;
	if(events&FDE_READ){
		struct sockaddr addr;
		socklen_t addrlen=sizeof(addr);
		int s=-1;
		JdwpProcess*proc;
		do{
			if((s=adb_socket_accept(control->listen_socket,&addr,&addrlen))>=0)continue;
			if(errno==EINTR)continue;
			if(errno==ECONNABORTED){
				printf("adbd: oops,the JDWP process died really quick\n");
				return;
			}
			printf("adbd: weird accept failed on jdwp control socket: %m\n");
			return;
		}while(s<0);
		if((proc=jdwp_process_alloc(s))==NULL)return;
	}
}
static JdwpControl _jdwp_control;
typedef struct{asocket socket;int pass;}JdwpSocket;
static void jdwp_socket_close(asocket*s){
	asocket*peer=s->peer;
	remove_socket(s);
	if(peer){peer->peer=NULL;peer->close(peer);}
	free(s);
}
static int jdwp_socket_enqueue(asocket*s,apacket*p){
	put_apacket(p);
	s->peer->close(s->peer);
	return -1;
}
static void jdwp_socket_ready(asocket*s){
	JdwpSocket*jdwp=(JdwpSocket*)s;
	asocket*peer=jdwp->socket.peer;
	if(jdwp->pass==0){
		apacket*p=get_apacket();
		p->len=jdwp_process_list((char*)p->data,MAX_PAYLOAD);
		peer->enqueue(peer,p);
		jdwp->pass=1;
	}else peer->close(peer);
}
asocket*
create_jdwp_service_socket(void){
	JdwpSocket*s=calloc(sizeof(*s),1);
	if(s==NULL)return NULL;
	install_local_socket(&s->socket);
	s->socket.ready=jdwp_socket_ready;
	s->socket.enqueue=jdwp_socket_enqueue;
	s->socket.close=jdwp_socket_close;
	s->pass=0;
	return&s->socket;
}
typedef struct JdwpTracker JdwpTracker;
struct JdwpTracker{asocket socket;JdwpTracker*next,*prev;int need_update;};
static JdwpTracker _jdwp_trackers_list;
static void jdwp_process_list_updated(void){
	char buffer[1024];
	int len;
	JdwpTracker*t=_jdwp_trackers_list.next;
	len=jdwp_process_list_msg(buffer,sizeof(buffer));
	for(;t!=&_jdwp_trackers_list;t=t->next){
		apacket*p=get_apacket();
		asocket*peer=t->socket.peer;
		memcpy(p->data,buffer,len);
		p->len=len;
		peer->enqueue(peer,p);
	}
}
static void jdwp_tracker_close(asocket*s){
	JdwpTracker*tracker=(JdwpTracker*)s;
	asocket*peer=s->peer;
	if(peer){peer->peer=NULL;peer->close(peer);}
	remove_socket(s);
	tracker->prev->next=tracker->next;
	tracker->next->prev=tracker->prev;
	free(s);
}
static void jdwp_tracker_ready(asocket*s){
	JdwpTracker*t=(JdwpTracker*)s;
	if(!t->need_update)return;
	apacket*p=get_apacket();
	t->need_update=0;
	p->len=jdwp_process_list_msg((char*)p->data,sizeof(p->data));
	s->peer->enqueue(s->peer,p);
}
static int jdwp_tracker_enqueue(asocket*s,apacket*p){
	put_apacket(p);
	s->peer->close(s->peer);
	return -1;
}
asocket*create_jdwp_tracker_service_socket(void){
	JdwpTracker*t=calloc(sizeof(*t),1);
	if(t==NULL)return NULL;
	t->next=&_jdwp_trackers_list;
	t->prev=t->next->prev;
	t->next->prev=t;
	t->prev->next=t;
	install_local_socket(&t->socket);
	t->socket.ready=jdwp_tracker_ready;
	t->socket.enqueue=jdwp_tracker_enqueue;
	t->socket.close=jdwp_tracker_close;
	t->need_update=1;
	return&t->socket;
}
int init_jdwp(void){
	_jdwp_list.next=&_jdwp_list;
	_jdwp_list.prev=&_jdwp_list;
	_jdwp_trackers_list.next=&_jdwp_trackers_list;
	_jdwp_trackers_list.prev=&_jdwp_trackers_list;
	return jdwp_control_init(&_jdwp_control,JDWP_CONTROL_NAME,JDWP_CONTROL_NAME_LEN);
}
