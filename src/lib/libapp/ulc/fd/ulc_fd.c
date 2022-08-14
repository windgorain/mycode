/*================================================================
*   Created by LiXingang
*   Description: ulc fd
*
================================================================*/
#include "bs.h"
#include "../h/ulc_fd.h"

#define ULC_FD_MAX 0xfffff

static ULC_FILE_S g_ulc_fds[ULC_FD_MAX] = {0};
static MUTEX_S g_ulc_lock;

static int _ulcfd_attach_file(int type, void *data, PF_ULC_FD_FREE free_func)
{
    int i;
    ULC_FILE_S *fs;

    for (i=0; i<ULC_FD_MAX; i++) {
        fs = &g_ulc_fds[i];
        if (! fs->data) {
            fs->type = type;
            fs->ref_cnt = 1;
            fs->free_func = free_func;
            ATOM_BARRIER();
            fs->data = data;
            return i;
        }
    }

    return -EBADF;
}

static int _ulcfd_ref(int fd, int ref_count)
{
    ULC_FILE_S *fs = &g_ulc_fds[fd];
    void *data;

    if ((fs->data == NULL) || (fs->ref_cnt <= 0)) {
        RETURN(BS_NO_SUCH);
    }

    fs->ref_cnt += ref_count;

    if (fs->ref_cnt <= 0) {
        data = fs->data;
        fs->data = NULL;
        fs->free_func(data);
    }

    return 0;
}

int ULC_FD_Init()
{
    MUTEX_Init(&g_ulc_lock);
    return 0;
}

int ULC_FD_Open(int type, void *data, PF_ULC_FD_FREE free_func)
{
    int fd;

    if ((type < 0) || (type >= ULC_FD_TYPE_MAX)) {
        return -EINVAL;
    }

    MUTEX_P(&g_ulc_lock);
    fd = _ulcfd_attach_file(type, data, free_func);
    MUTEX_V(&g_ulc_lock);

    return fd;
}

int ULC_FD_Close(int fd)
{
    return ULC_FD_Ref(fd, -1);
}

int ULC_FD_Ref(int fd, int ref_count)
{
    int ret;

    if ((fd < 0) || (fd >= ULC_FD_MAX)) {
        RETURN(BS_BAD_PARA);
    }

    MUTEX_P(&g_ulc_lock);
    ret = _ulcfd_ref(fd, ref_count);
    MUTEX_V(&g_ulc_lock);

    return ret;
}

int ULC_FD_IncRef(int fd)
{
    return ULC_FD_Ref(fd, 1);
}

int ULC_FD_DecRef(int fd)
{
    return ULC_FD_Ref(fd, -1);
}

ULC_FILE_S * ULC_FD_GetFile(int fd)
{
    if ((fd < 0) || (fd >= ULC_FD_MAX)) {
        return NULL;
    }

    if (g_ulc_fds[fd].data == NULL) {
        return NULL;
    }

    return &g_ulc_fds[fd];
}

void * ULC_FD_GetFileData(int fd)
{
    if ((fd < 0) || (fd >= ULC_FD_MAX)) {
        return NULL;
    }

    return g_ulc_fds[fd].data;
}

/* 获取file并增加引用计数 */
void * ULC_FD_RefFile(int fd)
{
    ULC_FD_IncRef(fd);
    return ULC_FD_GetFile(fd);
}

/* 获取filedata并增加引用计数 */
void * ULC_FD_RefFileData(int fd)
{
    ULC_FD_IncRef(fd);
    return ULC_FD_GetFileData(fd);
}

int ULC_FD_GetFileType(int fd)
{
    if ((fd < 0) || (fd >= ULC_FD_MAX)) {
        return -1;
    }

    return g_ulc_fds[fd].type;
}

/* 返回-1表示结束 */
int ULC_FD_GetNext(int curr /* -1表示获取第一个 */)
{
    int i;
    int start = curr + 1;

    if(curr < 0) {
        start = 0;
    }

    for (i=start; i<ULC_FD_MAX; i++) {
        if (g_ulc_fds[i].data) {
            return i;
        }
    }

    return -1;
}

/* 返回-1表示结束 */
int ULC_FD_GetNextOfType(int type, int curr /* -1表示获取第一个 */)
{
    while ((curr = ULC_FD_GetNext(curr)) >= 0) {
        if (ULC_FD_GetFileType(curr) == type) {
            return curr;
        }
    }

    return -1;
}

