#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "sysdeps.h"
#define TRACE_TAG TRACE_TRANSPORT
#include "adb.h"
static void transport_unref(atransport*t);
static atransport transport_list={.next=&transport_list,.prev=&transport_list,};
ADB_MUTEX_DEFINE(transport_lock);
void kick_transport(atransport*t){
	if(t&&!t->kicked){
		int kicked;
		adb_mutex_lock(&transport_lock);
		kicked=t->kicked;
		if(!kicked)t->kicked=1;
		adb_mutex_unlock(&transport_lock);
		if(!kicked)t->kick(t);
	}
}
void run_transport_disconnects(atransport*t){
	adisconnect*dis=t->disconnects.next;
	while(dis!=&t->disconnects){
		adisconnect*next=dis->next;
		dis->func(dis->opaque,t);
		dis=next;
	}
}
static int read_packet(int fd,const char*name,apacket**ppacket){
	char*p=(char*)ppacket;
	int r,len=sizeof(*ppacket);
	errno=0;
	while(len>0)if((r=adb_read(fd,p,len))>0){
		len-=r;
		p+=r;
	}else{
		if((r<0)&&(errno==EINTR))continue;
		return -1;
	}
	return 0;
}
static int write_packet(int fd,const char*name,apacket**ppacket){
	char*p=(char*)ppacket;
	int r,len=sizeof(ppacket);
	errno=0;
	len=sizeof(ppacket);
	while(len>0)if((r=adb_write(fd,p,len))>0){
		len-=r;
		p+=r;
	}else{
		if((r<0)&&(errno==EINTR))continue;
		return -1;
	}
	return 0;
}
static void transport_socket_events(int fd,unsigned events,void*_t){
	atransport*t=_t;
	if(events&FDE_READ){
		apacket*p=0;
		if(!read_packet(fd,t->serial,&p))handle_packet(p,(atransport*)_t);
	}
}
void send_packet(apacket*p,atransport*t){
	if(!t||!p)return;
	unsigned char*x;
	unsigned sum,count;
	p->msg.magic=p->msg.command^0xffffffff;
	count=p->msg.data_length;
	x=(unsigned char*)p->data;
	sum=0;
	while(count-->0)sum+=*x++;
	p->msg.data_check=sum;
	print_packet("send",p);
	if(write_packet(t->transport_socket,t->serial,&p))fatal_errno("adbd: cannot enqueue packet on transport socket");
}
static void*output_thread(void*_t){
	atransport*t=_t;
	apacket*p;
	p=get_apacket();
	p->msg.command=A_SYNC;
	p->msg.arg0=1;
	p->msg.arg1=++(t->sync_token);
	p->msg.magic=A_SYNC ^ 0xffffffff;
	if(write_packet(t->fd,t->serial,&p)){
		put_apacket(p);
		goto oops;
	}
	for(;;){
		if(t->read_from_remote((p=get_apacket()),t)==0){
			if(write_packet(t->fd,t->serial,&p)){
				put_apacket(p);
				goto oops;
			}
		}else{
			put_apacket(p);
			break;
		}
	}
	p=get_apacket();
	p->msg.command=A_SYNC;
	p->msg.arg0=0;
	p->msg.arg1=0;
	p->msg.magic=A_SYNC ^ 0xffffffff;
	if(write_packet(t->fd,t->serial,&p))put_apacket(p);
oops:
	kick_transport(t);
	transport_unref(t);
	return 0;
}
static void*input_thread(void*_t){
	atransport*t=_t;
	apacket*p;
	int active=0;
	for(;;){
		if(read_packet(t->fd,t->serial,&p))break;
		if(p->msg.command==A_SYNC){
			if(p->msg.arg0==0){
				put_apacket(p);
				break;
			}else if(p->msg.arg1==t->sync_token)active=1;
		}else if(active)t->write_to_remote(p,t);
		put_apacket(p);
	}
	close_all_sockets(t);
	kick_transport(t);
	transport_unref(t);
	return 0;
}
static int transport_registration_send=-1,transport_registration_recv=-1;
static fdevent transport_registration_fde;
void  update_transports(void){}
typedef struct tmsg tmsg;
struct tmsg{atransport*transport;int action;};
static int transport_read_action(int fd,struct tmsg*m){
	char*p=(char*)m;
	int len=sizeof(*m),r;
	while(len>0)if((r=adb_read(fd,p,len))>0){
		len-=r;
		p+=r;
	}else{
		if((r<0)&&(errno==EINTR))continue;
		return -1;
	}
	return 0;
}
static int transport_write_action(int fd,struct tmsg*m){
	char*p=(char*)m;
	int len=sizeof(*m),r;
	while(len>0)if((r=adb_write(fd,p,len))>0){
		len-=r;
		p+=r;
	}else{
		if((r<0)&&(errno==EINTR))continue;
		return -1;
	}
	return 0;
}
static void transport_registration_func(int _fd,unsigned ev,void*data){
	(void)data;
	tmsg m;
	adb_thread_t output_thread_ptr,input_thread_ptr;
	int s[2];
	atransport*t;
	if(!(ev&FDE_READ))return;
	if(transport_read_action(_fd,&m))fatal_errno("adbd: cannot read transport registration socket");
	t=m.transport;
	if(m.action==0){
		fdevent_remove(&(t->transport_fde));
		adb_close(t->fd);
		adb_mutex_lock(&transport_lock);
		t->next->prev=t->prev;
		t->prev->next=t->next;
		adb_mutex_unlock(&transport_lock);
		run_transport_disconnects(t);
		if(t->product)free(t->product);
		if(t->serial)free(t->serial);
		if(t->model)free(t->model);
		if(t->device)free(t->device);
		if(t->devpath)free(t->devpath);
		memset(t,0xee,sizeof(atransport));
		free(t);
		update_transports();
		return;
	}
	if(t->connection_state!=CS_NOPERM){
		t->ref_count=2;
		if(adb_socketpair(s))fatal_errno("adbd: cannot open transport socketpair");
		t->transport_socket=s[0];
		t->fd=s[1];
		fdevent_install(&(t->transport_fde),t->transport_socket,transport_socket_events,t);
		fdevent_set(&(t->transport_fde),FDE_READ);
		if(adb_thread_create(&input_thread_ptr,input_thread,t))fatal_errno("adbd: cannot create input thread");
		if(adb_thread_create(&output_thread_ptr,output_thread,t))fatal_errno("adbd: cannot create output thread");
	}
	adb_mutex_lock(&transport_lock);
	t->next=&transport_list;
	t->prev=transport_list.prev;
	t->next->prev=t;
	t->prev->next=t;
	adb_mutex_unlock(&transport_lock);
	t->disconnects.next=t->disconnects.prev=&t->disconnects;
	update_transports();
}
void init_transport_registration(void){
	int s[2];
	if(adb_socketpair(s))fatal_errno("adbd: cannot open transport registration socketpair");
	transport_registration_send=s[0];
	transport_registration_recv=s[1];
	fdevent_install(&transport_registration_fde,transport_registration_recv,transport_registration_func,0);
	fdevent_set(&transport_registration_fde,FDE_READ);
}
static void register_transport(atransport*transport){
	tmsg m;
	m.transport=transport;
	m.action=1;
	if(transport_write_action(transport_registration_send,&m))fatal_errno("adbd: cannot write transport registration socket\n");
}
static void remove_transport(atransport*transport){
	tmsg m;
	m.transport=transport;
	m.action=0;
	if(transport_write_action(transport_registration_send,&m))fatal_errno("adbd: cannot write transport registration socket\n");
}
static void transport_unref_locked(atransport*t){
	t->ref_count--;
	if(t->ref_count==0){
		if(!t->kicked){
			t->kicked=1;
			t->kick(t);
		}
		t->close(t);
		remove_transport(t);
	}
}
static void transport_unref(atransport*t){
	if(!t)return;
	adb_mutex_lock(&transport_lock);
	transport_unref_locked(t);
	adb_mutex_unlock(&transport_lock);
}
void add_transport_disconnect(atransport*t,adisconnect*dis){
	adb_mutex_lock(&transport_lock);
	dis->next=&t->disconnects;
	dis->prev=dis->next->prev;
	dis->prev->next=dis;
	dis->next->prev=dis;
	adb_mutex_unlock(&transport_lock);
}
void remove_transport_disconnect(atransport*t,adisconnect*dis){
	(void)t;
	dis->prev->next=dis->next;
	dis->next->prev=dis->prev;
	dis->next=dis->prev=dis;
}
static int qual_char_is_invalid(char ch){
	if('A'<=ch&&ch<='Z')return 0;
	if('a'<=ch&&ch<='z')return 0;
	if('0'<=ch&&ch<='9')return 0;
	return 1;
}
static int qual_match(const char*to_test,const char*prefix,const char*qual,int sanitize_qual){
	if(!to_test||!*to_test)return!qual||!*qual;
	if(!qual)return 0;
	if(prefix)while(*prefix)if(*prefix++!=*to_test++)return 0;
	while(*qual){
		char ch=*qual++;
		if(sanitize_qual&&qual_char_is_invalid(ch))ch='_';
		if(ch!=*to_test++)return 0;
	}
	return !*to_test;
}
atransport*acquire_one_transport(int state,transport_type ttype,const char*serial,char**error_out){
	atransport*t,*result=NULL;
	int ambiguous=0;
retry:
	if(error_out)*error_out="device not found";
	adb_mutex_lock(&transport_lock);
	for(t=transport_list.next;t!=&transport_list;t=t->next){
		if(t->connection_state==CS_NOPERM){
			if(error_out)*error_out="insufficient permissions for device";
			continue;
		}
		if(serial){
			if((
				t->serial&&
				!strcmp(serial,t->serial))||
				(t->devpath&&!strcmp(serial,t->devpath))||
				qual_match(serial,"product:",t->product,0)||
				qual_match(serial,"model:",t->model,1)||
				qual_match(serial,"device:",t->device,0)
			){
				if(result){
					if(error_out)*error_out="more than one device";
					ambiguous=1;
					result=NULL;
					break;
				}
				result=t;
			}
		}else if(ttype==kTransportUsb&&t->type==kTransportUsb){
			if(result){
				if(error_out)*error_out="more than one device";
				ambiguous=1;
				result=NULL;
				break;
			}
			result=t;
		}else if(ttype==kTransportLocal&&t->type==kTransportLocal){
			if(result){
				if(error_out)*error_out="more than one emulator";
				ambiguous=1;
				result=NULL;
				break;
			}
			result=t;
		}else if(ttype==kTransportAny){
			if(result){
				if(error_out)*error_out="more than one device and emulator";
				ambiguous=1;
				result=NULL;
				break;
			}
			result=t;
		}
	}
	adb_mutex_unlock(&transport_lock);
	if(result){
		if(result&&result->connection_state==CS_OFFLINE){
			if(error_out)*error_out="device offline";
			result=NULL;
		}
		if(result&&state!=CS_ANY&&result->connection_state!=state){
			if(error_out)*error_out="invalid device state";
			result=NULL;
		}
	}
	if(result){
		if(error_out)*error_out=NULL;
	}else if(state!=CS_ANY&&(serial||!ambiguous)){
		adb_sleep_ms(1000);
		goto retry;
	}
	return result;
}
void register_socket_transport(int s,const char*serial,int port,int local){
	atransport*t=calloc(1,sizeof(atransport));
	char buff[32];
	if(!serial){
		snprintf(buff,sizeof buff,"T-%p",t);
		serial=buff;
	}
	if(init_socket_transport(t,s,port,local)<0){
		adb_close(s);
		free(t);
		return;
	}
	if(serial)t->serial=strdup(serial);
	register_transport(t);
}
void register_usb_transport(usb_handle*usb,const char*serial,const char*devpath,unsigned writeable){
	atransport*t=calloc(1,sizeof(atransport));
	init_usb_transport(t,usb,(writeable ? CS_OFFLINE : CS_NOPERM));
	if(serial)t->serial=strdup(serial);
	if(devpath)t->devpath=strdup(devpath);
	register_transport(t);
}
void unregister_usb_transport(usb_handle*usb){
	atransport*t;
	adb_mutex_lock(&transport_lock);
	for(t=transport_list.next;t!=&transport_list;t=t->next)if(t->usb==usb&&t->connection_state==CS_NOPERM){
		t->next->prev=t->prev;
		t->prev->next=t->next;
		break;
	}
	adb_mutex_unlock(&transport_lock);
}
#undef TRACE_TAG
#define TRACE_TAG  TRACE_RWX
int readx(int fd,void*ptr,size_t len){
	char*p=ptr;
	int r;
	while(len>0)if((r=adb_read(fd,p,len))>0){
		len-=r;
		p+=r;
	}else{
		if(r<0&&errno==EINTR)continue;
		return -1;
	}
	return 0;
}
int writex(int fd,const void*ptr,size_t len){
	char*p=(char*)ptr;
	int r;
	while(len>0)if((r=adb_write(fd,p,len))>0){
		len-=r;
		p+=r;
	}else{
		if(r<0&&errno==EINTR)continue;
		return -1;
	}
	return 0;
}
int check_header(apacket*p){
	if(p->msg.magic!=(p->msg.command^0xffffffff))return -1;
	if(p->msg.data_length>MAX_PAYLOAD)return -1;
	return 0;
}
int check_data(apacket*p){
	unsigned count,sum;
	unsigned char*x;
	count=p->msg.data_length;
	x=p->data;
	sum=0;
	while(count-->0)sum +=*x++;
	return(sum !=p->msg.data_check)?-1:0;
}
