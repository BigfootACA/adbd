#ifndef __CUTILS_FS_H
#define __CUTILS_FS_H
#include<sys/types.h>
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) ({typeof(exp)_rc;do{_rc=(exp);}while(_rc==-1&&errno==EINTR);_rc;})
#endif
extern int fs_prepare_dir(const char*path,mode_t mode,uid_t uid,gid_t gid);
extern int fs_read_atomic_int(const char*path,int*value);
extern int fs_write_atomic_int(const char* path,int value);
#endif
