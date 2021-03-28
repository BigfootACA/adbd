#ifndef _FILE_SYNC_SERVICE_H_
#define _FILE_SYNC_SERVICE_H_
#define htoll(x) (x)
#define ltohl(x) (x)
#define MKID(a,b,c,d) ((a)|((b)<<8)|((c)<<16)|((d)<<24))
#define ID_STAT MKID('S','T','A','T')
#define ID_LIST MKID('L','I','S','T')
#define ID_ULNK MKID('U','L','N','K')
#define ID_SEND MKID('S','E','N','D')
#define ID_RECV MKID('R','E','C','V')
#define ID_DENT MKID('D','E','N','T')
#define ID_DONE MKID('D','O','N','E')
#define ID_DATA MKID('D','A','T','A')
#define ID_OKAY MKID('O','K','A','Y')
#define ID_FAIL MKID('F','A','I','L')
#define ID_QUIT MKID('Q','U','I','T')
typedef union{
	unsigned id;
	struct{unsigned id,namelen;}req;
	struct{unsigned id,mode,size,time;}stat;
	struct{unsigned id,mode,size,time,namelen;}dent;
	struct{unsigned id,size;}data;
	struct{unsigned id,msglen;}status;
}syncmsg;
void file_sync_service(int fd,void*cookie);
int do_sync_ls(const char*path);
int do_sync_push(const char*lpath,const char*rpath,int verifyApk);
int do_sync_sync(const char*lpath,const char*rpath,int listonly);
int do_sync_pull(const char*rpath,const char*lpath);
#define SYNC_DATA_MAX (64*1024)
#endif
