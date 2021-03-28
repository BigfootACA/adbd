#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"sysdeps.h"
#define TRACE_TAG TRACE_TRANSPORT
#include"adb.h"
static int remote_read(apacket*p,atransport*t){
	if(usb_read(t->usb,&p->msg,sizeof(amessage)))return -1;
	if(check_header(p))return -1;
	if(p->msg.data_length&&usb_read(t->usb,p->data,p->msg.data_length))return -1;
	if(check_data(p))return -1;
	return 0;
}
static int remote_write(apacket*p,atransport*t){
	unsigned size=p->msg.data_length;
	if(usb_write(t->usb,&p->msg,sizeof(amessage)))return -1;
	if(p->msg.data_length==0)return 0;
	if(usb_write(t->usb,&p->data,size))return -1;
	return 0;
}
static void remote_close(atransport*t){usb_close(t->usb);t->usb=0;}
static void remote_kick(atransport*t){usb_kick(t->usb);}
void init_usb_transport(atransport*t,usb_handle*h,int state){
	t->close=remote_close;
	t->kick=remote_kick;
	t->read_from_remote=remote_read;
	t->write_to_remote=remote_write;
	t->sync_token=1;
	t->connection_state=state;
	t->type=kTransportUsb;
	t->usb=h;
	HOST=0;
}
