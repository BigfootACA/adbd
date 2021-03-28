#ifndef __LINUX_FUNCTIONFS_H__
#define __LINUX_FUNCTIONFS_H__ 1
#include<linux/types.h>
#include<linux/ioctl.h>
#include<linux/usb/ch9.h>
enum{
	FUNCTIONFS_DESCRIPTORS_MAGIC=1,
	FUNCTIONFS_STRINGS_MAGIC=2
};
struct usb_endpoint_descriptor_no_audio{
	__u8  bLength,bDescriptorType,bEndpointAddress,bmAttributes;
	__le16 wMaxPacketSize;
	__u8  bInterval;
}__attribute__((packed));
struct usb_functionfs_descs_head{__le32 magic,length,fs_count,hs_count;}__attribute__((packed));
struct usb_functionfs_strings_head{__le32 magic,length,str_count,lang_count;}__attribute__((packed));
enum usb_functionfs_event_type {
	FUNCTIONFS_BIND,FUNCTIONFS_UNBIND,
	FUNCTIONFS_ENABLE,FUNCTIONFS_DISABLE,
	FUNCTIONFS_SETUP,
	FUNCTIONFS_SUSPEND,FUNCTIONFS_RESUME
};
struct usb_functionfs_event{
	union{struct usb_ctrlrequest setup;}__attribute__((packed))u;
	__u8 type,_pad[3];
}__attribute__((packed));
#define	FUNCTIONFS_FIFO_STATUS	_IO('g', 1)
#define	FUNCTIONFS_FIFO_FLUSH	_IO('g', 2)
#define	FUNCTIONFS_CLEAR_HALT	_IO('g', 3)
#define	FUNCTIONFS_INTERFACE_REVMAP	_IO('g', 128)
#define	FUNCTIONFS_ENDPOINT_REVMAP	_IO('g', 129)
#endif
