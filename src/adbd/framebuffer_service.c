#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "fdevent.h"
#include "adb.h"
#include <cutils/fs.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#define DDMS_RAWIMAGE_VERSION 1
struct fbinfo{unsigned int version,bpp,size,width,height,red_offset,red_length,blue_offset,blue_length,green_offset,green_length,alpha_offset,alpha_length;} __attribute__((packed));
void framebuffer_service(int fd,void*cookie){
	(void)cookie;
	struct fbinfo fbinfo;
	unsigned int i;
	char buf[640];
	int fd_screencap;
	int w,h,f;
	int fds[2];
	pid_t pid=-1;
	if(pipe(fds)<0)goto done;
	if((pid=fork())<0)goto done;
	if(pid==0){
		dup2(fds[1],STDOUT_FILENO);
		close(fds[0]);
		close(fds[1]);
		execlp("screencap","screncap",NULL);
		exit(1);
	}
	fd_screencap=fds[0];
	if(readx(fd_screencap,&w,4)||readx(fd_screencap,&h,4)||readx(fd_screencap,&f,4))goto done;
	fbinfo.version=DDMS_RAWIMAGE_VERSION;
	switch(f){
		case 1: /*RGBA_8888*/
			fbinfo.bpp=32;fbinfo.size=w*h*4;
			fbinfo.width=w;fbinfo.height=h;
			fbinfo.red_offset=0;fbinfo.red_length=8;
			fbinfo.green_offset=8;fbinfo.green_length=8;
			fbinfo.blue_offset=16;fbinfo.blue_length=8;
			fbinfo.alpha_offset=24;fbinfo.alpha_length=8;
		break;
		case 2: /*RGBX_8888*/
			fbinfo.bpp=32;fbinfo.size=w*h*4;
			fbinfo.width=w;fbinfo.height=h;
			fbinfo.red_offset=0;fbinfo.red_length=8;
			fbinfo.green_offset=8;fbinfo.green_length=8;
			fbinfo.blue_offset=16;fbinfo.blue_length=8;
			fbinfo.alpha_offset=24;fbinfo.alpha_length=0;
		break;
		case 3: /*RGB_888*/
			fbinfo.bpp=24;fbinfo.size=w*h*3;
			fbinfo.width=w;fbinfo.height=h;
			fbinfo.red_offset=0;fbinfo.red_length=8;
			fbinfo.green_offset=8;fbinfo.green_length=8;
			fbinfo.blue_offset=16;fbinfo.blue_length=8;
			fbinfo.alpha_offset=24;fbinfo.alpha_length=0;
		break;
		case 4: /*RGB_565*/
			fbinfo.bpp=16;fbinfo.size=w*h*2;
			fbinfo.width=w;fbinfo.height=h;
			fbinfo.red_offset=11;fbinfo.red_length=5;
			fbinfo.green_offset=5;fbinfo.green_length=6;
			fbinfo.blue_offset=0;fbinfo.blue_length=5;
			fbinfo.alpha_offset=0;fbinfo.alpha_length=0;
		break;
		case 5: /*BGRA_8888*/
			fbinfo.bpp=32;fbinfo.size=w*h*4;
			fbinfo.width=w;fbinfo.height=h;
			fbinfo.red_offset=16;fbinfo.red_length=8;
			fbinfo.green_offset=8;fbinfo.green_length=8;
			fbinfo.blue_offset=0;fbinfo.blue_length=8;
			fbinfo.alpha_offset=24;fbinfo.alpha_length=8;
		break;
		default:goto done;
	}
	if(writex(fd,&fbinfo,sizeof(fbinfo)))goto done;
	for(i=0;i<fbinfo.size;i+=sizeof(buf))if(readx(fd_screencap,buf,sizeof(buf))||writex(fd,buf,sizeof(buf)))goto done;
	if(readx(fd_screencap,buf,fbinfo.size%sizeof(buf))||writex(fd,buf,fbinfo.size%sizeof(buf)))goto done;
done:
	TEMP_FAILURE_RETRY(waitpid(pid,NULL,0));
	close(fds[0]);
	close(fds[1]);
	close(fd);
}
