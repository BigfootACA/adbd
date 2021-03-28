#ifndef __FDEVENT_H
#define __FDEVENT_H
#include<stdint.h>
#define FDE_READ              0x0001
#define FDE_WRITE             0x0002
#define FDE_ERROR             0x0004
#define FDE_TIMEOUT           0x0008
#define FDE_DONT_CLOSE        0x0080
typedef struct fdevent fdevent;
typedef void (*fd_func)(int fd,unsigned events,void*userdata);
extern fdevent*fdevent_create(int fd,fd_func func,void*arg);
extern void fdevent_destroy(fdevent*fde);
extern void fdevent_install(fdevent*fde,int fd,fd_func func,void *arg);
extern void fdevent_remove(fdevent*item);
extern void fdevent_set(fdevent*fde,unsigned events);
extern void fdevent_add(fdevent*fde,unsigned events);
extern void fdevent_del(fdevent*fde,unsigned events);
extern void fdevent_loop();
struct fdevent{
	fdevent *next,*prev;
	int fd,force_eof;
	unsigned short state,events;
	fd_func func;
	void *arg;
};
#endif
