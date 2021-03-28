#include"utils.h"
#include<stdarg.h>
#include<stdio.h>
#include<string.h>
char*buff_addc(char*buff,char*buffEnd,int c){
	int avail=buffEnd-buff;
	if(avail<=0)return buff;
	if(avail==1){
		buff[0]=0;
		return buff+1;
	}
	buff[0]=(char)c;
	buff[1]=0;
	return buff+1;
}
char*buff_adds(char*buff,char*buffEnd,const char*s){return buff_addb(buff,buffEnd,s,strlen(s));}
char*buff_addb(char*buff,char*buffEnd,const void*data,int len){
	int avail=buffEnd-buff;
	if(avail<=0||len<=0)return buff;
	if(len>avail)len=avail;
	memcpy(buff,data,len);
	buff+=len;
	if(buff>=buffEnd)buff[-1]=0;
	else buff[0]=0;
	return buff;
}
char*buff_add(char*buff,char*buffEnd,const char*format,...){
	int avail;
	if((avail=buffEnd-buff)>0){
		va_list args;
		int nn;
		va_start(args,format);
		nn=vsnprintf(buff,avail,format,args);
		va_end(args);
		if(nn<0)nn=avail;
		else if(nn>avail)nn=avail;
		buff+=nn;
		if(buff>=buffEnd)buff[-1]=0;
		else buff[0]=0;
	}
	return buff;
}
