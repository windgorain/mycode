/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ULC_FD_H
#define _ULC_FD_H
#ifdef __cplusplus
extern "C"
{
#endif

enum {
    ULC_FD_TYPE_MAP = 0,
    ULC_FD_TYPE_PROG,

    ULC_FD_TYPE_MAX
};

typedef void (*PF_ULC_FD_FREE)(void *file);

typedef struct {
    int ref_cnt;
    UCHAR type;
    void *data;
    PF_ULC_FD_FREE free_func;
}ULC_FILE_S;

int ULC_FD_Open(int type, void *data, PF_ULC_FD_FREE free_func);
int ULC_FD_Ref(int fd, int ref_count);
int ULC_FD_IncRef(int fd);
int ULC_FD_DecRef(int fd);
int ULC_FD_Close(int fd);
int ULC_FD_GetFileType(int fd);
int ULC_FD_GetFileSize(int fd);
void * ULC_FD_GetFileData(int fd);
void * ULC_FD_RefFile(int fd);
void * ULC_FD_RefFileData(int fd);
/* 返回-1表示结束 */
int ULC_FD_GetNext(int curr /* -1表示获取第一个 */);
/* 返回-1表示结束 */
int ULC_FD_GetNextOfType(int type, int curr /* -1表示获取第一个 */);

#ifdef __cplusplus
}
#endif
#endif //ULC_FD_H_
