#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/usb/ch9.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include "functionfs.h"
#include "sysdeps.h"
#define TRACE_TAG TRACE_USB
#include "adb.h"
#define MAX_PACKET_SIZE_FS 64
#define MAX_PACKET_SIZE_HS 512
struct usb_handle{
	adb_cond_t notify;
	adb_mutex_t lock;
	int (*write)(usb_handle*h,const void*data,int len);
	int (*read)(usb_handle*h,void*data,int len);
	void (*kick)(usb_handle*h);
	int fd,control,bulk_out,bulk_in;
};
static const struct{
	struct usb_functionfs_descs_head header;
	struct{
		struct usb_interface_descriptor intf;
		struct usb_endpoint_descriptor_no_audio source,sink;
	}__attribute__((packed))fs_descs,hs_descs;
}__attribute__((packed)) descriptors={
	.header={
		.magic=FUNCTIONFS_DESCRIPTORS_MAGIC,
		.length=sizeof(descriptors),
		.fs_count=3,
		.hs_count=3,
	},
	.fs_descs={
		.intf={
			.bLength=sizeof(descriptors.fs_descs.intf),
			.bDescriptorType=USB_DT_INTERFACE,
			.bInterfaceNumber=0,
			.bNumEndpoints=2,
			.bInterfaceClass=ADB_CLASS,
			.bInterfaceSubClass=ADB_SUBCLASS,
			.bInterfaceProtocol=ADB_PROTOCOL,
			.iInterface=1,
		},
		.source={
			.bLength=sizeof(descriptors.fs_descs.source),
			.bDescriptorType=USB_DT_ENDPOINT,
			.bEndpointAddress=1|USB_DIR_OUT,
			.bmAttributes=USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize=MAX_PACKET_SIZE_FS,
		},
		.sink={
			.bLength=sizeof(descriptors.fs_descs.sink),
			.bDescriptorType=USB_DT_ENDPOINT,
			.bEndpointAddress=2|USB_DIR_IN,
			.bmAttributes=USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize=MAX_PACKET_SIZE_FS,
		},
	},
	.hs_descs={
		.intf={
			.bLength=sizeof(descriptors.hs_descs.intf),
			.bDescriptorType=USB_DT_INTERFACE,
			.bInterfaceNumber=0,
			.bNumEndpoints=2,
			.bInterfaceClass=ADB_CLASS,
			.bInterfaceSubClass=ADB_SUBCLASS,
			.bInterfaceProtocol=ADB_PROTOCOL,
			.iInterface=1,
		},
		.source={
			.bLength=sizeof(descriptors.hs_descs.source),
			.bDescriptorType=USB_DT_ENDPOINT,
			.bEndpointAddress=1|USB_DIR_OUT,
			.bmAttributes=USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize=MAX_PACKET_SIZE_HS,
		},
		.sink={
			.bLength=sizeof(descriptors.hs_descs.sink),
			.bDescriptorType=USB_DT_ENDPOINT,
			.bEndpointAddress=2|USB_DIR_IN,
			.bmAttributes=USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize=MAX_PACKET_SIZE_HS,
		},
	},
};
#define STR_INTERFACE_ "ADB Interface"
static const struct{
	struct usb_functionfs_strings_head header;
	struct{
		__le16 code;
		const char str1[sizeof(STR_INTERFACE_)];
	}__attribute__((packed))lang0;
}__attribute__((packed))strings={
	.header={
		.magic=FUNCTIONFS_STRINGS_MAGIC,
		.length=sizeof(strings),
		.str_count=1,
		.lang_count=1,
	},
	.lang0={
		0x0409,
		STR_INTERFACE_,
	},
};
static void*usb_adb_open_thread(void*x){
	struct usb_handle*usb=(struct usb_handle*)x;
	int fd;
	while(1){
		adb_mutex_lock(&usb->lock);
		while(usb->fd!=-1)adb_cond_wait(&usb->notify,&usb->lock);
		adb_mutex_unlock(&usb->lock);
		do{
			fd=unix_open("/dev/android_adb",O_RDWR);
			if(fd<0)fd=unix_open("/dev/android",O_RDWR);
			if(fd<0)adb_sleep_ms(1000);
		}while(fd<0);
		close_on_exec(fd);
		usb->fd=fd;
		register_usb_transport(usb,0,0,1);
	}
	return 0;
}
static int usb_adb_write(usb_handle*h,const void*data,int len){
	int n;
	if((n=adb_write(h->fd,data,len))!=len){
		printf("adbd: usb fd %d write %d: %m\n",h->fd,n);
		return -1;
	}
	return 0;
}
static int usb_adb_read(usb_handle*h,void*data,int len){
	int n;
	if((n=adb_read(h->fd,data,len))!=len) {
		printf("adbd: usb fd %d read %d: %m\n",h->fd,n);
		return -1;
	}
	return 0;
}
static void usb_adb_kick(usb_handle*h){
	adb_mutex_lock(&h->lock);
	adb_close(h->fd);
	h->fd=-1;
	adb_cond_signal(&h->notify);
	adb_mutex_unlock(&h->lock);
}
static void usb_adb_init(){
	usb_handle*h;
	adb_thread_t tid;
	int fd;
	h=calloc(1,sizeof(usb_handle));
	h->write=usb_adb_write;
	h->read=usb_adb_read;
	h->kick=usb_adb_kick;
	h->fd=-1;
	adb_cond_init(&h->notify,0);
	adb_mutex_init(&h->lock,0);
	if((fd=unix_open("/dev/android_adb_enable",O_RDWR))>=0)close_on_exec(fd);
	if(adb_thread_create(&tid,usb_adb_open_thread,h))fatal_errno("adbd: cannot create usb thread");
}
static void init_functionfs(struct usb_handle*h){
	ssize_t ret;
	if((h->control=adb_open(USB_FFS_ADB_EP0,O_RDWR))<0) {
		printf("adbd: %s cannot open control endpoint: %m\n",USB_FFS_ADB_EP0);
		goto err;
	}
	if((ret=adb_write(h->control,&descriptors,sizeof(descriptors)))<0){
		printf("adbd: %s write descriptors failed: %m\n",USB_FFS_ADB_EP0);
		goto err;
	}
	if((ret=adb_write(h->control,&strings,sizeof(strings)))<0){
		printf("adbd: %s writing strings failed: %m\n",USB_FFS_ADB_EP0);
		goto err;
	}
	if((h->bulk_out=adb_open(USB_FFS_ADB_OUT,O_RDWR))<0){
		printf("adbd: %s cannot open bulk-out ep: %m\n",USB_FFS_ADB_OUT);
		goto err;
	}
	h->bulk_in=adb_open(USB_FFS_ADB_IN,O_RDWR);
	if(h->bulk_in<0)goto err;
	return;
err:
	if(h->bulk_in>0){adb_close(h->bulk_in);h->bulk_in=-1;}
	if(h->bulk_out>0){adb_close(h->bulk_out);h->bulk_out=-1;}
	if(h->control>0){adb_close(h->control);h->control=-1;}
	return;
}
static void*usb_ffs_open_thread(void*x){
	struct usb_handle*usb=(struct usb_handle*)x;
	while(1){
		adb_mutex_lock(&usb->lock);
		while(usb->control!=-1)adb_cond_wait(&usb->notify,&usb->lock);
		adb_mutex_unlock(&usb->lock);
		while(1){
			init_functionfs(usb);
			if(usb->control>=0)break;
			adb_sleep_ms(1000);
		}
		register_usb_transport(usb,0,0,1);
	}
	return 0;
}
static int bulk_write(int bulk_in,const char*buf,size_t length){
	size_t count=0;
	int ret;
	do{
		if((ret=adb_write(bulk_in,buf+count,length-count))>=0)count+=ret;
		else if(errno!=EINTR)return ret;
	}while(count<length);
	return count;
}
static int usb_ffs_write(usb_handle*h,const void*data,int len){
	int n;
	if((n=bulk_write(h->bulk_in,data,len))==len)return 0;
	printf("adbd: fd %d write %d: %m\n",h->bulk_out,n);
	return -1;
}
static int bulk_read(int bulk_out,char*buf,size_t length){
	size_t count=0;
	int ret;
	do{
		if((ret=adb_read(bulk_out,buf+count,length-count))>=0)count+=ret;
		else if(errno!=EINTR){
			printf("adbd: bulk read failed fd %d length %ld count %ld: %m\n",bulk_out,length,count);
			return ret;
		}
	}while(count<length);
	return count;
}
static int usb_ffs_read(usb_handle*h,void*data,int len){
	int n;
	if((n=bulk_read(h->bulk_out,data,len))==len)return 0;
	printf("adbd: fd %d read %d: %m\n",h->bulk_out,n);
	return -1;
}
static void usb_ffs_kick(usb_handle*h){
	if(ioctl(h->bulk_in,FUNCTIONFS_CLEAR_HALT)<0)printf("adbd: usb_ffs_kick source fd %d clear halt failed: %m\n",h->bulk_in);
	if(ioctl(h->bulk_out,FUNCTIONFS_CLEAR_HALT)<0)printf("adbd: usb_ffs_kick sink fd %d clear halt failed: %m\n",h->bulk_out);
	adb_mutex_lock(&h->lock);
	adb_close(h->control);
	adb_close(h->bulk_out);
	adb_close(h->bulk_in);
	h->control=h->bulk_out=h->bulk_in=-1;
	adb_cond_signal(&h->notify);
	adb_mutex_unlock(&h->lock);
}
static void usb_ffs_init(){
	usb_handle*h;
	adb_thread_t tid;
	h=calloc(1,sizeof(usb_handle));
	h->write=usb_ffs_write;
	h->read=usb_ffs_read;
	h->kick=usb_ffs_kick;
	h->control =-1;
	h->bulk_out=-1;
	h->bulk_out=-1;
	adb_cond_init(&h->notify,0);
	adb_mutex_init(&h->lock,0);
	if(adb_thread_create(&tid,usb_ffs_open_thread,h))fatal_errno("adbd: cannot create usb thread\n");
}
void usb_init(){if(access(USB_FFS_ADB_EP0,F_OK)==0)usb_ffs_init();else usb_adb_init();}
void usb_cleanup(){}
int usb_write(usb_handle*h,const void*data,int len){return h->write(h,data,len);}
int usb_read(usb_handle*h,void*data,int len){return h->read(h,data,len);}
int usb_close(usb_handle*h){return 0;}
void usb_kick(usb_handle*h){h->kick(h);}
