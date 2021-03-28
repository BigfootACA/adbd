#ifndef _UTILS_LOGGER_H
#define _UTILS_LOGGER_H
#include<stdint.h>
#include<sys/ioctl.h>
struct logger_entry{
	uint16_t len,__pad;
	int32_t pid,tid,sec,nsec;
	char msg[0];
};
struct logger_entry_v2{
	uint16_t len,hdr_size;
	int32_t pid,tid,sec,nsec;
	uint32_t euid;
	char msg[0];
};
#define LOGGER_LOG_MAIN           "log/main"
#define LOGGER_LOG_RADIO          "log/radio"
#define LOGGER_LOG_EVENTS         "log/events"
#define LOGGER_LOG_SYSTEM         "log/system"
#define LOGGER_ENTRY_MAX_PAYLOAD  4076
#define LOGGER_ENTRY_MAX_LEN      (5*1024)
#define __LOGGERIO                0xAE
#define LOGGER_GET_LOG_BUF_SIZE   _IO(__LOGGERIO,1)
#define LOGGER_GET_LOG_LEN        _IO(__LOGGERIO,2)
#define LOGGER_GET_NEXT_ENTRY_LEN _IO(__LOGGERIO,3)
#define LOGGER_FLUSH_LOG          _IO(__LOGGERIO,4)
#define LOGGER_GET_VERSION        _IO(__LOGGERIO,5)
#define LOGGER_SET_VERSION        _IO(__LOGGERIO,6)
#endif /* _UTILS_LOGGER_H */
