/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2011-7-19
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/exec_utl.h"
#include "utl/passwd_utl.h"
#include "utl/cff_utl.h"
#include "utl/mypoll_utl.h"
#include "utl/local_info.h"
#include "utl/npipe_utl.h"
#include "comp/comp_poller.h"

#define _PIPECMDS_NAME_SIZE 128

enum {
    PIPECMDS_NAME_TYPE_NONE = 0,
    PIPECMDS_NAME_TYPE_STRING,
    PIPECMDS_NAME_TYPE_PKEY
};

typedef struct {
    int name_type;
    char name[_PIPECMDS_NAME_SIZE];
}PIPECMDS_NAME_S;

static void _svr_Ob();

static PIPECMDS_NAME_S g_pipecmds_name;
static int g_ListenFd = -1;
static BOOL_T g_bEnabled = FALSE;
static int g_IsInteractive = 0;
static volatile int g_NeedToClose = 0;
static void *g_PollerIns;
static MYPOLL_HANDLE g_hMypoll = NULL;
static OB_S g_Ob = {.func = _svr_Ob};

static char * _pipecmds_GetName()
{
    if (g_pipecmds_name.name_type == PIPECMDS_NAME_TYPE_NONE) {
        return "file_pipecmd";
    }

    if (g_pipecmds_name.name_type == PIPECMDS_NAME_TYPE_STRING) {
        return g_pipecmds_name.name;
    }

    if (g_pipecmds_name.name_type == PIPECMDS_NAME_TYPE_PKEY) {
        return g_pipecmds_name.name;
    }

    return NULL;
}

static void _svr_Clean()
{
    if (g_ListenFd >= 0) {
        Socket_Close(g_ListenFd);
        g_ListenFd = -1;
    }

    char * name = _pipecmds_GetName();
    unlink(name);
}

static void _svr_Ob()
{
    if (g_NeedToClose) {
        g_NeedToClose = 0;
        MyPoll_Del(g_hMypoll, g_ListenFd);
        MyPoll_Restart(g_hMypoll);
        _svr_Clean();
        g_bEnabled = FALSE;
    }
}

PLUG_API BS_STATUS _pipecmds_SaveCmd (IN HANDLE hFileHandle)
{
    if (g_pipecmds_name.name_type == PIPECMDS_NAME_TYPE_STRING) {
        CMD_EXP_OutputCmd(hFileHandle, "name string %s",
                g_pipecmds_name.name);
    }

    if (g_pipecmds_name.name_type == PIPECMDS_NAME_TYPE_PKEY) {
        CMD_EXP_OutputCmd(hFileHandle, "name process-key");
    }

    if (g_IsInteractive) {
        CMD_EXP_OutputCmd(hFileHandle, "interactive");
    }

    if (g_bEnabled == TRUE) {
        CMD_EXP_OutputCmd(hFileHandle, "server enable");
    }

    return BS_OK;
}

static VOID _svr_Send(HANDLE hExec, CHAR *msg)
{
    INT fd = HANDLE_UINT(EXEC_GetUD(hExec, 0));

    Socket_Write(fd, (UCHAR*)msg, strlen(msg), 0);

    return;
}

static UCHAR _svr_GetChar(HANDLE hExec)
{
    UCHAR ucCmdChar;
    INT iSocketId = HANDLE_UINT(EXEC_GetUD(hExec, 0));
	UINT uiReadLen;

	if (BS_OK != Socket_Read2(iSocketId, &ucCmdChar, 1, &uiReadLen, 0)) {
        return -1;
    }

    return ucCmdChar;
}

static void _svr_CloseConn(int fd, HANDLE hExec, CMD_EXP_RUNNER runner)
{
    EXEC_Delete(hExec);
    CMD_EXP_DestroyRunner(runner);
    MyPoll_Del(g_hMypoll, fd);
    Socket_Close(fd);
}

static BS_WALK_RET_E _svr_SocketEventIn(INT iSocketId, UINT uiEvent,
        USER_HANDLE_S *ud)
{
    CMD_EXP_RUNNER hRunner = ud->ahUserHandle[0];
    HANDLE hExec = ud->ahUserHandle[1];
    UINT uiReadlen;
    char buf[128];

    if (uiEvent & MYPOLL_EVENT_ERR) {
        _svr_CloseConn(iSocketId, hExec, hRunner);
        return BS_WALK_CONTINUE;
    }

    if (uiEvent & MYPOLL_EVENT_IN) {
        if (BS_OK != Socket_Read2(iSocketId, buf, sizeof(buf), &uiReadlen, 0)) {
            _svr_CloseConn(iSocketId, hExec, hRunner);
            return BS_WALK_CONTINUE;
        }

        EXEC_AttachToSelfThread(hExec);

        if (CmdExp_RunString(hRunner, buf, uiReadlen) == BS_STOP) {
            _svr_CloseConn(iSocketId, hExec, hRunner);
            return BS_WALK_CONTINUE;
        }
    }

    return BS_WALK_CONTINUE;
}

