#ifndef _ANDROID_FILESYSTEM_CONFIG_H_
#define _ANDROID_FILESYSTEM_CONFIG_H_
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#define AID_ROOT             0     /* traditional unix root user */
#define AID_SYSTEM           1000  /* system server */
#define AID_RADIO            1001  /* telephony subsystem, RIL */
#define AID_BLUETOOTH        1002  /* bluetooth subsystem */
#define AID_GRAPHICS         1003  /* graphics devices */
#define AID_INPUT            1004  /* input devices */
#define AID_AUDIO            1005  /* audio devices */
#define AID_CAMERA           1006  /* camera devices */
#define AID_LOG              1007  /* log devices */
#define AID_COMPASS          1008  /* compass device */
#define AID_MOUNT            1009  /* mountd socket */
#define AID_WIFI             1010  /* wifi subsystem */
#define AID_ADB              1011  /* android debug bridge (adbd) */
#define AID_INSTALL          1012  /* group for installing packages */
#define AID_MEDIA            1013  /* mediaserver process */
#define AID_DHCP             1014  /* dhcp client */
#define AID_SDCARD_RW        1015  /* external storage write access */
#define AID_VPN              1016  /* vpn system */
#define AID_KEYSTORE         1017  /* keystore subsystem */
#define AID_USB              1018  /* USB devices */
#define AID_DRM              1019  /* DRM server */
#define AID_MDNSR            1020  /* MulticastDNSResponder (service discovery) */
#define AID_GPS              1021  /* GPS daemon */
#define AID_UNUSED1          1022  /* deprecated, DO NOT USE */
#define AID_MEDIA_RW         1023  /* internal media storage write access */
#define AID_MTP              1024  /* MTP USB driver access */
#define AID_UNUSED2          1025  /* deprecated, DO NOT USE */
#define AID_DRMRPC           1026  /* group for drm rpc */
#define AID_NFC              1027  /* nfc subsystem */
#define AID_SDCARD_R         1028  /* external storage read access */
#define AID_SHELL            32011 /* adb and debug shell user */
#define AID_CACHE            2001  /* cache access */
#define AID_DIAG             2002  /* access to diagnostic resources */
#define AID_NET_BT_ADMIN     3001  /* bluetooth: create any socket */
#define AID_NET_BT           3002  /* bluetooth: create sco, rfcomm or l2cap sockets */
#define AID_INET             3003  /* can create AF_INET and AF_INET6 sockets */
#define AID_NET_RAW          3004  /* can create raw INET sockets */
#define AID_NET_ADMIN        3005  /* can configure interfaces and routing tables. */
#define AID_NET_BW_STATS     3006  /* read bandwidth statistics */
#define AID_NET_BW_ACCT      3007  /* change bandwidth statistics accounting */
#define AID_NET_BT_STACK     3008  /* bluetooth: access config files */
#define AID_QCOM_ONCRPC      3009  /* can read/write /dev/oncrpc files */
#define AID_QCOM_DIAG        3010  /* can read/write /dev/diag */
#define AID_MISC             9998  /* access to misc storage */
#define AID_NOBODY           9999
#define AID_APP              10000 /* first app user */
#define AID_ISOLATED_START   99000 /* start of uids for fully isolated sandboxed processes */
#define AID_ISOLATED_END     99999 /* end of uids for fully isolated sandboxed processes */
#define AID_USER             100000/* offset for uid ranges for each user */
#define AID_SHARED_GID_START 50000 /* start of gids for apps in each user to share */
#define AID_SHARED_GID_END   59999 /* start of gids for apps in each user to share */
#if !defined(EXCLUDE_FS_CONFIG_STRUCTURES)
struct android_id_info{const char*name;unsigned aid;};
static const struct android_id_info android_ids[]={
	{"root",         AID_ROOT},
	{"system",       AID_SYSTEM},
	{"radio",        AID_RADIO},
	{"bluetooth",    AID_BLUETOOTH},
	{"graphics",     AID_GRAPHICS},
	{"input",        AID_INPUT},
	{"audio",        AID_AUDIO},
	{"camera",       AID_CAMERA},
	{"log",          AID_LOG},
	{"compass",      AID_COMPASS},
	{"mount",        AID_MOUNT},
	{"wifi",         AID_WIFI},
	{"dhcp",         AID_DHCP},
	{"adb",          AID_ADB},
	{"install",      AID_INSTALL},
	{"media",        AID_MEDIA},
	{"drm",          AID_DRM},
	{"mdnsr",        AID_MDNSR},
	{"nfc",          AID_NFC},
	{"drmrpc",       AID_DRMRPC},
	{"shell",        AID_SHELL},
	{"cache",        AID_CACHE},
	{"diag",         AID_DIAG},
	{"net_bt_admin", AID_NET_BT_ADMIN},
	{"net_bt",       AID_NET_BT},
	{"net_bt_stack", AID_NET_BT_STACK},
	{"sdcard_r",     AID_SDCARD_R},
	{"sdcard_rw",    AID_SDCARD_RW},
	{"media_rw",     AID_MEDIA_RW},
	{"vpn",          AID_VPN},
	{"keystore",     AID_KEYSTORE},
	{"usb",          AID_USB},
	{"mtp",          AID_MTP},
	{"gps",          AID_GPS},
	{"inet",         AID_INET},
	{"net_raw",      AID_NET_RAW},
	{"net_admin",    AID_NET_ADMIN},
	{"net_bw_stats", AID_NET_BW_STATS},
	{"net_bw_acct",  AID_NET_BW_ACCT},
	{"qcom_oncrpc",  AID_QCOM_ONCRPC},
	{"qcom_diag",    AID_QCOM_DIAG},
	{"misc",         AID_MISC},
	{"nobody",       AID_NOBODY},
};
#define android_id_count (sizeof(android_ids)/sizeof(android_ids[0]))
struct fs_path_config{unsigned mode,uid,gid;const char*prefix;};
static struct fs_path_config android_dirs[]={
	{00770, AID_SYSTEM,   AID_CACHE,    "cache"},
	{00771, AID_SYSTEM,   AID_SYSTEM,   "data/app"},
	{00771, AID_SYSTEM,   AID_SYSTEM,   "data/app-private"},
	{00771, AID_SYSTEM,   AID_SYSTEM,   "data/dalvik-cache"},
	{00771, AID_SYSTEM,   AID_SYSTEM,   "data/data"},
	{00771, AID_SHELL,    AID_SHELL,    "data/local/tmp"},
	{00771, AID_SHELL,    AID_SHELL,    "data/local"},
	{01771, AID_SYSTEM,   AID_MISC,     "data/misc"},
	{00770, AID_DHCP,     AID_DHCP,     "data/misc/dhcp"},
	{00775, AID_MEDIA_RW, AID_MEDIA_RW, "data/media"},
	{00775, AID_MEDIA_RW, AID_MEDIA_RW, "data/media/Music"},
	{00771, AID_SYSTEM,   AID_SYSTEM,   "data"},
	{00750, AID_ROOT,     AID_SHELL,    "sbin"},
	{00755, AID_ROOT,     AID_ROOT,     "system/addon.d"},
	{00755, AID_ROOT,     AID_SHELL,    "system/bin"},
	{00755, AID_ROOT,     AID_SHELL,    "system/vendor"},
	{00755, AID_ROOT,     AID_SHELL,    "system/xbin"},
	{00755, AID_ROOT,     AID_ROOT,     "system/etc/ppp"},
	{00777, AID_ROOT,     AID_ROOT,     "sdcard"},
	{00755, AID_ROOT,     AID_ROOT,     0},
};
static struct fs_path_config android_files[]={
	{00440, AID_ROOT,      AID_SHELL,     "system/etc/init.goldfish.rc"},
	{00550, AID_ROOT,      AID_SHELL,     "system/etc/init.goldfish.sh"},
	{00440, AID_ROOT,      AID_SHELL,     "system/etc/init.trout.rc"},
	{00550, AID_ROOT,      AID_SHELL,     "system/etc/init.ril"},
	{00550, AID_ROOT,      AID_SHELL,     "system/etc/init.testmenu"},
	{00550, AID_DHCP,      AID_SHELL,     "system/etc/dhcpcd/dhcpcd-run-hooks"},
	{00440, AID_BLUETOOTH, AID_BLUETOOTH, "system/etc/dbus.conf"},
	{00444, AID_RADIO,     AID_AUDIO,     "system/etc/AudioPara4.csv"},
	{00555, AID_ROOT,      AID_ROOT,      "system/etc/ppp/*"},
	{00555, AID_ROOT,      AID_ROOT,      "system/etc/rc.*"},
	{00755, AID_ROOT,      AID_ROOT,      "system/addon.d/*"},
	{00644, AID_SYSTEM,    AID_SYSTEM,    "data/app/*"},
	{00644, AID_MEDIA_RW,  AID_MEDIA_RW,  "data/media/*"},
	{00644, AID_SYSTEM,    AID_SYSTEM,    "data/app-private/*"},
	{00644, AID_APP,       AID_APP,       "data/data/*"},
	{02755, AID_ROOT,      AID_NET_RAW,   "system/bin/ping"},
	{02750, AID_ROOT,      AID_INET,      "system/bin/netcfg"},
	{06755, AID_ROOT,      AID_ROOT,      "system/xbin/su"},
	{06755, AID_ROOT,      AID_ROOT,      "system/xbin/librank"},
	{06755, AID_ROOT,      AID_ROOT,      "system/xbin/procrank"},
	{06755, AID_ROOT,      AID_ROOT,      "system/xbin/procmem"},
	{06755, AID_ROOT,      AID_ROOT,      "system/xbin/tcpdump"},
	{04770, AID_ROOT,      AID_RADIO,     "system/bin/pppd-ril"},
	{06750, AID_ROOT,      AID_SHELL,     "system/bin/run-as"},
	{06750, AID_ROOT,      AID_SYSTEM,    "system/bin/rebootcmd"},
	{00755, AID_ROOT,      AID_SHELL,     "system/bin/*"},
	{00755, AID_ROOT,      AID_ROOT,      "system/lib/valgrind/*"},
	{00755, AID_ROOT,      AID_SHELL,     "system/xbin/*"},
	{00755, AID_ROOT,      AID_SHELL,     "system/vendor/bin/*"},
	{00750, AID_ROOT,      AID_SHELL,     "sbin/*"},
	{00755, AID_ROOT,      AID_ROOT,      "bin/*"},
	{00750, AID_ROOT,      AID_SHELL,     "init*"},
	{00750, AID_ROOT,      AID_SHELL,     "charger*"},
	{00750, AID_ROOT,      AID_SHELL,     "sbin/fs_mgr"},
	{00640, AID_ROOT,      AID_SHELL,     "fstab.*"},
	{00755, AID_ROOT,      AID_SHELL,     "system/etc/init.d/*"},
	{00644, AID_ROOT,      AID_ROOT,       0},
};
static inline void fs_config(const char*path,int dir,unsigned*uid,unsigned*gid,unsigned*mode){
	struct fs_path_config*pc;
	int plen;
	pc=dir?android_dirs:android_files;
	plen=strlen(path);
	for(;pc->prefix;pc++){
		int len=strlen(pc->prefix);
		if(dir){
			if(plen<len)continue;
			if(!strncmp(pc->prefix,path,len))break;
			continue;
		}
		if(pc->prefix[len-1]=='*'){
			if(!strncmp(pc->prefix,path,len-1))break;
		}else if(plen==len){
			if(!strncmp(pc->prefix,path,len))break;
		}
	}
	*uid=pc->uid;
	*gid=pc->gid;
	*mode=(*mode&(~07777))|pc->mode;
}
#endif
#endif
