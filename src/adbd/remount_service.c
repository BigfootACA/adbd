#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/mount.h>
#include<errno.h>
#include"sysdeps.h"
#define TRACE_TAG TRACE_ADB
#include"adb.h"
static int system_ro=1;
static char*find_mount(const char*dir){
	int fd,res;
	char*token=NULL,buf[4096];
	const char delims[]="\n";
	if((fd=unix_open("/proc/mounts",O_RDONLY))<0)return NULL;
	buf[sizeof(buf)-1]='\0';
	adb_read(fd,buf,sizeof(buf)-1);
	adb_close(fd);
	token=strtok(buf,delims);
	while(token){
		char mount_dev[256],mount_dir[256];
		int mount_freq,mount_passno;
		res=sscanf(token,"%255s %255s %*s %*s %d %d\n",mount_dev,mount_dir,&mount_freq,&mount_passno);
		mount_dev[255]=0;
		mount_dir[255]=0;
		if(res==4&&(strcmp(dir,mount_dir)==0))return strdup(mount_dev);
		token=strtok(NULL,delims);
	}
	return NULL;
}
static int remount_system(){
	char*dev;
	if(system_ro==0)return 0;
	if(!(dev=find_mount("/system")))return-1;
	system_ro=mount(dev,"/system","none",MS_REMOUNT,NULL);
	free(dev);
	return system_ro;
}
static void write_string(int fd,const char*str){writex(fd,str,strlen(str));}
void remount_service(int fd,void*cookie){
	(void)cookie;
	int ret;
	if(!(ret=remount_system()))write_string(fd,"remount succeeded\n");
	else{
		char buffer[200];
		snprintf(buffer,sizeof(buffer),"remount failed: %s\n",strerror(errno));
		write_string(fd,buffer);
	}
	adb_close(fd);
}
