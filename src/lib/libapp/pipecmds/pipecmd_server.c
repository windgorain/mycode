/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2011-7-19
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/socket_utl.h"
#include "utl/exec_utl.h"
#include "utl/passwd_utl.h"
#include "utl/cff_utl.h"
#include "utl/mypoll_utl.h"
#include "utl/local_info.h"
#include "utl/npipe_utl.h"
#include "app/pipe_app_lib.h"
#include "comp/comp_poller.h"

#define PIPECMD_PIPE_FILE_PATH "/tmp/file_pipecmd"

static PIPE_APP_S g_pipecmd_server;

static void _pipecmd_server_send_data(HANDLE hExec, void *msg, int len)
{
    STREAM_CONN_S *conn = EXEC_GetUD(hExec, 0);
    StreamServer_Send(conn, msg, len);
    return;
}

static UCHAR _pipecmd_server_get_char(HANDLE hExec)
{
    UCHAR ucCmdChar;
    INT iSocketId = HANDLE_UINT(EXEC_GetUD(hExec, 0));
	UINT uiReadLen;

	if (BS_OK != Socket_Read2(iSocketId, &ucCmdChar, 1, &uiReadLen, 0)) {
        return -1;
    }

    return ucCmdChar;
}

static void _pipecmd_server_send(HANDLE hExec, CHAR *msg)
{
    _pipecmd_server_send_data(hExec, msg, strlen(msg));
}

static int _pipecmd_server_new_conn(void *cfg, STREAM_CONN_S *conn)
{
    HANDLE hExec;
    HANDLE hCmdRunner;

    hCmdRunner = CMD_EXP_CreateRunner(CMD_EXP_RUNNER_TYPE_PIPECMD);
    if (NULL == hCmdRunner) {
        RETURN(BS_NO_MEMORY);
    }
    CmdExp_AltEnable(hCmdRunner, 0);

    hExec = EXEC_Create(_pipecmd_server_send, _pipecmd_server_get_char);
    if (NULL == hExec) {
        CMD_EXP_DestroyRunner(hCmdRunner);
        RETURN(BS_NO_MEMORY);
    }

    EXEC_SetUD(hExec, 0, conn);

    EXEC_Attach(hExec);
    CmdExp_RunnerOutputPrefix(hCmdRunner);

    conn->ud[0] = hCmdRunner;
    conn->ud[1] = hExec;

    return 0;
}

static void _pipecmd_process_line_end(void *cfg, STREAM_CONN_S *conn, int ret)
{
    UCHAR uret[2];

    uret[0] = 0;
    uret[1] = (UCHAR)(CHAR)ret;

    StreamServer_Send(conn, (void*)uret, 2);
    StreamServer_SendFinish(conn);

    return;
}

static int _pipecmd_server_recv_msg(void *cfg, STREAM_CONN_S *conn)
{
    CMD_EXP_RUNNER hRunner = conn->ud[0];
    HANDLE hExec = conn->ud[1];
    VBUF_S *vbuf = &conn->recv_vbuf;
    char *data;
    int data_len;
    int ret;

    data = VBUF_GetData(vbuf);
    data_len = VBUF_GetDataLength(vbuf);
    if (data_len <= 0) {
        return 0;
    }

    EXEC_Attach(hExec);
    ret = CmdExp_RunString(hRunner, data, data_len);

    if ((data[data_len - 1] == '\n') || (data[data_len - 1] == '?')) { /* 输入结束 */
        _pipecmd_process_line_end(cfg, conn, ret);
    }

    VBUF_CutAll(vbuf);

    return 0;
}

static int _pipecmd_server_close_conn(void *cfg, STREAM_CONN_S *conn)
{
    CMD_EXP_RUNNER hRunner = conn->ud[0];
    HANDLE hExec = conn->ud[1];

    conn->ud[0] = NULL;
    conn->ud[1] = NULL;

    if (hExec) {
        EXEC_Delete(hExec);
    }
    if (hRunner) {
        CMD_EXP_DestroyRunner(hRunner);
    }

    return 0;
}

