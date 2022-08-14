/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/stream_server.h"
#include "app/pipe_app_lib.h"
#include "app/cioctl_pub.h"

#define CIOCTL_PIPE_FILE_PATH "/tmp/cioctl_pipe"

static PIPE_APP_S g_cioctl_server;

static int _cioctl_server_recv_msg(void *cfg, STREAM_CONN_S *conn)
{
    VBUF_S *vbuf = &conn->recv_vbuf;
    CIOCTL_REQUEST_S *req;
    int msg_size;
    VBUF_S *reply_vbuf = &conn->send_vbuf;
    CIOCTL_REPLY_S reply = {0};
    CIOCTL_REPLY_S *reply_ptr;
    int ret;

    req = VBUF_GetData(vbuf);
    if (! req) {
        return 0;
    }

    msg_size = ntohl(req->size);

    if (VBUF_GetDataLength(vbuf) < msg_size) {
        return 0;
    }

    if (VBUF_CatFromBuf(reply_vbuf, &reply, sizeof(reply)) < 0) {
        return -1;
    }

    ret = CIOCTL_ProcessRequest(req, reply_vbuf);

    reply_ptr = VBUF_GetData(reply_vbuf);
    reply_ptr->size = VBUF_GetDataLength(reply_vbuf);
    reply_ptr->size = htonl(reply_ptr->size);
    reply_ptr->ret = htonl(ret);

    if (ret >= 0) {
        StreamServer_Send(conn, NULL, 0);
        StreamServer_SendFinish(conn);
    }

    VBUF_CutAll(vbuf);

    return ret;
}

static int _cioctl_server_conn_event(void *cfg, STREAM_CONN_S *conn, UINT event)
{
    int ret = 0;

    switch (event) {
        case STREAM_SERVER_EVENT_RECV_MSG:
            ret = _cioctl_server_recv_msg(cfg, conn);
            break;
    }

    return ret;
}

CONSTRUCTOR(init) {
    PipeApp_Init(&g_cioctl_server, CIOCTL_PIPE_FILE_PATH);
    g_cioctl_server.server.event_func = _cioctl_server_conn_event;
}

DESTRUCTOR(fini) {
    PipeApp_Clean(&g_cioctl_server);
}

int CIOCTL_SERVER_SetNamePKey(char *pkey_name)
{
    if (g_cioctl_server.enable) {
        return BS_ERR;
    }

    g_cioctl_server.name_type = PIPE_APP_NAME_TYPE_PKEY;
    snprintf(g_cioctl_server.cfg_name, sizeof(g_cioctl_server.cfg_name), "cioctl_pipe_%s", pkey_name);

    return 0;
}

int CIOCTL_SERVER_SetNameString(char *name)
{
    if (g_cioctl_server.enable) {
        return BS_ERR;
    }

    g_cioctl_server.name_type = PIPE_APP_NAME_TYPE_STRING;
    strlcpy(g_cioctl_server.cfg_name, name, sizeof(g_cioctl_server.cfg_name));

    return 0;
}

int CIOCTL_SERVER_SetNameDefault()
{
    if (g_cioctl_server.enable) {
        return BS_ERR;
    }

    g_cioctl_server.name_type = PIPE_APP_NAME_TYPE_NONE;
    strlcpy(g_cioctl_server.cfg_name, CIOCTL_PIPE_FILE_PATH, sizeof(g_cioctl_server.cfg_name));

    return 0;
}

int CIOCTL_SERVER_GetNameType()
{
    return g_cioctl_server.name_type;
}

char * CIOCTL_SERVER_GetName()
{
    return g_cioctl_server.cfg_name;
}

int CIOCTL_SERVER_Enable()
{
    return PipeApp_Start(&g_cioctl_server);
}

int CIOCTL_SERVER_Disable()
{
    PipeApp_Stop(&g_cioctl_server);
    return 0;
}

BOOL_T CIOCTL_SERVER_IsEnabled()
{
    return g_cioctl_server.enable;
}
