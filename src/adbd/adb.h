#ifndef __ADB_H
#define __ADB_H
#include<limits.h>
#include"transport.h"
#define MAX_PAYLOAD 4096
#define A_SYNC 0x434e5953
#define A_CNXN 0x4e584e43
#define A_OPEN 0x4e45504f
#define A_OKAY 0x59414b4f
#define A_CLSE 0x45534c43
#define A_WRTE 0x45545257
#define A_AUTH 0x48545541
#define A_VERSION 0x01000000
#define ADB_VERSION_MAJOR 1
#define ADB_VERSION_MINOR 0
#define ADB_SERVER_VERSION 31
typedef struct amessage amessage;
typedef struct apacket apacket;
typedef struct asocket asocket;
typedef struct alistener alistener;
typedef struct aservice aservice;
typedef struct atransport atransport;
typedef struct adisconnect adisconnect;
typedef struct usb_handle usb_handle;
struct amessage{unsigned command,arg0,arg1,data_length,data_check,magic;};
struct apacket{
	apacket*next;
	unsigned len;
	unsigned char*ptr;
	amessage msg;
	unsigned char data[MAX_PAYLOAD];
};
struct asocket{
	asocket*next,*prev;
	unsigned id;
	int closing,exit_on_close;
	asocket*peer;
	fdevent fde;
	int fd;
	apacket*pkt_first,*pkt_last;
	int (*enqueue)(asocket*s,apacket*pkt);
	void(*ready)(asocket*s),(*close)(asocket*s);
	void*extra;
	atransport*transport;
};