static int _pipecmd_server_conn_event(void *cfg, STREAM_CONN_S *conn, UINT event)
{
    int ret = 0;

    switch (event) {
        case STREAM_SERVER_EVENT_NEW_CONN:
            ret = _pipecmd_server_new_conn(cfg, conn);
            break;
        case STREAM_SERVER_EVENT_RECV_MSG:
            ret = _pipecmd_server_recv_msg(cfg, conn);
            break;
        case STREAM_SERVER_EVENT_CLOSE_CONN:
            ret = _pipecmd_server_close_conn(cfg, conn);
            break;
    }

    return ret;
}

CONSTRUCTOR(init) {
    PipeApp_Init(&g_pipecmd_server, PIPECMD_PIPE_FILE_PATH);
    g_pipecmd_server.server.event_func = _pipecmd_server_conn_event;
}

DESTRUCTOR(fini) {
    PipeApp_Clean(&g_pipecmd_server);
}

int PIPECMDS_Init()
{
    return 0;
}

/* name process-key */
PLUG_API BS_STATUS PIPECMDS_CmdNamePKey(int argc, char **argv)
{
    char *name = ProcessKey_GetKey();
    if ((name == NULL) || (name[0] == '\0')) {
        EXEC_OutInfo(" Have not process key.\r\n");
        return BS_ERR;
    }

    if (g_pipecmd_server.server.enable) {
        EXEC_OutInfo(" The server has enabled, please stop.\r\n");
        return BS_ERR;
    }

    g_pipecmd_server.name_type = PIPE_APP_NAME_TYPE_PKEY;
    scnprintf(g_pipecmd_server.cfg_name, sizeof(g_pipecmd_server.cfg_name), "pipe_%s", name);

    return 0;
}

/* name string %STRING<1-127> */
PLUG_API BS_STATUS PIPECMDS_CmdNameString(int argc, char **argv)
{
    if (g_pipecmd_server.server.enable) {
        EXEC_OutInfo(" The server has enabled, please stop.\r\n");
        return BS_ERR;
    }

    g_pipecmd_server.name_type = PIPE_APP_NAME_TYPE_STRING;
    strlcpy(g_pipecmd_server.cfg_name, argv[2], sizeof(g_pipecmd_server.cfg_name));

    return 0;
}

/* no name */
PLUG_API BS_STATUS PIPECMDS_CmdNoName(int argc, char **argv)
{
    if (g_pipecmd_server.server.enable) {
        EXEC_OutInfo(" The server has enabled, please stop.\r\n");
        return BS_ERR;
    }

    g_pipecmd_server.name_type = PIPE_APP_NAME_TYPE_NONE;
    strlcpy(g_pipecmd_server.cfg_name, PIPECMD_PIPE_FILE_PATH, sizeof(g_pipecmd_server.cfg_name));

    return 0;
}

/* server enable */
PLUG_API BS_STATUS PIPECMDS_CmdEnable(IN UINT ulArgc, IN CHAR ** argv)
{
    int ret;

    ret = PipeApp_Start(&g_pipecmd_server);
    if (ret < 0) {
        EXEC_OutInfo(" Start server failed: %s:%d:%d \r\n",
                ErrCode_GetFileName(), ErrCode_GetLine(), ErrCode_GetErrCode());
    }

    return ret;
}

/* no server enable */
PLUG_API BS_STATUS PIPECMDS_CmdDisable(IN UINT ulArgc, IN CHAR ** argv)
{
    PipeApp_Stop(&g_pipecmd_server);
    return 0;
}

PLUG_API BS_STATUS _pipecmds_SaveCmd (IN HANDLE hFileHandle)
{
    if (g_pipecmd_server.name_type == PIPE_APP_NAME_TYPE_STRING) {
        CMD_EXP_OutputCmd(hFileHandle, "name string %s", g_pipecmd_server.cfg_name);
    }

    if (g_pipecmd_server.name_type == PIPE_APP_NAME_TYPE_PKEY) {
        CMD_EXP_OutputCmd(hFileHandle, "name process-key");
    }

    if (g_pipecmd_server.server.enable) {
        CMD_EXP_OutputCmd(hFileHandle, "server enable");
    }

    return BS_OK;
}

