/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/umap_utl.h"
#include "app/cioctl_pub.h"
#include "../h/ulcapp_cfg_lock.h"
#include "../h/ulcapp_runtime.h"

enum {
    ULC_CIOCTL_LOOKUP_ELE = 0,
    ULC_CIOCTL_DELETE_ELE,
    ULC_CIOCTL_UPDATE_ELE,
    ULC_CIOCTL_GETNEXT_KEY,
};

typedef struct {
    UINT cmd;
}ULC_CIOCTL_REQUEST_S;

typedef struct {
    UINT fd;
    UINT flag;
    UCHAR data[0];
}ULC_CIOCTL_S;

static int _ulcapp_ioctl_lookup_ele(MYBPF_RUNTIME_S *runtime, void *data, int data_len, VBUF_S *reply)
{
    int fd;
    int key_len;
    ULC_CIOCTL_S *d = data;
    UMAP_HEADER_S *hdr;

    if (data_len <= sizeof(ULC_CIOCTL_S)) {
        RETURN(BS_BAD_REQUEST);
    }

    key_len = data_len - sizeof(ULC_CIOCTL_S);
    fd = ntohl(d->fd);

    hdr = UMAP_GetByFd(runtime->ufd_ctx, fd);
    if (! hdr) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    if (key_len < hdr->size_key) {
        RETURN(BS_BAD_PARA);
    }

    void *value = UMAP_LookupElem(hdr, d->data);
    if (! value) {
        return BS_NO_SUCH;
    }

    return VBUF_CatBuf(reply, value, hdr->size_value);
}

static int _ulcapp_ioctl_delete_ele(MYBPF_RUNTIME_S *runtime, void *data, int data_len, VBUF_S *reply)
{
    int fd;
    int key_len;
    ULC_CIOCTL_S *d = data;
    UMAP_HEADER_S *hdr;

    if (data_len <= sizeof(ULC_CIOCTL_S)) {
        RETURN(BS_BAD_REQUEST);
    }

    key_len = data_len - sizeof(ULC_CIOCTL_S);
    fd = ntohl(d->fd);

    hdr = UMAP_GetByFd(runtime->ufd_ctx, fd);
    if (! hdr) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    if (key_len < hdr->size_key) {
        RETURN(BS_BAD_PARA);
    }

    return UMAP_DeleteElem(hdr, d->data);
}

static int _ulcapp_ioctl_update_ele(MYBPF_RUNTIME_S *runtime, void *data, int data_len, VBUF_S *reply)
{
    ULC_CIOCTL_S *d = data;
    UMAP_HEADER_S *hdr;

    if (data_len <= sizeof(ULC_CIOCTL_S)) {
        RETURN(BS_BAD_REQUEST);
    }

    int kv_len = data_len - sizeof(ULC_CIOCTL_S);
    int fd = ntohl(d->fd);
    UINT flag = ntohl(d->flag);

    hdr = UMAP_GetByFd(runtime->ufd_ctx, fd);
    if (! hdr) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    if (kv_len < hdr->size_key + hdr->size_value) {
        RETURN(BS_BAD_PARA);
    }

    void *key = d->data;
    void *value = (char*)key + hdr->size_key;

    return UMAP_UpdateElem(hdr, key, value, flag);
}

static int _ulcapp_ioctl_getnext_key(MYBPF_RUNTIME_S *runtime, void *data, int data_len, VBUF_S *reply)
{
    int fd;
    int key_len;
    ULC_CIOCTL_S *d = data;
    UMAP_HEADER_S *hdr;
    void *next_key;
    int ret;

    if (data_len <= sizeof(ULC_CIOCTL_S)) {
        RETURN(BS_BAD_REQUEST);
    }

    key_len = data_len - sizeof(ULC_CIOCTL_S);
    fd = ntohl(d->fd);

    hdr = UMAP_GetByFd(runtime->ufd_ctx, fd);
    if (! hdr) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    if (key_len < hdr->size_key) {
        RETURN(BS_BAD_PARA);
    }

    ret = UMAP_GetNextKey(hdr, d->data, &next_key);
    if (ret < 0) {
        return BS_NO_SUCH;
    }

    return VBUF_CatBuf(reply, &next_key, hdr->size_key);
}

static int _ulcapp_ioctl_process_request_locked(MYBPF_RUNTIME_S *runtime, CIOCTL_REQUEST_S *request, VBUF_S *reply)
{
    int totle_len = ntohl(request->size);
    int req_len = totle_len - sizeof(CIOCTL_REQUEST_S);
    ULC_CIOCTL_REQUEST_S *req = (void*)(request + 1);
    int ret;
    UINT cmd;

    if (req_len < sizeof(ULC_CIOCTL_REQUEST_S)) {
        RETURN(BS_BAD_REQUEST);
    }

    void *data = (req + 1);
    int data_len = req_len - sizeof(ULC_CIOCTL_REQUEST_S);

    cmd = ntohl(req->cmd);

    switch (cmd) {
        case ULC_CIOCTL_LOOKUP_ELE:
            ret = _ulcapp_ioctl_lookup_ele(runtime, data, data_len, reply);
            break;
        case ULC_CIOCTL_DELETE_ELE:
            ret = _ulcapp_ioctl_delete_ele(runtime, data, data_len, reply);
            break;
        case ULC_CIOCTL_UPDATE_ELE:
            ret = _ulcapp_ioctl_update_ele(runtime, data, data_len, reply);
            break;
        case ULC_CIOCTL_GETNEXT_KEY:
            ret = _ulcapp_ioctl_getnext_key(runtime, data, data_len, reply);
            break;
        default:
            ret = BS_NOT_SUPPORT;
            break;
    }

    return ret;
}

static int _ulcapp_ioctl_process_request(CIOCTL_REQUEST_S *request, VBUF_S *reply)
{
    int ret;
    MYBPF_RUNTIME_S *runtime = ULCAPP_GetRuntime();

    ULCAPP_CfgLock();
    ret = _ulcapp_ioctl_process_request_locked(runtime, request, reply);
    ULCAPP_CfgUnlock();
    return ret;
}

static CIOCTL_OB_S g_ulcapp_cioctl_ob = {
    .name = "ulc",
    .func = _ulcapp_ioctl_process_request,
};

int ULCAPP_IOCTL_Init()
{
    return CIOCTL_RegOb(&g_ulcapp_cioctl_ob);
}

