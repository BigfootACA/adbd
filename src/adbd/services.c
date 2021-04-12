#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<pwd.h>
#include<sys/cdefs.h>
#include<sys/reboot.h>
#include"sysdeps.h"
#define TRACE_TAG  TRACE_SERVICES
#include"adb.h"
#include"file_sync_service.h"
typedef struct stinfo stinfo;
struct stinfo{
	void(*func)(int fd,void*cookie);
	int fd;
	void*cookie;
};
void *service_bootstrap_func(void*x){
	stinfo *sti=x;
	sti->func(sti->fd,sti->cookie);
	free(sti);
	return 0;
}
extern int recovery_mode;
static void recover_service(int s,void*cookie){
	unsigned char buf[4096];
	unsigned count=(unsigned)cookie;
	int fd;
	if((fd=adb_creat("/tmp/update",0644))<0){
		adb_close(s);
		return;
	}
	while(count>0){
		unsigned xfer=(count>4096)?4096:count;
		if(readx(s,buf,xfer)||writex(fd,buf,xfer))break;
		count-=xfer;
	}
	writex(s,count?"OKAY":"FAIL",4);
	adb_close(fd);
	adb_close(s);
	fd=adb_creat("/tmp/update.begin",0644);
	adb_close(fd);
}
void restart_tcp_service(int fd,void*cookie){
	char buf[100];
	char value[PROPERTY_VALUE_MAX];
	int port=(int)cookie;
	if(port<=0){
		snprintf(buf,sizeof(buf),"invalid port\n");
		writex(fd,buf,strlen(buf));
		adb_close(fd);
		return;
	}
	snprintf(value,sizeof(value),"%d",port);
	snprintf(buf,sizeof(buf),"restarting in TCP mode port: %d\n",port);
	writex(fd,buf,strlen(buf));
	adb_close(fd);
}
void restart_usb_service(int fd,void*cookie){
	char buf[100];
	snprintf(buf,sizeof(buf),"restarting in USB mode\n");
	writex(fd,buf,strlen(buf));
	adb_close(fd);
}
void reboot_service(int fd,void*arg){
	int pid,ret;
	sync();
	pid=fork();
	if(pid==0){
		if(execl("/usr/bin/reboot","reboot",arg,NULL)!=0){
			sync();
			reboot(RB_AUTOBOOT);
		}
		exit(0);
		return;
	}else if(pid>0)waitpid(pid,&ret,0);
	free(arg);
	adb_close(fd);
}
static int create_service_thread(void(*func)(int,void*),void*cookie){
	stinfo *sti;
	adb_thread_t t;
	int s[2];
	if(adb_socketpair(s)){
		printf("adbd: cannot create service socket pair\n");
		return -1;
	}
	if(!(sti=malloc(sizeof(stinfo)))){
		fatal("adbd: cannot allocate stinfo");
		return -1;
	}
	sti->func=func;
	sti->cookie=cookie;
	sti->fd=s[1];
	if(adb_thread_create(&t,service_bootstrap_func,sti)){
		free(sti);
		adb_close(s[0]);
		adb_close(s[1]);
		printf("adbd: cannot create service thread\n");
		return -1;
	}
	return s[0];
}
static int create_subprocess(const char *cmd,const char *arg0,const char *arg1,pid_t *pid){
	char *devname;
	int ptm;
	if((ptm=unix_open("/dev/ptmx",O_RDWR))<0){
		printf("adbd: cannot open /dev/ptmx: %m\n");
		return -1;
	}
	fcntl(ptm,F_SETFD,FD_CLOEXEC);
	if(grantpt(ptm)||unlockpt(ptm)||((devname=(char*)ptsname(ptm))==0)){
		printf("adbd: trouble with /dev/ptmx: %m\n");
		adb_close(ptm);
		return -1;
	}
	*pid=fork();
	if(*pid<0){
		printf("adbd: fork failed: %m\n");
		adb_close(ptm);
		return -1;
	}
	if(*pid==0){
		int pts;
		setsid();
		if((pts=unix_open(devname,O_RDWR))<0){
			fprintf(stderr,"adbd: child failed to open pseudo-term slave: %s\n",devname);
			exit(-1);
		}
		dup2(pts,0);
		dup2(pts,1);
		dup2(pts,2);
		adb_close(pts);
		adb_close(ptm);
		char text[64];
		snprintf(text,sizeof text,"/proc/%d/oom_score_adj",getpid());
		int fd;
		if((fd=adb_open(text,O_WRONLY))>=0){
			adb_write(fd,"0",1);
			adb_close(fd);
		}else printf("adbd: unable to open %s\n",text);
		char*g;
		if(!getenv("TERM"))setenv("TERM","xterm-256color",0);
		if((g=getenv("HOME"))&&cmd==shell&&chdir(g)<0)fprintf(stderr,"adbd: chdir to %s failed\n",g);
		execl(cmd,cmd,arg0,arg1,NULL);
		if(errno>0)fprintf(stderr,"adbd: exec '%s' failed: %m\n",cmd);
		exit(-1);
	}else return ptm;
}
static void subproc_waiter_service(int fd,void *cookie){
	pid_t pid=(pid_t)cookie;
	for(;;){
		int status;
		if((waitpid(pid,&status,0))==pid){
			if(WIFSIGNALED(status))printf("adbd: subproc killed by signal %d\n",WTERMSIG(status));
			else if(!WIFEXITED(status))printf("adbd: subproc did not exit! status %d\n",status);
			else if(WEXITSTATUS(status)>=0)printf("adbd: subproc exit code %d\n",WEXITSTATUS(status));
			else continue;
			break;
		}
	}
	printf("adbd: shell exited pid %d",pid);
	printf(errno>0?": %m\n":".\n");
	if(SHELL_EXIT_NOTIFY_FD>=0){
		writex(SHELL_EXIT_NOTIFY_FD,&fd,sizeof(fd));
		printf("adbd: notified shell exit pid %d",pid);
		printf(errno>0?": %m\n":".\n");
	}
}
static int create_subproc_thread(const char *name){
	stinfo *sti;
	adb_thread_t t;
	int ret_fd;
	pid_t pid=0;
	ret_fd=name?create_subprocess(shell,"-c",name,&pid):create_subprocess(shell,"-",0,&pid);
	printf("adbd: create_subprocess fd %d pid %d\n",ret_fd,pid);
	sti=malloc(sizeof(stinfo));
	if(!sti){
		fatal("adbd: cannot allocate stinfo");
		return -1;
	}
	sti->func=subproc_waiter_service;
	sti->cookie=(void*)pid;
	sti->fd=ret_fd;
	if(adb_thread_create( &t,service_bootstrap_func,sti)){
		free(sti);
		adb_close(ret_fd);
		printf("adbd: cannot create service thread\n");
		return -1;
	}
	return ret_fd;
}
int service_to_fd(const char *name){
	int ret=-1;
	if(!strncmp(name,"tcp:",4)){
		int port=atoi(name+ 4);
		name=strchr(name+4,':');
		if(name!=0)return -1;
		else if((ret=socket_loopback_client(port,SOCK_STREAM))>=0)disable_tcp_nagle(ret);
	}else if(!strncmp(name,"local:",6))ret=socket_local_client(name+6,ANDROID_SOCKET_NAMESPACE_RESERVED,SOCK_STREAM);
	else if(!strncmp(name,"localreserved:",14))ret=socket_local_client(name+14,ANDROID_SOCKET_NAMESPACE_RESERVED,SOCK_STREAM);
	else if(!strncmp(name,"localabstract:",14))ret=socket_local_client(name+14,ANDROID_SOCKET_NAMESPACE_ABSTRACT,SOCK_STREAM);
	else if(!strncmp(name,"localfilesystem:",16))ret=socket_local_client(name+16,ANDROID_SOCKET_NAMESPACE_FILESYSTEM,SOCK_STREAM);
	else if(!strncmp("dev:",name,4))ret=unix_open(name+ 4,O_RDWR);
	else if(!strncmp(name,"framebuffer:",12))ret=create_service_thread(framebuffer_service,0);
	else if(recovery_mode&&!strncmp(name,"recover:",8))ret=create_service_thread(recover_service,(void*)atoi(name+8));
	else if(!strncmp(name,"jdwp:",5))ret=create_jdwp_connection_fd(atoi(name+5));
	else if(!strncmp(name,"log:",4))ret=create_service_thread(log_service,get_log_file_path(name+4));
	else if(!HOST&&!strncmp(name,"shell:",6))ret=name[6]?create_subproc_thread(name+6):create_subproc_thread(0);
	else if(!strncmp(name,"sync:",5))ret=create_service_thread(file_sync_service,NULL);
	else if(!strncmp(name,"remount:",8))ret=create_service_thread(remount_service,NULL);
	else if(!strncmp(name,"reboot:",7)){
		void*arg=strdup(name+7);
		if(!arg)return -1;
		ret=create_service_thread(reboot_service,arg);
	}else if(!strncmp(name,"tcpip:",6)){
		int port;
		if(sscanf(name+6,"%d",&port)==0)port=0;
		ret=create_service_thread(restart_tcp_service,(void*)port);
	}else if(!strncmp(name,"usb:",4))ret=create_service_thread(restart_usb_service,NULL);
	if (ret >=0)close_on_exec(ret);
	return ret;
}
