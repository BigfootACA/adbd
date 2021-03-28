#define TRACE_TAG   TRACE_ADB
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<stdbool.h>
#include<ctype.h>
#include<errno.h>
#include<getopt.h>
#include<stddef.h>
#include<string.h>
#include<limits.h>
#include<time.h>
#include<sys/time.h>
#include<stdint.h>
#include<sys/types.h>
#include<sys/prctl.h>
#include<sys/mount.h>
#include<grp.h>
#include<pwd.h>
#include<shadow.h>
#include<unistd.h>
#include"sysdeps.h"
#include"adb.h"
#include"adb_auth.h"
#include"android_filesystem_config.h"
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
ADB_MUTEX_DEFINE(D_lock);
char*shell=NULL;
int HOST=0,gListenAll=0;
int recovery_mode=0;
const char*values[]={"Linux","Systemd","GNU"};
int auth_enabled=0;
char*adb_external_storage=NULL,*adb_device_banner="device",*log_file=NULL,*log_dir="/var/log/adbd";
void fatal(const char*fmt,...){
	va_list ap;
	va_start(ap,fmt);
	vfprintf(stderr,fmt,ap);
	fprintf(stderr,"\n");
	va_end(ap);
	exit(-1);
}
void fatal_errno(const char*fmt,...){
	va_list ap;
	va_start(ap,fmt);
	vfprintf(stderr,fmt,ap);
	fprintf(stderr,": %m\n");
	va_end(ap);
	exit(-1);
}
apacket*get_apacket(void){
	apacket*p=malloc(sizeof(apacket));
	if(p==0)fatal("adbd: failed to allocate an apacket");
	memset(p,0,sizeof(apacket)-MAX_PAYLOAD);
	return p;
}
void put_apacket(apacket*p){free(p);}
void handle_online(atransport*t){printf("adbd: status change to online\n");t->online=1;}
void handle_offline(atransport*t){printf("adbd: status change to offline\n");t->online=0;run_transport_disconnects(t);}
static void send_ready(unsigned local,unsigned remote,atransport*t){
	apacket*p=get_apacket();
	p->msg.command=A_OKAY;
	p->msg.arg0=local;
	p->msg.arg1=remote;
	send_packet(p,t);
}
static void send_close(unsigned local,unsigned remote,atransport*t){
	apacket*p=get_apacket();
	p->msg.command=A_CLSE;
	p->msg.arg0=local;
	p->msg.arg1=remote;
	send_packet(p,t);
}
static size_t fill_connect_data(char*buf,size_t bufsize){
	static const char*cnxn_props[]={"ro.product.name","ro.product.model","ro.product.device",NULL};
	size_t remaining=bufsize;
	size_t len;
	len=snprintf(buf,remaining,"%s::",adb_device_banner);
	remaining-=len;
	buf+=len;
	for(int i=0;i<3;i++){
		len=snprintf(buf,remaining,"%s=%s;",cnxn_props[i],values[i]);
		remaining-=len;
		buf+=len;
	}
	return bufsize-remaining+1;
}
static void send_connect(atransport*t){
	apacket*cp=get_apacket();
	cp->msg.command=A_CNXN;
	cp->msg.arg0=A_VERSION;
	cp->msg.arg1=MAX_PAYLOAD;
	cp->msg.data_length=fill_connect_data((char*)cp->data,sizeof(cp->data));
	send_packet(cp,t);
}
static void send_auth_request(atransport*t){
	apacket*p;
	int ret;
	if((ret=adb_auth_generate_token(t->token,sizeof(t->token)))!=sizeof(t->token)){
		printf("adbd: error generating token ret=%d\n",ret);
		return;
	}
	p=get_apacket();
	memcpy(p->data,t->token,ret);
	p->msg.command=A_AUTH;
	p->msg.arg0=ADB_AUTH_TOKEN;
	p->msg.data_length=ret;
	send_packet(p,t);
}
static void send_auth_response(uint8_t*token,size_t token_size,atransport*t){
	apacket*p=get_apacket();
	int ret;
	if(!(ret=adb_auth_sign(t->key,token,token_size,p->data))){
		printf("adbd: error signing the token\n");
		put_apacket(p);
		return;
	}
	p->msg.command=A_AUTH;
	p->msg.arg0=ADB_AUTH_SIGNATURE;
	p->msg.data_length=ret;
	send_packet(p,t);
}
static void send_auth_publickey(atransport*t){
	apacket*p=get_apacket();
	int ret;
	if(!(ret=adb_auth_get_userkey(p->data,sizeof(p->data)))){
		printf("adbd: failed to get user public key\n");
		put_apacket(p);
		return;
	}
	p->msg.command=A_AUTH;
	p->msg.arg0=ADB_AUTH_RSAPUBLICKEY;
	p->msg.data_length=ret;
	send_packet(p,t);
}
void adb_auth_verified(atransport*t){
	handle_online(t);
	send_connect(t);
}
static char*connection_state_name(atransport*t){
	if(t==NULL)return "unknown";
	switch(t->connection_state){
		case CS_BOOTLOADER:return "bootloader";
		case CS_DEVICE:return "device";
		case CS_OFFLINE:return "offline";
		default:return "unknown";
	}
}
static void qual_overwrite(char**dst,const char*src){
	if(!dst)return;
	free(*dst);
	*dst=NULL;
	if(!src||!*src)return;
	*dst=strdup(src);
}
void parse_banner(char*banner,atransport*t){
	static const char*prop_seps=";",key_val_sep='=';
	char*cp,*type;
	type=banner;
	if((cp=strchr(type,':'))){
		*cp++=0;
		if((cp=strchr(cp,':'))){
			char*save,*key;
			if((key=adb_strtok_r(cp + 1,prop_seps,&save)))do{
				if((cp=strchr(key,key_val_sep))){
					*cp++='\0';
					if(!strcmp(key,"ro.product.name"))qual_overwrite(&t->product,cp);
					else if(!strcmp(key,"ro.product.model"))qual_overwrite(&t->model,cp);
					else if(!strcmp(key,"ro.product.device"))qual_overwrite(&t->device,cp);
				}
			}while((key=adb_strtok_r(NULL,prop_seps,&save)));
		}
	}
	if(!strcmp(type,"bootloader")){
		printf("adbd: setting connection_state to CS_BOOTLOADER\n");
		t->connection_state=CS_BOOTLOADER;
		update_transports();
		return;
	}
	if(!strcmp(type,"device")){
		printf("adbd: setting connection_state to CS_DEVICE\n");
		t->connection_state=CS_DEVICE;
		update_transports();
		return;
	}
	if(!strcmp(type,"recovery")){
		printf("adbd: setting connection_state to CS_RECOVERY\n");
		t->connection_state=CS_RECOVERY;
		update_transports();
		return;
	}
	if(!strcmp(type,"sideload")){
		printf("adbd: setting connection_state to CS_SIDELOAD\n");
		t->connection_state=CS_SIDELOAD;
		update_transports();
		return;
	}
	t->connection_state=CS_HOST;
}
void handle_packet(apacket*p,atransport*t){
	asocket*s;
	print_packet("recv",p);
	switch(p->msg.command){
		case A_SYNC:
			if(p->msg.arg0){
				send_packet(p,t);
				if(HOST)send_connect(t);
			}else{
				t->connection_state=CS_OFFLINE;
				handle_offline(t);
				send_packet(p,t);
			}
		return;
		case A_CNXN:
			if(t->connection_state !=CS_OFFLINE){
				t->connection_state=CS_OFFLINE;
				handle_offline(t);
			}
			parse_banner((char*)p->data,t);
			if(HOST||!auth_enabled){
				handle_online(t);
				if(!HOST)send_connect(t);
			}else send_auth_request(t);
		break;
		case A_AUTH:
			if(p->msg.arg0==ADB_AUTH_TOKEN){
				t->key=adb_auth_nextkey(t->key);
				if(t->key)send_auth_response(p->data,p->msg.data_length,t);
				else send_auth_publickey(t);
			}else if(p->msg.arg0==ADB_AUTH_SIGNATURE){
				if(adb_auth_verify(t->token,p->data,p->msg.data_length)){
					adb_auth_verified(t);
					t->failed_auth_attempts=0;
				}else{
					if(t->failed_auth_attempts++>10)adb_sleep_ms(1000);
					send_auth_request(t);
				}
			}else if(p->msg.arg0==ADB_AUTH_RSAPUBLICKEY)adb_auth_confirm_key(p->data,p->msg.data_length,t);
		break;
		case A_OPEN:
			if(!t->online)break;
			char*name=(char*)p->data;
			name[p->msg.data_length>0?p->msg.data_length-1:0]=0;
			if((s=create_local_service_socket(name))==0)send_close(0,p->msg.arg0,t);
			else{
				s->peer=create_remote_socket(p->msg.arg0,t);
				s->peer->peer=s;
				send_ready(s->id,s->peer->id,t);
				s->ready(s);
			}
		break;
		case A_OKAY:
			if(!t->online||!(s=find_local_socket(p->msg.arg1)))break;
			if(s->peer==0){
				s->peer=create_remote_socket(p->msg.arg0,t);
				s->peer->peer=s;
			}
			s->ready(s);
		break;
		case A_CLSE:if(t->online&&(s=find_local_socket(p->msg.arg1)))s->close(s);break;
		case A_WRTE:
			if(!t->online||!(s=find_local_socket(p->msg.arg1)))break;
			unsigned rid=p->msg.arg0;
			p->len=p->msg.data_length;
			if(s->enqueue(s,p)==0)send_ready(s->id,rid,t);
		return;
		default:printf("adbd: handle_packet what is %08x?!\n",p->msg.command);
	}
	put_apacket(p);
}
alistener listener_list={
	.next=&listener_list,
	.prev=&listener_list,
};
static void ss_listener_event_func(int _fd,unsigned ev,void*_l){
	asocket*s;
	if(!(ev&FDE_READ))return;
	struct sockaddr addr;
	socklen_t alen;
	int fd;
	alen=sizeof(addr);
	if((fd=adb_socket_accept(_fd,&addr,&alen))<0)return;
	adb_socket_setbufsize(fd,CHUNK_SIZE);
	if((s=create_local_socket(fd))){
		connect_to_smartsocket(s);
		return;
	}
	adb_close(fd);
}
static void listener_event_func(int _fd,unsigned ev,void*_l){
	alistener*l=_l;
	asocket*s;
	if(!(ev&FDE_READ))return;
	struct sockaddr addr;
	socklen_t alen;
	int fd;
	alen=sizeof(addr);
	if((fd=adb_socket_accept(_fd,&addr,&alen))<0)return;
	if((s=create_local_socket(fd))){
		s->transport=l->transport;
		connect_to_remote(s,l->connect_to);
		return;
	}
	adb_close(fd);
}
static void free_listener(alistener*l){
	if(l->next){
		l->next->prev=l->prev;
		l->prev->next=l->next;
		l->next=l->prev=l;
	}
	fdevent_remove(&l->fde);
	if(l->local_name)free((char*)l->local_name);
	if(l->connect_to)free((char*)l->connect_to);
	if(l->transport)remove_transport_disconnect(l->transport,&l->disconnect);
	free(l);
}
static void listener_disconnect(void*_l,atransport*t){
	alistener*l=_l;
	free_listener(l);
}
int local_name_to_fd(const char*name){
	int port;
	if(!strncmp("tcp:",name,4)){
		port=atoi(name + 4);
		return (gListenAll>0)?socket_inaddr_any_server(port,SOCK_STREAM):socket_loopback_server(port,SOCK_STREAM);
	}
	if(!strncmp(name,"local:",6))return socket_local_server(name+6,ANDROID_SOCKET_NAMESPACE_ABSTRACT,SOCK_STREAM);
	else if(!strncmp(name,"localabstract:",14))return socket_local_server(name+14,ANDROID_SOCKET_NAMESPACE_ABSTRACT,SOCK_STREAM);
	else if(!strncmp(name,"localfilesystem:",16))return socket_local_server(name+16,ANDROID_SOCKET_NAMESPACE_FILESYSTEM,SOCK_STREAM);
	printf("adbd: unknown local portname '%s'\n",name);
	return -1;
}
static int format_listener(alistener*l,char*buffer,size_t buffer_len){
	int local_len=strlen(l->local_name);
	int connect_len=strlen(l->connect_to);
	int serial_len=strlen(l->transport->serial);
	if(buffer!=NULL)snprintf(
		buffer,
		buffer_len,
		"%s %s %s\n",
		l->transport->serial,
		l->local_name,
		l->connect_to
	);
	return local_len+connect_len+serial_len + 3;
}
static int format_listeners(char*buf,size_t buflen){
	alistener*l;
	int result=0;
	for(l=listener_list.next;l!=&listener_list;l=l->next){
		if(l->connect_to[0]=='*')continue;
		int len=format_listener(l,buf,buflen);
		result+=len;
		if(buf!=NULL){
			buf+=len;
			buflen-=len;
			if(buflen<=0)break;
		}
	}
	return result;
}
static int remove_listener(const char*local_name,atransport*transport){
	alistener*l;
	for(l=listener_list.next;l!=&listener_list;l=l->next)
		if(!strcmp(local_name,l->local_name)){
			listener_disconnect(l,l->transport);
			return 0;
		}
	return -1;
}
static void remove_all_listeners(void){
	alistener*l,*l_next;
	for(l=listener_list.next;l!=&listener_list;l=l_next){
		l_next=l->next;
		if(l->connect_to[0]=='*')continue;
		listener_disconnect(l,l->transport);
	}
}
typedef enum {
	INSTALL_STATUS_OK=0,
	INSTALL_STATUS_INTERNAL_ERROR=-1,
	INSTALL_STATUS_CANNOT_BIND=-2,
	INSTALL_STATUS_CANNOT_REBIND=-3,
}install_status_t;
static install_status_t install_listener(const char*local_name,const char*connect_to,atransport*transport,int no_rebind){
	alistener*l;
	for(l=listener_list.next;l!=&listener_list; l=l->next){
		if(strcmp(local_name,l->local_name)==0){
			char*cto;
			if(l->connect_to[0]=='*')return INSTALL_STATUS_INTERNAL_ERROR;
			if(no_rebind)return INSTALL_STATUS_CANNOT_REBIND;
			cto=strdup(connect_to);
			if(cto==0)return INSTALL_STATUS_INTERNAL_ERROR;
			free((void*)l->connect_to);
			l->connect_to=cto;
			if(l->transport!=transport){
				remove_transport_disconnect(l->transport,&l->disconnect);
				l->transport=transport;
				add_transport_disconnect(l->transport,&l->disconnect);
			}
			return INSTALL_STATUS_OK;
		}
	}
	if((l=calloc(1,sizeof(alistener)))==0)goto nomem;
	if((l->local_name=strdup(local_name))==0)goto nomem;
	if((l->connect_to=strdup(connect_to))==0)goto nomem;
	if((l->fd=local_name_to_fd(local_name))<0){
		free((void*)l->local_name);
		free((void*)l->connect_to);
		free(l);
		printf("adbd: cannot bind '%s'\n",local_name);
		return -2;
	}
	close_on_exec(l->fd);
	if(!strcmp(l->connect_to,"*smartsocket*"))fdevent_install(&l->fde,l->fd,ss_listener_event_func,l);
	else fdevent_install(&l->fde,l->fd,listener_event_func,l);
	fdevent_set(&l->fde,FDE_READ);
	l->next=&listener_list;
	l->prev=listener_list.prev;
	l->next->prev=l;
	l->prev->next=l;
	l->transport=transport;
	if(transport){
		l->disconnect.opaque=l;
		l->disconnect.func =listener_disconnect;
		add_transport_disconnect(transport,&l->disconnect);
	}
	return INSTALL_STATUS_OK;
nomem:
	fatal("adbd: cannot allocate listener");
	return INSTALL_STATUS_INTERNAL_ERROR;
}
static void adb_cleanup(void){usb_cleanup();}
int start_device_log(void){
	if(log_file!=NULL&&strcmp(log_file,"-")==0)return 0;
	int fd;
	struct tm now;
	time_t t;
	if(!log_file){
		adb_mkdir(log_dir,0775);
		tzset();
		time(&t);
		localtime_r(&t,&now);
		char*tm=malloc(64*sizeof(char*));
		memset(tm,0,64*sizeof(char*));
		strftime(tm,63,"%Y-%m-%d-%H-%M-%S",&now);
		log_file=malloc(PATH_MAX*sizeof(char*));
		memset(log_file,0,PATH_MAX*sizeof(char*));
		snprintf(log_file,PATH_MAX,"%s/adb-%s.log",log_dir,tm);
	}
	if((fd=unix_open(log_file,O_WRONLY|O_CREAT|O_TRUNC,0640))<0){
		perror("adbd: cannot open log file");
		return -1;
	}
	dup2(fd,1);
	dup2(fd,2);
	fprintf(stderr,"--- adb starting(pid %d)---\n",getpid());
	adb_close(fd);
	fd=unix_open("/dev/null",O_RDONLY);
	dup2(fd,0);
	adb_close(fd);
	return 0;
}
void build_local_name(char*target_str,size_t target_size,int server_port){snprintf(target_str,target_size,"tcp:%d",server_port);}
int adb_main(int force_usb,int server_port){
	umask(000);
	atexit(adb_cleanup);
	signal(SIGPIPE,SIG_IGN);
	init_transport_registration();
	if(auth_enabled)adb_auth_init();
	if(NULL!=adb_external_storage)setenv("EXTERNAL_STORAGE",adb_external_storage,1);
	else printf("adbd: ADB_EXTERNAL_STORAGE is not set. Leaving EXTERNAL_STORAGE unchanged.\n");
	char local_name[30];
	build_local_name(local_name,sizeof(local_name),server_port);
	if(install_listener(local_name,"*smartsocket*",NULL,0))exit(1);
	int usb=0;
	if(force_usb!=2&&(access(USB_ADB_PATH,F_OK)==0||access(USB_FFS_ADB_EP0,F_OK)==0)){
		printf("adbd: using USB.\n");
		usb_init();
		usb=1;
	}
	if(force_usb==1&&usb!=1){
		printf("adbd: failed to open USB\n");
		return 1;
	}else if(usb!=1){
		printf("adbd: using Local Port.\n");
		local_init(DEFAULT_ADB_LOCAL_TRANSPORT_PORT);
	}
	init_jdwp();
	fdevent_loop();
	usb_cleanup();
	return 0;
}
int handle_host_request(char*service,transport_type ttype,char*serial,int reply_fd,asocket*s){
	atransport*transport=NULL;
	char buf[4096];
	if(!strcmp(service,"kill")){
		fprintf(stderr,"adbd: adb server killed by remote request\n");
		fflush(stdout);
		adb_write(reply_fd,"OKAY",4);
		usb_cleanup();
		exit(0);
	}
	if(!strcmp(service,"list-forward")){
		char header[9];
		int buffer_size=format_listeners(NULL,0);
		char*buffer=malloc(buffer_size+1);
		(void)format_listeners(buffer,buffer_size+1);
		snprintf(header,sizeof header,"OKAY%04x",buffer_size);
		writex(reply_fd,header,8);
		writex(reply_fd,buffer,buffer_size);
		free(buffer);
		return 0;
	}
	if(!strcmp(service,"killforward-all")){
		remove_all_listeners();
		adb_write(reply_fd,"OKAYOKAY",8);
		return 0;
	}
	if(!strncmp(service,"forward:",8)||!strncmp(service,"killforward:",12)){
		char*local,*remote,*err;
		int r;
		atransport*transport;
		int createForward=strncmp(service,"kill",4);
		int no_rebind=0;
		local=strchr(service,':')+1;
		if(createForward&&!strncmp(local,"norebind:",9)){
			no_rebind=1;
			local=strchr(local,':')+ 1;
		}
		remote=strchr(local,';');
		if(createForward){
			if(remote==0){
				sendfailmsg(reply_fd,"malformed forward spec");
				return 0;
			}
			*remote++=0;
			if((local[0]==0)||(remote[0]==0)||(remote[0]=='*')){
				sendfailmsg(reply_fd,"malformed forward spec");
				return 0;
			}
		}else if(local[0]==0){
			sendfailmsg(reply_fd,"malformed forward spec");
			return 0;
		}
		if(!(transport=acquire_one_transport(CS_ANY,ttype,serial,&err))){
			sendfailmsg(reply_fd,err);
			return 0;
		}
		if((r=createForward?install_listener(local,remote,transport,no_rebind):remove_listener(local,transport))==0){
			writex(reply_fd,"OKAYOKAY",8);
			return 0;
		}
		if(createForward)switch(r){
			case INSTALL_STATUS_CANNOT_BIND:sendfailmsg(reply_fd,"cannot bind to socket");break;
			case INSTALL_STATUS_CANNOT_REBIND:sendfailmsg(reply_fd,"cannot rebind existing socket");break;
			default:sendfailmsg(reply_fd,"internal error");
		}else sendfailmsg(reply_fd,"cannot remove listener");
		return 0;
	}
	if(!strncmp(service,"get-state",strlen("get-state"))){
		transport=acquire_one_transport(CS_ANY,ttype,serial,NULL);
		char*state=connection_state_name(transport);
		snprintf(buf,sizeof buf,"OKAY%04x%s",(unsigned)strlen(state),state);
		writex(reply_fd,buf,strlen(buf));
		return 0;
	}
	return -1;
}