static int _svr_NewConn(int fd)
{
    HANDLE hExec;
    HANDLE hCmdRunner;
    USER_HANDLE_S ud;

    Socket_SetNoBlock(fd, TRUE);

    hCmdRunner = CMD_EXP_CreateRunner();
    if (NULL == hCmdRunner) {
        RETURN(BS_NO_MEMORY);
    }
    CmdExp_AltEnable(hCmdRunner, g_IsInteractive);

    hExec = EXEC_Create(_svr_Send, _svr_GetChar);
    if (NULL == hExec) {
        CMD_EXP_DestroyRunner(hCmdRunner);
        RETURN(BS_NO_MEMORY);
    }

    EXEC_SetUD(hExec, 0, UINT_HANDLE(fd));

    EXEC_AttachToSelfThread(hExec);
    CMD_EXP_RunnerStart(hCmdRunner);

    ud.ahUserHandle[0] = hCmdRunner;
    ud.ahUserHandle[1] = hExec;

    MyPoll_SetEvent(g_hMypoll, fd, MYPOLL_EVENT_IN | MYPOLL_EVENT_ERR,
            _svr_SocketEventIn, &ud);

    return 0;
}

static BS_WALK_RET_E _svr_ListenSocketEventIn(IN INT iSocketId,
        IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    INT fd;
    uid_t stUid;

    if (uiEvent & MYPOLL_EVENT_IN) {
        fd = NPIPE_Accept(iSocketId, &stUid);
        if (fd >= 0) {
            if (0 != _svr_NewConn(fd)) {
                Socket_Close(fd);
            }
        }
    }

    return BS_WALK_CONTINUE;
}

static BS_STATUS _svr_Start()
{
    char * name = _pipecmds_GetName();
    if ((NULL == name) || (name[0] == '\0')) {
        EXEC_OutInfo(" Can get name.\r\n");
        RETURN(BS_ERR);
    }

    if (NULL == g_PollerIns) {
        g_PollerIns = PollerComp_Get("vty");
        if (NULL == g_PollerIns) {
            EXEC_OutInfo(" Can get poller.\r\n");
            RETURN(BS_NOT_FOUND);
        }
        PollerComp_RegOb(g_PollerIns, &g_Ob);
    }

    g_hMypoll = PollerComp_GetMyPoll(g_PollerIns);
    g_ListenFd = NPIPE_Listen(name);
    if (g_ListenFd < 0) {
        _svr_Clean();
        RETURN(BS_CAN_NOT_OPEN);
    }

    Socket_SetNoBlock(g_ListenFd, TRUE);

    if (MyPoll_SetEvent(g_hMypoll, g_ListenFd,
                MYPOLL_EVENT_IN | MYPOLL_EVENT_ERR,
                _svr_ListenSocketEventIn, NULL)) {
        _svr_Clean();
        EXEC_OutInfo(" Can not add socket to poll.\r\n");
        RETURN(BS_ERR);
    }

    MyPoll_Trigger(g_hMypoll);

    return 0;
}

/* name process-key */
PLUG_API BS_STATUS PIPECMDS_CmdNamePKey(IN UINT ulArgc, IN CHAR ** argv)
{
    char *name = ProcessKey_GetKey();
    if ((name == NULL) || (name[0] == '\0')) {
        EXEC_OutInfo(" Have not process key.\r\n");
        return BS_ERR;
    }

    if (g_bEnabled) {
        EXEC_OutInfo(" The server has enabled, please stop.\r\n");
        return BS_ERR;
    }

    g_pipecmds_name.name_type = PIPECMDS_NAME_TYPE_PKEY;
    snprintf(g_pipecmds_name.name, _PIPECMDS_NAME_SIZE, "pipe_%s", name);

    return 0;
}

/* name string %STRING<1-127> */
PLUG_API BS_STATUS PIPECMDS_CmdNameString(IN UINT ulArgc, IN CHAR ** argv)
{
    if (g_bEnabled) {
        EXEC_OutInfo(" The server has enabled, please stop.\r\n");
        return BS_ERR;
    }

    g_pipecmds_name.name_type = PIPECMDS_NAME_TYPE_STRING;
    strlcpy(g_pipecmds_name.name, argv[2], _PIPECMDS_NAME_SIZE);

    return 0;
}

/* no name */
PLUG_API BS_STATUS PIPECMDS_CmdNoName(IN UINT ulArgc, IN CHAR ** argv)
{
    if (g_bEnabled) {
        EXEC_OutInfo(" The server has enabled, please stop.\r\n");
        return BS_ERR;
    }

    g_pipecmds_name.name_type = PIPECMDS_NAME_TYPE_NONE;
    return 0;
}

/* server enable */
PLUG_API BS_STATUS PIPECMDS_CmdEnable(IN UINT ulArgc, IN CHAR ** argv)
{
    int ret;

    if (g_bEnabled == TRUE) {
        return 0;
    }

    ret = _svr_Start();
    if (0 != ret) {
        return ret;
    }

    g_bEnabled = TRUE;

    return BS_OK;
}

/* no server enable */
PLUG_API BS_STATUS PIPECMDS_CmdDisable(IN UINT ulArgc, IN CHAR ** argv)
{
    if (g_bEnabled == FALSE) {
        return 0;
    }

    g_NeedToClose = 1;
    PollerComp_Trigger(g_PollerIns);
    return BS_OK;
}

/* [no] interactive */
PLUG_API BS_STATUS PIPECMDS_CmdInteractive(UINT argc, CHAR ** argv, void *env)
{
    if (argv[0][0] == 'n') {
        g_IsInteractive = 0;
    } else {
        g_IsInteractive = 1;
    }

	return BS_OK;
}


PLUG_API int PIPECMDS_Init()
{
    return 0;
}

DESTRUCTOR(fini) {
    _svr_Clean();
}

