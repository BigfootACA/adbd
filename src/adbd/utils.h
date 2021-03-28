#ifndef _ADB_UTILS_H
#define _ADB_UTILS_H
extern char*buff_addc(char*,char*,int);
extern char*buff_adds(char*,char*,const char*);
extern char*buff_addb(char*,char*,const void*,int);
extern char*buff_add(char*,char*,const char*,...);
#define BUFF_DECL(_buff,_cursor,_end,_size) char _buff[_size],*_cursor=_buff,*_end=_cursor+(_size)
#endif
