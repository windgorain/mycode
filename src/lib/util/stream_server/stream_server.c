/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/stream_server.h"
#include "utl/socket_utl.h"
#include "utl/npipe_utl.h"
#include "utl/mypoll_utl.h"
#include "comp/comp_poller.h"

static int _stream_server_notify_event(STREAM_SERVER_S *cfg, STREAM_CONN_S *conn, UINT event)
{
    if (! cfg->event_func) {
        return 0;
    }

    return cfg->event_func(cfg, conn, event);
}

static void _stream_server_close_conn(STREAM_CONN_S *conn)
{
    STREAM_SERVER_S *cfg = conn->cfg;

    _stream_server_notify_event(cfg, conn, STREAM_SERVER_EVENT_CLOSE_CONN);
    MyPoll_Del(cfg->mypoll, conn->fd);
    Socket_Close(conn->fd);
    VBUF_Finit(&conn->recv_vbuf);
    VBUF_Finit(&conn->send_vbuf);
    MEM_Free(conn);
}

static int _stream_server_send(STREAM_CONN_S *conn, void *msg, int len)
{
    VBUF_S *vbuf = &conn->send_vbuf;
    void *data;
    int send_len;
    int ret;

    if (len > 0) {
        ret = VBUF_CatFromBuf(vbuf, msg, len);
        if (ret < 0) {
            return ret;
        }
    }

    int data_len = VBUF_GetDataLength(vbuf);
    if (0 == data_len) {
        return 0;
    }

    data = VBUF_GetData(vbuf);
    send_len = Socket_Write(conn->fd, data, data_len, 0);
    if (send_len > 0) {
        VBUF_CutHead(vbuf, send_len);
    }

    if ((conn->send_finish) && (VBUF_GetDataLength(&conn->send_vbuf) == 0)) {
        _stream_server_close_conn(conn);
    }

    return 0;
}

static int _stream_server_recv(STREAM_CONN_S *conn)
{
    STREAM_SERVER_S *cfg;
    UINT read_len;
    int ret;
    char buf[2048];

    cfg = conn->cfg;

    ret = Socket_Read2(conn->fd, buf, sizeof(buf), &read_len, 0);
    if (BS_OK != ret) {
        return -1;
    }

    if (read_len == 0) {
        return 0;
    }

    if (! cfg->event_func) {
        return 0;
    }

    ret = VBUF_CatFromBuf(&conn->recv_vbuf, buf, read_len);
    if (ret < 0) {
        return ret;
    }

    return _stream_server_notify_event(conn->cfg, conn, STREAM_SERVER_EVENT_RECV_MSG);
}

static BS_WALK_RET_E _stream_server_socket_event(int fd, UINT event, USER_HANDLE_S *ud)
{
    STREAM_CONN_S *conn = ud->ahUserHandle[0];

    if ((event & MYPOLL_EVENT_ERR) || (event & MYPOLL_EVENT_HUP)) {
        _stream_server_close_conn(conn);
        return BS_WALK_CONTINUE;
    }

    if (event & MYPOLL_EVENT_IN) {
        if (_stream_server_recv(conn) < 0) {
            _stream_server_close_conn(conn);
        }
    }

    if (event & MYPOLL_EVENT_OUT) {
        _stream_server_send(conn, NULL, 0);
    }

    return BS_WALK_CONTINUE;
}

static int _stream_server_new_conn(STREAM_SERVER_S *cfg, int fd)
{
    USER_HANDLE_S ud;
    STREAM_CONN_S *conn;

    conn = MEM_ZMalloc(sizeof(STREAM_CONN_S));
    if (! conn) {
        return -1;
    }

    conn->fd = fd;
    conn->cfg = cfg;
    VBUF_Init(&conn->recv_vbuf);
    VBUF_Init(&conn->send_vbuf);
    Socket_SetNoBlock(fd, TRUE);

    if (_stream_server_notify_event(cfg, conn, STREAM_SERVER_EVENT_NEW_CONN) < 0) {
        MEM_Free(conn);
        return -1;
    }

    ud.ahUserHandle[0] = conn;

    if (MyPoll_SetEvent(cfg->mypoll, fd, MYPOLL_EVENT_IN | MYPOLL_EVENT_ERR,
                _stream_server_socket_event, &ud) < 0) {
        _stream_server_close_conn(conn);
    }

    return 0;
}

static BS_WALK_RET_E _stream_server_listen_socket_event(int listen_fd, UINT event, USER_HANDLE_S *ud)
{
    int fd;
    STREAM_SERVER_S *cfg = ud->ahUserHandle[0];

    if (event & MYPOLL_EVENT_IN) {
        fd = Socket_Accept(listen_fd, NULL, NULL);
        if (fd >= 0) {
            if (0 != _stream_server_new_conn(cfg, fd)) {
                Socket_Close(fd);
            }
        }
    }

    return BS_WALK_CONTINUE;
}

static int _stream_server_open_listen(STREAM_SERVER_S *cfg)
{
    char *pipe_name = cfg->pipe_name;

    if (cfg->enable) {
        return 0;
    }

    if ((!pipe_name) || (pipe_name[0] == '\0')) {
        RETURN(BS_BAD_PARA);
    }

    cfg->listen_fd = NPIPE_OpenStream(pipe_name);
    if (cfg->listen_fd < 0) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    return 0;
}

static void _stream_server_close_listen(STREAM_SERVER_S *cfg)
{
    if (cfg->listen_fd >= 0) {
        Socket_Close(cfg->listen_fd);
        cfg->listen_fd = -1;
        if (cfg->type == STREAM_SERVER_TYPE_PIPE) {
            unlink(cfg->pipe_name);
        }
    }
}

int StreamServer_Start(STREAM_SERVER_S *cfg)
{
    USER_HANDLE_S ud;
    int ret;

    if (cfg->enable) {
        return 0;
    }

    ret = _stream_server_open_listen(cfg);
    if (ret < 0) {
        return ret;
    }

    Socket_SetNoBlock(cfg->listen_fd, TRUE);

    ud.ahUserHandle[0] = cfg;
    if (MyPoll_SetEvent(cfg->mypoll, cfg->listen_fd, MYPOLL_EVENT_IN | MYPOLL_EVENT_ERR,
                _stream_server_listen_socket_event, &ud)) {
        _stream_server_close_listen(cfg);
        RETURN(BS_ERR);
    }

    cfg->enable = 1;

    MyPoll_Trigger(cfg->mypoll);

    return 0;
}


void StreamServer_Stop(STREAM_SERVER_S *cfg)
{
    if (! cfg->enable) {
        return;
    }

    MyPoll_Del(cfg->mypoll, cfg->listen_fd);
    MyPoll_Restart(cfg->mypoll);
    _stream_server_close_listen(cfg);

    cfg->enable = 0;
}

int StreamServer_Send(STREAM_CONN_S *conn, void *msg, int len)
{
    return _stream_server_send(conn, msg, len);
}

void StreamServer_SendFinish(STREAM_CONN_S *conn)
{
    conn->send_finish = 1;

    if (VBUF_GetDataLength(&conn->send_vbuf) == 0) {
        _stream_server_close_conn(conn);
    }
}

