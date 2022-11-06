/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: user space fd: 用户态fd
*
================================================================*/
#include "bs.h"
#include "utl/ufd_utl.h"

#define UFD_MAX_FD 0xfffff

static UFD_FILE_S g_ufd_fds[UFD_MAX_FD] = {0};

static int _ufd_attach_file(int type, void *data, PF_UFD_FREE free_func)
{
    int i;
    UFD_FILE_S *fs;

    for (i=0; i<UFD_MAX_FD; i++) {
        fs = &g_ufd_fds[i];
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

static int _ufd_ref(int fd, int ref_count)
{
    UFD_FILE_S *fs = &g_ufd_fds[fd];
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

int UFD_Open(int type, void *data, PF_UFD_FREE free_func)
{
    return _ufd_attach_file(type, data, free_func);
}

int UFD_Close(int fd)
{
    return UFD_Ref(fd, -1);
}

int UFD_Ref(int fd, int ref_count)
{
    int ret;

    if ((fd < 0) || (fd >= UFD_MAX_FD)) {
        RETURN(BS_BAD_PARA);
    }

    ret = _ufd_ref(fd, ref_count);

    return ret;
}

int UFD_IncRef(int fd)
{
    return UFD_Ref(fd, 1);
}

int UFD_DecRef(int fd)
{
    return UFD_Ref(fd, -1);
}

UFD_FILE_S * UFD_GetFile(int fd)
{
    if ((fd < 0) || (fd >= UFD_MAX_FD)) {
        return NULL;
    }

    if (g_ufd_fds[fd].data == NULL) {
        return NULL;
    }

    return &g_ufd_fds[fd];
}

void * UFD_GetFileData(int fd)
{
    if ((fd < 0) || (fd >= UFD_MAX_FD)) {
        return NULL;
    }

    return g_ufd_fds[fd].data;
}

/* 获取file并增加引用计数 */
UFD_FILE_S * UFD_RefFile(int fd)
{
    UFD_IncRef(fd);
    return UFD_GetFile(fd);
}

/* 获取filedata并增加引用计数 */
void * UFD_RefFileData(int fd)
{
    UFD_IncRef(fd);
    return UFD_GetFileData(fd);
}

int UFD_GetFileType(int fd)
{
    if ((fd < 0) || (fd >= UFD_MAX_FD)) {
        return -1;
    }

    return g_ufd_fds[fd].type;
}

/* 返回-1表示结束 */
int UFD_GetNext(int curr /* -1表示获取第一个 */)
{
    int i;
    int start = curr + 1;

    if(curr < 0) {
        start = 0;
    }

    for (i=start; i<UFD_MAX_FD; i++) {
        if (g_ufd_fds[i].data) {
            return i;
        }
    }

    return -1;
}

/* 返回-1表示结束 */
int UFD_GetNextOfType(int type, int curr /* -1表示获取第一个 */)
{
    while ((curr = UFD_GetNext(curr)) >= 0) {
        if (UFD_GetFileType(curr) == type) {
            return curr;
        }
    }

    return -1;
}

