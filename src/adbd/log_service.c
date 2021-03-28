#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/socket.h>
#include<cutils/logger.h>
#include"sysdeps.h"
#include"adb.h"
#define LOG_FILE_DIR "/var/log/"
void write_log_entry(int fd,struct logger_entry*buf);
void log_service(int fd,void*cookie){
	char*log_filepath=cookie;
	int logfd=unix_open(log_filepath,O_RDONLY);
	if(logfd<0)goto done;
	unsigned char buf[LOGGER_ENTRY_MAX_LEN+1]__attribute__((aligned(4)));
	struct logger_entry*entry=(struct logger_entry*)buf;
	while(1){
		int ret;
		if((ret=unix_read(logfd,entry,LOGGER_ENTRY_MAX_LEN))<0){
			if(errno==EINTR||errno==EAGAIN)continue;
			goto done;
		}else if(!ret)goto done;
		entry->msg[entry->len]='\0';
		write_log_entry(fd,entry);
	}
done:
	unix_close(fd);
	free(log_filepath);
}
char*get_log_file_path(const char*log_name){
	char*log_device=malloc(strlen(LOG_FILE_DIR)+strlen(log_name)+1);
	strcpy(log_device,LOG_FILE_DIR);
	strcat(log_device,log_name);
	return log_device;
}
void write_log_entry(int fd,struct logger_entry*buf){
	size_t size=sizeof(struct logger_entry)+buf->len;
	writex(fd,buf,size);
}