struct adisconnect{
	void(*func)(void*opaque,atransport*t);
	void*opaque;
	adisconnect*next,*prev;
};
typedef enum transport_type{kTransportUsb,kTransportLocal,kTransportAny,kTransportHost,}transport_type;
#define TOKEN_SIZE 20
struct atransport{
	atransport*next,*prev;
	int (*read_from_remote)(apacket*p,atransport*t),(*write_to_remote)(apacket*p,atransport*t);
	void (*close)(atransport*t),(*kick)(atransport*t);
	int fd,transport_socket;
	fdevent transport_fde;
	int ref_count;
	unsigned sync_token;
	int connection_state,online;
	transport_type type;
	usb_handle*usb;
	int sfd;
	char*serial,*product,*model,*device,*devpath;
	int adb_port;
	int kicked;
	adisconnect disconnects;
	void*key;
	unsigned char token[TOKEN_SIZE];
	fdevent auth_fde;
	unsigned failed_auth_attempts;
};
struct alistener{
	alistener*next,*prev;
	fdevent fde;
	int fd;
	const char*local_name,*connect_to;
	atransport*transport;
	adisconnect disconnect;
};
extern void print_packet(const char*label,apacket*p);
extern asocket*find_local_socket(unsigned id);
extern void install_local_socket(asocket*s);
extern void remove_socket(asocket*s);
extern void close_all_sockets(atransport*t);
#define LOCAL_CLIENT_PREFIX "linux-systemd-"
extern asocket*create_local_socket(int fd);
extern asocket*create_local_service_socket(const char*destination);
extern asocket*create_remote_socket(unsigned id,atransport*t);
extern void connect_to_remote(asocket*s,const char*destination);
extern void connect_to_smartsocket(asocket*s);
extern void fatal(const char*fmt,...);
extern void fatal_errno(const char*fmt,...);
extern void handle_packet(apacket*p,atransport*t);
extern void send_packet(apacket*p,atransport*t);
extern void get_my_path(char*s,size_t maxLen);
extern int launch_server(int server_port);
extern int adb_main(int force_usb,int server_port);
extern void init_transport_registration(void);
extern int list_transports(char*buf,size_t bufsize,int long_listing);
extern void update_transports(void);
extern asocket*create_device_tracker(void);
extern atransport*acquire_one_transport(int state,transport_type ttype,const char*serial,char**error_out);
extern void add_transport_disconnect(atransport*t,adisconnect*dis);
extern void remove_transport_disconnect(atransport*t,adisconnect*dis);
extern void run_transport_disconnects(atransport*t);
extern void kick_transport(atransport*t);
extern int init_socket_transport(atransport*t,int s,int port,int local);
extern void init_usb_transport(atransport*t,usb_handle*usb,int state);
extern void close_usb_devices();
extern void register_socket_transport(int s,const char*serial,int port,int local);
extern void unregister_transport(atransport*t);
extern void unregister_all_tcp_transports();
extern void register_usb_transport(usb_handle*h,const char*serial,const char*devpath,unsigned writeable);
extern void unregister_usb_transport(usb_handle*usb);
extern atransport*find_transport(const char*serial);
extern int service_to_fd(const char*name);
extern int init_jdwp(void);
extern asocket*create_jdwp_service_socket();
extern asocket*create_jdwp_tracker_service_socket();
extern int create_jdwp_connection_fd(int jdwp_pid);
typedef enum{BACKUP,RESTORE}BackupOperation;
extern void framebuffer_service(int fd,void*cookie);
extern void log_service(int fd,void*cookie);
extern void remount_service(int fd,void*cookie);
extern char*get_log_file_path(const char*log_name);
extern apacket*get_apacket(void);
extern void put_apacket(apacket*p);
extern int check_header(apacket*p);
extern int check_data(apacket*p);
#define ADB_TRACE	1
typedef enum{
	TRACE_ADB=0,
	TRACE_SOCKETS,
	TRACE_PACKETS,
	TRACE_TRANSPORT,
	TRACE_RWX,
	TRACE_USB,
	TRACE_SYNC,
	TRACE_SYSDEPS,
	TRACE_JDWP,
	TRACE_SERVICES,
	TRACE_AUTH
}AdbTrace;
extern int adb_trace_mask;
extern unsigned char adb_trace_output_count;
extern void adb_trace_init(void);
#define print_packet(tag,p) do{}while(0)
#define DEFAULT_ADB_PORT 5038
#define DEFAULT_ADB_LOCAL_TRANSPORT_PORT 5555
#define ADB_CLASS 0xff
#define ADB_SUBCLASS 0x42
#define ADB_PROTOCOL 0x1
extern void local_init(int port);
extern int local_connect(int port);
extern int local_connect_arbitrary_ports(int console_port,int adb_port);
extern void usb_init();
extern void usb_cleanup();
extern int usb_write(usb_handle*h,const void*data,int len);
extern int usb_read(usb_handle*h,void*data,int len);
extern int usb_close(usb_handle*h);
extern void usb_kick(usb_handle*h);
extern unsigned host_to_le32(unsigned n);
extern int adb_commandline(int argc,char**argv);
extern int connection_state(atransport*t);
extern int start_device_log(void);
#define CS_ANY -1
#define CS_OFFLINE 0
#define CS_BOOTLOADER 1
#define CS_DEVICE 2
#define CS_HOST 3
#define CS_RECOVERY 4
#define CS_NOPERM 5
#define CS_SIDELOAD 6
extern char*shell;
extern int HOST;
extern int SHELL_EXIT_NOTIFY_FD;
extern int recovery_mode;
extern const char*values[];
extern int auth_enabled;
extern char*adb_external_storage;
extern char*adb_device_banner;
extern char*log_file;
extern char*log_dir;
#define CHUNK_SIZE (64*1024)
#define USB_ADB_PATH "/dev/adb/usb"
#define USB_FFS_ADB_PATH "/dev/usb-ffs/adb/"
#define USB_FFS_ADB_EP(x) USB_FFS_ADB_PATH#x
#define USB_FFS_ADB_EP0 USB_FFS_ADB_EP(ep0)
#define USB_FFS_ADB_OUT USB_FFS_ADB_EP(ep1)
#define USB_FFS_ADB_IN	USB_FFS_ADB_EP(ep2)
extern int sendfailmsg(int fd,const char*reason);
extern int handle_host_request(char*service,transport_type ttype,char*serial,int reply_fd,asocket*s);
#endif
