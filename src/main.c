#include<getopt.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdbool.h>
#include<strings.h>
#include"adbd/fdevent.h"
#include"adbd/adb.h"
static int usage(){
	puts(
		"Android Debug Bridge Daemon for Embedded Linux\n"
		"\n"
		"Usage: \n"
		"  adbd [--recovery] [--auth] [--logdir LOGDIR] [--log LOGFILE] [--shell SHELL] \n"
		"       [--banner BANNER] [--devname DEVICE_NAME] [--devmodel DEVICE_MODEL] \n"
		"       [--devproduct DEVICE_PRODUCT] [--external EXTERNAL_STORAGE] [--daemon] \n"
		"\n"
		"Defaults: \n"
		"  LOGDIR     : \"/var/log/adb\"\n"
		"  LOGFILE    : \"LOGDIR/adb-%%Y-%%m-%%d-%%H-%%M-%%S.log\"\n"
		"  BANNER     : \"device\"\n"
		"  DEVNAME    : \"Linux\"\n"
		"  DEVMODEL   : \"Systemd\"\n"
		"  DEVPRODUCT : \"GNU\"\n"
		"  EXTERNAL   : env var 'ADB_EXTERNAL_STORAGE' or \"/media\"\n"
		"  SHELL      : env var 'SHELL' or \"/bin/sh\"\n"
		"\n"
		"Values: \n"
		"  LOGDIR     : writable folder path (must exists).\n"
		"  LOGFILE    : writable file path (not exists) or '-' for stdout/stderr.\n"
		"  BANNER     : \"bootloader\", \"sideload\", \"recovery\", \"device\"\n"
		"  DEVNAME    : string (A-Z, a-z, 0-9, '-', '_')\n"
		"  DEVMODEL   : string (A-Z, a-z, 0-9, '-', '_')\n"
		"  DEVPRODUCT : string (A-Z, a-z, 0-9, '-', '_')\n"
		"  EXTERNAL   : folder path. (must exists)\n"
		"  SHELL      : executable file path. (must exists)\n"
		"  PROTOCOL   : force protocol. (usb or tcp)\n"
		"\n"
		"Options: \n"
		"\t --help                  , -h            : show this help.\n"
		"\t --protocol PROTOCOL     , -P PROTOCOL   : set protocol.\n"
		"\t --usb                   , -u            : set to USB protocol.\n"
		"\t --tcp                   , -t            : set to TCP protocol.\n"
		"\t --daemon                , -d            : run in daemon mode.\n"
		"\t --auth                  , -a            : turn on adbd need auth.\n"
		"\t --recovery              , -r            : set adbd to recovery mode.\n"
		"\t --devname DEVNAME       , -n DEVNAME    : device name. (ro.product.name)\n"
		"\t --devmodel DEVMODEL     , -m DEVMODEL   : device model. (ro.product.model)\n"
		"\t --devproduct DEVPRODUCT , -p DEVPRODUCT : device product. (ro.product.product)\n"
		"\t --logdir LOGDIR         , -l LOGDIR     : logs save folder.\n"
		"\t --log LOGFILE           , -o LOGFILE    : log save path. (ignore LOGDIR)\n"
		"\t --banner BANNER         , -b BANNER     : device banner show in 'adb devices'.\n"
		"\t --shell SHELL           , -s SHELL      : system shell. (adb shell)\n"
		"\t --external EXTERNAL     , -e EXTERNAL   : external storage.\n"
		"\n"
		"Support Protocols: usb, tcp"
	);
	return 2;
}
int main(int argc,char**argv){
	int o;
	const char*sopts="utrdahl:o:b:n:m:p:s:e:P:";
	const struct option lopts[]={
		{"recovery",   no_argument,       NULL, 'r'},
		{"daemon",     no_argument,       NULL, 'd'},
		{"auth",       no_argument,       NULL, 'a'},
		{"help",       no_argument,       NULL, 'h'},
		{"tcp",        no_argument,       NULL, 't'},
		{"usb",        no_argument,       NULL, 'u'},
		{"protocol",   required_argument, NULL, 'P'},
		{"logdir",     required_argument, NULL, 'l'},
		{"log",        required_argument, NULL, 'o'},
		{"banner",     required_argument, NULL, 'b'},
		{"devname",    required_argument, NULL, 'n'},
		{"devmodel",   required_argument, NULL, 'm'},
		{"devproduct", required_argument, NULL, 'p'},
		{"shell",      required_argument, NULL, 's'},
		{"external",   required_argument, NULL, 'e'},
		{NULL, 0, NULL, 0}
	};
	bool daemon=false;
	shell=getenv("SHELL");
	if(!shell)shell="/bin/sh";
	adb_external_storage=getenv("ADB_EXTERNAL_STORAGE");
	if(!adb_external_storage)adb_external_storage="/media";
	char proto=0;
	while((o=getopt_long(argc,argv,sopts,lopts,NULL))!=-1)switch(o){
		case 'h':return usage();break;
		case 'r':recovery_mode=1;break;
		case 'd':daemon=true;break;
		case 'a':auth_enabled=1;break;
		case 'l':log_dir=optarg;break;
		case 'o':log_file=optarg;break;
		case 'b':adb_device_banner=optarg;break;
		case 'n':values[0]=optarg;break;
		case 'm':values[1]=optarg;break;
		case 'p':values[2]=optarg;break;
		case 's':shell=optarg;break;
		case 'e':adb_external_storage=optarg;break;
		case 'u':proto=1;break;
		case 't':proto=2;break;
		case 'P':
			if(!strcasecmp(optarg,"tcp"))proto=2;
			else if(!strcasecmp(optarg,"usb"))proto=1;
			else{
				fprintf(stderr,"Invalid argument '%s' for --mode\n",optarg);
				return 2;
			}
		break;
		default:fprintf(stderr,"Unknown argument: %c\n",(char)o);return 2;
	}
	if(access(shell,X_OK)!=0){
		perror("adbd: no executable shell found");
		return 1;
	}
	if(daemon){
		pid_t p=fork();
		if(p>0){
			printf("adbd: daemon run with pid %d.\n",(int)p);
			return 0;
		}else if(p==0)setsid();
	        else if(p<0){
			perror("adbd: fork");
			return 1;
	        }
	}
	start_device_log();
	printf("adbd: adbd main starting\n");
	return adb_main(proto,DEFAULT_ADB_PORT);
}
