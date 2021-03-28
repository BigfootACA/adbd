#include<sys/ioctl.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<stdarg.h>
#include<stddef.h>
#include"fdevent.h"
#include"transport.h"
#include"sysdeps.h"
int SHELL_EXIT_NOTIFY_FD=-1;
static void fatal(const char*fn,const char*fmt,...){
	va_list ap;
	va_start(ap,fmt);
	fprintf(stderr,"%s:",fn);
	vfprintf(stderr,fmt,ap);
	va_end(ap);
	abort();
}
#define FATAL(x...) fatal(__FUNCTION__,x)
#define D(...) ((void)0)
#define dump_fde(fde,info) do{}while(0)
#define FDE_EVENTMASK  0x00ff
#define FDE_STATEMASK  0xff00
#define FDE_ACTIVE     0x0100
#define FDE_PENDING    0x0200
#define FDE_CREATED    0x0400
static void fdevent_plist_enqueue(fdevent*node);
static void fdevent_plist_remove(fdevent*node);
static fdevent*fdevent_plist_dequeue(void);
static void fdevent_subproc_event_func(int fd,unsigned events,void*userdata);
static fdevent list_pending={.next=&list_pending,.prev=&list_pending,};
static fdevent**fd_table=0;
static int fd_table_max=0;
#include <sys/epoll.h>
static int epoll_fd=-1;
static void fdevent_init(){
	if((epoll_fd=epoll_create(256))<0){
		perror("epoll_create");
		exit(1);
		return;
	}
	fcntl(epoll_fd,F_SETFD,FD_CLOEXEC);
}
static void fdevent_connect(fdevent*fde){
	struct epoll_event ev;
	memset(&ev,0,sizeof(ev));
	ev.events=0;
	ev.data.ptr=fde;

}
static void fdevent_disconnect(fdevent*fde){
	struct epoll_event ev;
	memset(&ev,0,sizeof(ev));
	ev.events=0;
	ev.data.ptr=fde;
	epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fde->fd,&ev);
}
static void fdevent_update(fdevent*fde,unsigned events){
	struct epoll_event ev;
	int active=(fde->state&FDE_EVENTMASK)!=0;
	memset(&ev,0,sizeof(ev));
	ev.events=0;
	ev.data.ptr=fde;
	if(events&FDE_READ)ev.events|=EPOLLIN;
	if(events&FDE_WRITE)ev.events|=EPOLLOUT;
	if(events&FDE_ERROR)ev.events|=(EPOLLERR|EPOLLHUP);
	fde->state=(fde->state&FDE_STATEMASK)|events;
	int x;
	if(ev.events)x=active?EPOLL_CTL_MOD:EPOLL_CTL_ADD;
	else if(active)x=EPOLL_CTL_DEL;
	else return;
	if(epoll_ctl(epoll_fd,x,fde->fd,&ev)){
		perror("epoll_ctl\n");
		exit(1);
	}
}
static void fdevent_process(){
	struct epoll_event events[256];
	fdevent*fde;
	int i,n;
	if((n=epoll_wait(epoll_fd,events,256,-1))<0){
		if(errno==EINTR)return;
		perror("epoll_wait");
		exit(1);
		return;
	}
	for(i=0;i<n;i++){
		struct epoll_event*ev=events+i;
		fde=ev->data.ptr;
		if(ev->events&EPOLLIN)fde->events|=FDE_READ;
		if(ev->events&EPOLLOUT)fde->events|=FDE_WRITE;
		if(ev->events&(EPOLLERR|EPOLLHUP))fde->events|=FDE_ERROR;
		if(fde->events){
			if(fde->state&FDE_PENDING)continue;
			fde->state|=FDE_PENDING;
			fdevent_plist_enqueue(fde);
		}
	}
}
static void fdevent_register(fdevent*fde){
	if(fde->fd<0)FATAL("bogus negative fd (%d)\n",fde->fd);
	if(fde->fd>=fd_table_max){
		int oldmax=fd_table_max;
		if(fde->fd>32000)FATAL("bogus huuuuge fd (%d)\n",fde->fd);
		if(fd_table_max==0){
			fdevent_init();
			fd_table_max=256;
		}
		while(fd_table_max<=fde->fd)fd_table_max*=2;
		fd_table=realloc(fd_table,sizeof(fdevent*)*fd_table_max);
		if(fd_table==0)FATAL("could not expand fd_table to %d entries\n",fd_table_max);
		memset(fd_table+oldmax,0,sizeof(int)*(fd_table_max-oldmax));
	}
	fd_table[fde->fd]=fde;
}
static void fdevent_unregister(fdevent*fde){
	if((fde->fd<0)||(fde->fd >=fd_table_max))FATAL("fd out of range (%d)\n",fde->fd);
	if(fd_table[fde->fd] !=fde)FATAL("fd_table out of sync [%d]\n",fde->fd);
	fd_table[fde->fd]=0;
	if(!(fde->state & FDE_DONT_CLOSE)) {
		dump_fde(fde,"close");
		adb_close(fde->fd);
	}
}
static void fdevent_plist_enqueue(fdevent*node){
	fdevent*list=&list_pending;
	node->next=list;
	node->prev=list->prev;
	node->prev->next=node;
	list->prev=node;
}
static void fdevent_plist_remove(fdevent*node){
	node->prev->next=node->next;
	node->next->prev=node->prev;
	node->next=0;
	node->prev=0;
}
static fdevent*fdevent_plist_dequeue(void){
	fdevent*list=&list_pending;
	fdevent*node=list->next;
	if(node==list)return 0;
	list->next=node->next;
	list->next->prev=list;
	node->next=0;
	node->prev=0;
	return node;
}
static void fdevent_call_fdfunc(fdevent*fde){
	unsigned events=fde->events;
	fde->events=0;
	if(!(fde->state&FDE_PENDING))return;
	fde->state &=(~FDE_PENDING);
	dump_fde(fde,"callback");
	fde->func(fde->fd,events,fde->arg);
}
static void fdevent_subproc_event_func(int fd,unsigned ev,void*userdata){
	if((fd<0)||(fd >=fd_table_max))FATAL("fd %d out of range for fd_table \n",fd);
	fdevent*fde=fd_table[fd];
	fdevent_add(fde,FDE_READ);
	if(ev&FDE_READ){
		int subproc_fd;
		if(readx(fd,&subproc_fd,sizeof(subproc_fd)))FATAL("Failed to read the subproc's fd from fd=%d\n",fd);
		if((subproc_fd<0)||(subproc_fd>=fd_table_max))return;
		fdevent*subproc_fde=fd_table[subproc_fd];
		if(!subproc_fde)return;
		if(subproc_fde->fd!=subproc_fd)return;
		subproc_fde->force_eof=1;
		int rcount=0;
		ioctl(subproc_fd,FIONREAD,&rcount);
		if(rcount)return;
		subproc_fde->events|=FDE_READ;
		if(subproc_fde->state&FDE_PENDING)return;
		subproc_fde->state|=FDE_PENDING;
		fdevent_call_fdfunc(subproc_fde);
	}
}
fdevent*fdevent_create(int fd,fd_func func,void*arg){
	fdevent*fde=(fdevent*)malloc(sizeof(fdevent));
	if(fde==0)return 0;
	fdevent_install(fde,fd,func,arg);
	fde->state|=FDE_CREATED;
	return fde;
}
void fdevent_destroy(fdevent*fde){
	if(fde==0)return;
	if(!(fde->state&FDE_CREATED))FATAL("fde %p not created by fdevent_create\n",fde);
	fdevent_remove(fde);
}
void fdevent_install(fdevent*fde,int fd,fd_func func,void*arg){
	memset(fde,0,sizeof(fdevent));
	fde->state=FDE_ACTIVE;
	fde->fd=fd;
	fde->force_eof=0;
	fde->func=func;
	fde->arg=arg;
	fcntl(fd,F_SETFL,O_NONBLOCK);
	fdevent_register(fde);
	dump_fde(fde,"connect");
	fdevent_connect(fde);
	fde->state|=FDE_ACTIVE;
}
void fdevent_remove(fdevent*fde){
	if(fde->state&FDE_PENDING)fdevent_plist_remove(fde);
	if(fde->state&FDE_ACTIVE){
		fdevent_disconnect(fde);
		dump_fde(fde,"disconnect");
		fdevent_unregister(fde);
	}
	fde->state=0;
	fde->events=0;
}
void fdevent_set(fdevent*fde,unsigned events){
	events&=FDE_EVENTMASK;
	if((fde->state&FDE_EVENTMASK)==events) return;
	if(fde->state&FDE_ACTIVE) {
		fdevent_update(fde,events);
		dump_fde(fde,"update");
	}
	fde->state=(fde->state&FDE_STATEMASK)|events;
	if(fde->state&FDE_PENDING){
		fde->events&=(~events);
		if(fde->events==0){
			fdevent_plist_remove(fde);
			fde->state&=(~FDE_PENDING);
		}
	}
}
void fdevent_add(fdevent*fde,unsigned events){fdevent_set(fde,(fde->state&FDE_EVENTMASK)|(events&FDE_EVENTMASK));}
void fdevent_del(fdevent*fde,unsigned events){fdevent_set(fde,(fde->state&FDE_EVENTMASK)&(~(events & FDE_EVENTMASK)));}
void fdevent_subproc_setup(){
	int s[2];
	if(adb_socketpair(s))FATAL("cannot create shell-exit socket-pair\n");
	SHELL_EXIT_NOTIFY_FD=s[0];
	fdevent*fde;
	if(!(fde=fdevent_create(s[1],fdevent_subproc_event_func,NULL)))FATAL("cannot create fdevent for shell-exit handler\n");
	fdevent_add(fde,FDE_READ);
}
void fdevent_loop(){
	fdevent*fde;
	fdevent_subproc_setup();
	for(;;){
		fdevent_process();
		while((fde=fdevent_plist_dequeue()))fdevent_call_fdfunc(fde);
	}
}
