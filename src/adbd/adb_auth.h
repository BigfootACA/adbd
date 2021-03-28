#ifndef __ADB_AUTH_H
#define __ADB_AUTH_H
extern void adb_auth_init(void);
extern void adb_auth_verified(atransport*t);
#define ADB_AUTH_TOKEN         1
#define ADB_AUTH_SIGNATURE     2
#define ADB_AUTH_RSAPUBLICKEY  3
static inline int adb_auth_sign(void* key,void*token,size_t token_size,void*sig){return 0;}
static inline void*adb_auth_nextkey(void*current){return NULL;}
static inline int adb_auth_get_userkey(unsigned char*data,size_t len){return 0;}
extern int adb_auth_generate_token(void*token,size_t token_size);
extern int adb_auth_verify(void*token,void*sig,int siglen);
extern void adb_auth_confirm_key(unsigned char*data,size_t len,atransport*t);
extern void adb_auth_reload_keys(void);
#endif
