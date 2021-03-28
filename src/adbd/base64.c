#include<sys/cdefs.h>
#include<sys/types.h>
#include<sys/param.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include"arpa_nameser.h"
#include<assert.h>
#include<ctype.h>
#include<resolv.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
static const char Base64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64='=';
int b64_pton(char const*src,u_char*target,size_t targsize){
	size_t tarindex;
	int state, ch;
	char *pos;
	if(!src||!target)return -1;
	assert(src !=NULL);
	assert(target !=NULL);
	state=0;
	tarindex=0;
	while((ch=(u_char)*src++)!='\0'){
		if(isspace(ch))continue;
		if(ch==Pad64)break;
		if((pos=strchr(Base64,ch))==0)return(-1);
		switch(state){
			case 0:
				if(target){
					if(tarindex>=targsize)return(-1);
					target[tarindex]=(pos-Base64)<<2;
				}
				state=1;
			break;
			case 1:
				if(target){
					if(tarindex+1>=targsize)return(-1);
					target[tarindex]|=(u_int32_t)(pos-Base64)>>4;
					target[tarindex+1]=((pos-Base64)&0x0f)<<4;
				}
				tarindex++;
				state=2;
			break;
			case 2:
				if(target){
					if(tarindex+1>=targsize)return(-1);
					target[tarindex]|=(u_int32_t)(pos-Base64)>>2;
					target[tarindex+1]=((pos-Base64)&0x03)<<6;
				}
				tarindex++;
				state=3;
			break;
			case 3:
				if(target){
					if(tarindex>=targsize)return(-1);
					target[tarindex]|=(pos-Base64);
				}
				tarindex++;
				state=0;
			break;
			default:abort();
		}
	}
	if(ch==Pad64){
		ch=*src++;
		switch(state){
			case 0:case 1:return(-1);
			case 2:
				for(;ch!='\0';ch=(u_char)*src++)if(!isspace(ch))break;
				if(ch!=Pad64)return(-1);
				ch=*src++;
			/* FALLTHROUGH */
			case 3:
				for(;ch!='\0';ch=(u_char)*src++)if(!isspace(ch))return(-1);
				if(target&&target[tarindex]!=0)return(-1);
			break;
		}
	}else if(state!=0)return(-1);
	return(tarindex);
}
