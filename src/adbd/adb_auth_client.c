#include<stdio.h>
#include<string.h>
#include<resolv.h>
#include<cutils/list.h>
#include<cutils/sockets.h>
#include"sysdeps.h"
#include"adb.h"
#include"adb_auth.h"
#include"fdevent.h"
#include"mincrypt/rsa.h"
#define TRACE_TAG TRACE_AUTH
extern int b64_pton(char const*src,u_char*target,size_t targsize);
struct adb_public_key{struct listnode node;RSAPublicKey key;};
static struct listnode key_list;
static char *key_paths[]={"/etc/adb_keys","/etc/adb/keys","/adb_keys","/data/misc/adb/adb_keys",NULL};
static fdevent listener_fde;
static int framework_fd=-1;
static void read_keys(const char *file,struct listnode *list){
	struct adb_public_key *key;
	FILE *f;
	char buf[MAX_PAYLOAD],*sep;
	int ret;
	if(!(f=fopen(file,"r"))){
		printf("Can't open '%s'\n",file);
		return;
	}
	while(fgets(buf,sizeof(buf),f)){
		if(!(key=calloc(1,sizeof(*key)+4))){
			printf("Can't malloc key\n");
			break;
		}
		sep=strpbrk(buf," \t");
		if(sep)*sep='\0';
		if((ret=b64_pton(buf,(u_char*)&key->key,sizeof(key->key)+4)!=sizeof(key->key))){
			printf("%s: Invalid base64 data ret=%d\n",file,ret);
			free(key);
			continue;
		}
		if(key->key.len!=RSANUMWORDS) {
			printf("%s: Invalid key len %d\n",file,key->key.len);
			free(key);
			continue;
		}
		list_add_tail(list,&key->node);
	}
	fclose(f);
}
static void free_keys(struct listnode *list){
	struct listnode *item;
	while(!list_empty(list)){
		item=list_head(list);
		list_remove(item);
		free(node_to_item(item,struct adb_public_key,node));
	}
}
void adb_auth_reload_keys(void){
	char *path,**paths=key_paths;
	struct stat buf;
	free_keys(&key_list);
	while((path=*paths++))if(!stat(path,&buf)){
		printf("Loading keys from '%s'\n",path);
		read_keys(path,&key_list);
	}
}
int adb_auth_generate_token(void *token,size_t token_size){
	FILE *f;
	int ret;
	if(!(f=fopen("/dev/urandom","r")))return 0;
	ret=fread(token,token_size,1,f);
	fclose(f);
	return ret*token_size;
}
int adb_auth_verify(void *token,void *sig,int siglen){
	/*
	struct listnode *item;
	struct adb_public_key *key;
	if(siglen!=RSANUMBYTES)return 0;
	list_for_each(item,&key_list){
		key=node_to_item(item,struct adb_public_key,node);
		if(RSA_verify(&key->key,sig,siglen,token))return 1;
	}*/
	return 0;
}
static void adb_auth_event(int fd,unsigned events,void *data){
	atransport *t=data;
	char response[2];
	int ret;
	if(!(events&FDE_READ))return;
	if((ret=unix_read(fd,response,sizeof(response)))<0){
		printf("Disconnect");
		fdevent_remove(&t->auth_fde);
		framework_fd=-1;
	}else if(ret==2&&response[0]=='O'&&response[1]=='K'){
		adb_auth_reload_keys();
		adb_auth_verified(t);
	}
}
void adb_auth_confirm_key(unsigned char *key,size_t len,atransport *t){
	char msg[MAX_PAYLOAD];
	int ret;
	if(framework_fd<0){
		printf("Client not connected\n");
		return;
	}
	if(key[len-1]!='\0'){
		printf("Key must be a null-terminated string\n");
		return;
	}
	if((ret=snprintf(msg,sizeof(msg),"PK%s",key))>=(signed)sizeof(msg)){
		printf("Key too long. ret=%d\n",ret);
		return;
	}
	printf("Sending '%s'\n",msg);
	if((ret=unix_write(framework_fd,msg,ret))<0) {
		printf("Failed to write PK,errno=%d\n",errno);
		return;
	}
	fdevent_install(&t->auth_fde,framework_fd,adb_auth_event,t);
	fdevent_add(&t->auth_fde,FDE_READ);
}
static void adb_auth_listener(int fd,unsigned events,void *data){
	struct sockaddr addr;
	socklen_t alen;
	int s;
	alen=sizeof(addr);
	if((s=adb_socket_accept(fd,&addr,&alen))<0) {
		printf("Failed to accept: errno=%d\n",errno);
		return;
	}
	framework_fd=s;
}
void adb_auth_init(void){
	int fd;
	list_init(&key_list);
	adb_auth_reload_keys();
	if((fd=android_get_control_socket("adbd"))<0){
		printf("Failed to get adbd socket\n");
		return;
	}
	if(listen(fd,4)<0){
		printf("Failed to listen on '%d'\n",fd);
		return;
	}
	fdevent_install(&listener_fde,fd,adb_auth_listener,NULL);
	fdevent_add(&listener_fde,FDE_READ);
}
