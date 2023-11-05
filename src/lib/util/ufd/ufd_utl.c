/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.1.2
*   Description: user space fd: 用户态fd
*
================================================================*/
#include "bs.h"
#include "utl/ufd_utl.h"

static int _ufd_attach_file(UFD_S *ctx, int type, void *data, PF_UFD_FREE free_func)
{
    int i;
    UFD_FILE_S *fs;

    for (i=0; i<ctx->capacity; i++) {
        fs = &ctx->fds[i];
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

static int _ufd_ref(UFD_S *ctx, int fd, int ref_count)
{
    UFD_FILE_S *fs = &ctx->fds[fd];
    void *data;

    if ((fs->data == NULL) || (fs->ref_cnt <= 0)) {
        RETURN(BS_NO_SUCH);
    }

    fs->ref_cnt += ref_count;

    if (fs->ref_cnt <= 0) {
        data = fs->data;
        fs->data = NULL;
        fs->free_func(ctx, data);
    }

    return 0;
}

int UFD_Init(INOUT UFD_S *ctx, UINT capacity)
{
    ctx->capacity = capacity;
    return 0;
}

UFD_S * UFD_Create(UINT capacity)
{
    UFD_S *ctx = MEM_ZMalloc(sizeof(UFD_S) + capacity * sizeof(UFD_FILE_S));
    if (! ctx) {
        return NULL;
    }

    UFD_Init(ctx, capacity);

    return ctx;
}

void UFD_Destroy(UFD_S *ctx)
{
    int i;
    UFD_FILE_S *fs;

    for (i=0; i<ctx->capacity; i++) {
        fs = &ctx->fds[i];
        if ((fs->data) && (fs->ref_cnt) && (fs->free_func)){
            fs->free_func(ctx, fs->data);
        }
    }

    MEM_Free(ctx);
}

int UFD_Open(UFD_S *ctx, int type, void *data, PF_UFD_FREE free_func)
{
    return _ufd_attach_file(ctx, type, data, free_func);
}

int UFD_Close(UFD_S *ctx, int fd)
{
    return UFD_Ref(ctx, fd, -1);
}

int UFD_Ref(UFD_S *ctx, int fd, int ref_count)
{
    int ret;

    if ((fd < 0) || (fd >= ctx->capacity)) {
        RETURN(BS_BAD_PARA);
    }

    ret = _ufd_ref(ctx, fd, ref_count);

    return ret;
}

int UFD_IncRef(UFD_S *ctx, int fd)
{
    return UFD_Ref(ctx, fd, 1);
}

int UFD_DecRef(UFD_S *ctx, int fd)
{
    return UFD_Ref(ctx, fd, -1);
}

UFD_FILE_S * UFD_GetFile(UFD_S *ctx, int fd)
{
    if ((fd < 0) || (fd >= ctx->capacity)) {
        return NULL;
    }

    if (ctx->fds[fd].data == NULL) {
        return NULL;
    }

    return &ctx->fds[fd];
}

void * UFD_GetFileData(UFD_S *ctx, int fd)
{
    if ((fd < 0) || (fd >= ctx->capacity)) {
        return NULL;
    }

    return ctx->fds[fd].data;
}


UFD_FILE_S * UFD_RefFile(UFD_S *ctx, int fd)
{
    UFD_IncRef(ctx, fd);
    return UFD_GetFile(ctx, fd);
}


void * UFD_RefFileData(UFD_S *ctx, int fd)
{
    UFD_IncRef(ctx, fd);
    return UFD_GetFileData(ctx, fd);
}

int UFD_GetFileType(UFD_S *ctx, int fd)
{
    if ((fd < 0) || (fd >= ctx->capacity)) {
        return -1;
    }

    return ctx->fds[fd].type;
}


int UFD_GetNext(UFD_S *ctx, int curr )
{
    int i;
    int start = curr + 1;

    if(curr < 0) {
        start = 0;
    }

    for (i=start; i<ctx->capacity; i++) {
        if (ctx->fds[i].data) {
            return i;
        }
    }

    return -1;
}


int UFD_GetNextOfType(UFD_S *ctx, int type, int curr )
{
    while ((curr = UFD_GetNext(ctx, curr)) >= 0) {
        if (UFD_GetFileType(ctx, curr) == type) {
            return curr;
        }
    }

    return -1;
}

