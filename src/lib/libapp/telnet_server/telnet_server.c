/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/process_utl.h"
#include "utl/telnet_server.h"
#include "utl/exec_utl.h"
#include "utl/local_info.h"
#include "utl/passwd_utl.h"
#include "utl/cff_utl.h"
#include "utl/auto_port.h"
#include "utl/mypoll_utl.h"
#include "comp/comp_poller.h"

#define TELNET_SERVICE_MAX 32

typedef struct {
    
    AUTOPORT_S autoport;
    UINT bind_ip;
    int listen_fd;
    UINT enabled:1;
    UINT need_to_close:1;
    UINT interactive:1;
    int muc_id;
}TELNET_SVR_S;

static void _svr_Ob();


static TELNET_SVR_S g_services[TELNET_SERVICE_MAX];
static void *g_PollerIns;
static MYPOLL_HANDLE g_hMypoll = NULL;
static OB_S g_Ob = {.func = _svr_Ob};

static void _svr_Clean(TELNET_SVR_S *svr)
{
    if (svr->listen_fd >= 0) {
        Socket_Close(svr->listen_fd);
        svr->listen_fd = -1;
    }
}

static int _svr_OpenPort(USHORT port, void *ud)
{
    TELNET_SVR_S *svr = ud;

    svr->listen_fd = Socket_Create(AF_INET, SOCK_STREAM);
    if (svr->listen_fd < 0) {
        _svr_Clean(svr);
        RETURN(BS_CAN_NOT_OPEN);
    }

    Socket_SetNoBlock(svr->listen_fd, TRUE);

    if (BS_OK != Socket_Listen(svr->listen_fd,
                htonl(svr->bind_ip), htons(port), 5)) {
        _svr_Clean(svr);
        RETURN(BS_ERR);
    }

    return port;
}

static void _svr_Ob()
{
    int i;

    for (i=0; i<TELNET_SERVICE_MAX; i++) {
        TELNET_SVR_S * svr = &g_services[i];
        if (svr->need_to_close) {
            svr->need_to_close = 0;
            if (svr->enabled) {
                MyPoll_Del(g_hMypoll, svr->listen_fd);
                MyPoll_Restart(g_hMypoll);
                _svr_Clean(svr);
                svr->enabled = 0;
            }
        }
    }
}

static int _telsvr_SvrNeedSave(TELNET_SVR_S *svr)
{
    if (svr->interactive) {
        return 1;
    }

    if (svr->autoport.port_type != AUTOPORT_TYPE_SET) {
        return 1;
    }

    if (svr->autoport.port != 0) {
        return 1;
    }

    if (svr->bind_ip != 0) {
        return 1;
    }

    if (svr->enabled) {
        return 1;
    }

    return 0;
}

static void _telsvr_SaveSvr(HANDLE file, TELNET_SVR_S *svr)
{
    if (svr->interactive) {
        CMD_EXP_OutputCmd(file, "interactive");
    }

    if (svr->muc_id > 0) {
        CMD_EXP_OutputCmd(file, "muc %d", svr->muc_id);
    }

    if (svr->autoport.port_type == AUTOPORT_TYPE_SET) {
        CMD_EXP_OutputCmd(file, "port %u", svr->autoport.port);
    } else if (svr->autoport.port_type == AUTOPORT_TYPE_INC) {
        CMD_EXP_OutputCmd(file, "port range %u %u",
                svr->autoport.v1, svr->autoport.v2);
    } else if (svr->autoport.port_type == AUTOPORT_TYPE_PID) {
        CMD_EXP_OutputCmd(file, "port pid base %u", svr->autoport.v2);
    } else if (svr->autoport.port_type == AUTOPORT_TYPE_ADD) {
        CMD_EXP_OutputCmd(file, "port process-index base %u", svr->autoport.v2);
    } else if (svr->autoport.port_type == AUTOPORT_TYPE_ANY) {
        CMD_EXP_OutputCmd(file, "port any");
    }

    if (svr->bind_ip != 0) {
        CMD_EXP_OutputCmd(file, "ip %s", Socket_IpToName(svr->bind_ip));
    }

    if (svr->enabled) {
        CMD_EXP_OutputCmd(file, "server enable");
    }
}

PLUG_API BS_STATUS _telsvr_SaveCmd(HANDLE hFile)
{
    int i;

    for (i=0; i<TELNET_SERVICE_MAX; i++) {
        TELNET_SVR_S *svr = &g_services[i];
        if (0 == _telsvr_SvrNeedSave(svr)) {
            continue;
        }
        if (0 != CMD_EXP_OutputMode(hFile, "service %d", i)) {
            continue;
        }
        _telsvr_SaveSvr(hFile, svr);
        CMD_EXP_OutputModeQuit(hFile);
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

static void _svr_CloseConn(int fd, HANDLE hExec, TEL_CTRL_S *tels)
{
    EXEC_Delete(hExec);
    CMD_EXP_DestroyRunner(tels->hCmdRunner);
    MEM_Free(tels);
    MyPoll_Del(g_hMypoll, fd);
    Socket_Close(fd);
}

static int _svr_SocketEventIn(INT iSocketId, UINT uiEvent, USER_HANDLE_S *ud)
{
    TEL_CTRL_S *tels = ud->ahUserHandle[0];
    HANDLE hExec = ud->ahUserHandle[1];
    UCHAR buf[128];
    UINT uiReadlen;
    int ret;

    if (uiEvent & MYPOLL_EVENT_ERR) {
        _svr_CloseConn(iSocketId, hExec, tels);
        return 0;
    }

    if (uiEvent & MYPOLL_EVENT_IN) {
        if (BS_OK != Socket_Read2(iSocketId, buf, sizeof(buf), &uiReadlen, 0)) {
            _svr_CloseConn(iSocketId, hExec, tels);
            return 0;
        }

        EXEC_Attach(hExec);
        ret = TELS_Run(tels, buf, uiReadlen);
        if (BS_STOP == ret) {
            _svr_CloseConn(iSocketId, hExec, tels);
            return 0;
        }
    }

    return 0;
}

static int _svr_NewConn(TELNET_SVR_S *svr, int fd)
{
    USER_HANDLE_S ud;
    HANDLE hExec;
    HANDLE hCmdRunner;
    TEL_CTRL_S *tels;

    Socket_SetNoBlock(fd, TRUE);

    hCmdRunner = CMD_EXP_CreateRunner(CMD_EXP_RUNNER_TYPE_TELNET);
    if (NULL == hCmdRunner) {
        RETURN(BS_NO_MEMORY);
    }

    if (svr->interactive) {
        CmdExp_AltEnable(hCmdRunner, 1);
    }

    if (svr->muc_id > 0) {
        CmdExp_SetRunnerMucID(hCmdRunner, svr->muc_id);
        CmdExp_SetRunnerLevel(hCmdRunner, CMD_EXP_LEVEL_MUC);
    }

    hExec = EXEC_Create(_svr_Send, _svr_GetChar);
    if (NULL == hExec) {
        CMD_EXP_DestroyRunner(hCmdRunner);
        RETURN(BS_NO_MEMORY);
    }

    tels = MEM_ZMalloc(sizeof(TEL_CTRL_S));
    if (tels == NULL) {
        CMD_EXP_DestroyRunner(hCmdRunner);
        EXEC_Delete(hExec);
        RETURN(BS_NO_MEMORY);
    }

    EXEC_SetUD(hExec, 0, UINT_HANDLE(fd));
    TELS_Init(tels, fd, hCmdRunner);
    if (svr->interactive) {
        TELS_Hsk(fd);
    }

    EXEC_Attach(hExec);
    CmdExp_RunnerOutputPrefix(hCmdRunner);

    ud.ahUserHandle[0] = tels;
    ud.ahUserHandle[1] = hExec;

    MyPoll_SetEvent(g_hMypoll, fd, MYPOLL_EVENT_IN | MYPOLL_EVENT_ERR,
            _svr_SocketEventIn, &ud);

    return 0;
}

static int _svr_ListenSocketEventIn(int iSocketId, UINT uiEvent, USER_HANDLE_S *ud)
{
    int fd;
    TELNET_SVR_S *svr = ud->ahUserHandle[0];

    if (uiEvent & MYPOLL_EVENT_IN) {
        fd = Socket_Accept(iSocketId, NULL, NULL);
        if (fd >= 0) {
            if (0 != _svr_NewConn(svr, fd)) {
                Socket_Close(fd);
            }
        }
    }

    return 0;
}

static int _svr_InitPoller()
{
    if (! g_PollerIns) {
        g_PollerIns = PollerComp_Get(NULL);
        if (NULL == g_PollerIns) {
            RETURN(BS_NOT_FOUND);
        }
        g_hMypoll = PollerComp_GetMyPoll(g_PollerIns);
        PollerComp_RegOb(g_PollerIns, &g_Ob);
    }

    return 0;
}

static BS_STATUS _svr_Start(TELNET_SVR_S *svr)
{
    _svr_InitPoller();

    int ret = AutoPort_Open(&svr->autoport, _svr_OpenPort, svr);
    if (ret < 0) {
        EXEC_OutInfo(" Can not create server socket.\r\n");
        RETURN(BS_ERR);
    }

    USER_HANDLE_S ud;
    ud.ahUserHandle[0] = svr;

    if (MyPoll_SetEvent(g_hMypoll, svr->listen_fd,
                MYPOLL_EVENT_IN | MYPOLL_EVENT_ERR,
                _svr_ListenSocketEventIn, &ud)) {
        _svr_Clean(svr);
        EXEC_OutInfo(" Can not add socket to poll.\r\n");
        RETURN(BS_ERR);
    }

    MyPoll_Trigger(g_hMypoll);

    return 0;
}

static TELNET_SVR_S * _svr_GetServie(void *env)
{
    UINT index;
    CHAR *pcModeValue = CMD_EXP_GetCurrentModeValue(env);
    TXT_Atoui(pcModeValue, &index);

    return &g_services[index];
}


PLUG_API BS_STATUS TELSVR_CmdService(UINT ulArgc, CHAR **argv, VOID *pEnv)
{
    return BS_OK;
}


PLUG_API BS_STATUS TELSVR_CmdInteractive(UINT argc, CHAR ** argv, void *env)
{
    TELNET_SVR_S *svr = _svr_GetServie(env);
    svr->interactive = 1;
	return BS_OK;
}


PLUG_API BS_STATUS TELSVR_CmdMuc(int argc, char **argv, void *env)
{
    TELNET_SVR_S *svr = _svr_GetServie(env);

    if (argv[0][0] == 'n') {
        svr->muc_id = 0;
    } else {
        svr->muc_id = TXT_Str2Ui(argv[1]);
    }
    return 0;
}


PLUG_API BS_STATUS TELSVR_CmdSetIp(UINT argc, CHAR ** argv, void *env)
{
    TELNET_SVR_S *svr = _svr_GetServie(env);
    svr->bind_ip = Socket_NameToIpHost(argv[1]);
	return BS_OK;
}


PLUG_API BS_STATUS TELSVR_CmdSetPort(UINT ulArgc, CHAR ** argv, void *env)
{
    UINT ulPort = 23;
    TELNET_SVR_S *svr = _svr_GetServie(env);

    TXT_Atoui(argv[1], &ulPort);
    svr->autoport.port_type = AUTOPORT_TYPE_SET;
    svr->autoport.port = ulPort;

	return BS_OK;
}


PLUG_API BS_STATUS TELSVR_CmdSetPortProcessIndex(UINT ulArgc, CHAR ** argv,
        void *env)
{
    UINT base = 10000;
    TELNET_SVR_S *svr = _svr_GetServie(env);

    if (ulArgc >= 4) {
        TXT_Atoui(argv[3], &base);
    }

    svr->autoport.port_type = AUTOPORT_TYPE_ADD;
    svr->autoport.v1 = ProcessKey_GetIndex();
    svr->autoport.v2 = base;

    return 0;
}


PLUG_API BS_STATUS TELSVR_CmdSetPortPID(UINT ulArgc, CHAR ** argv, void *env)
{
    UINT base = 0;
    TELNET_SVR_S *svr = _svr_GetServie(env);

    if (ulArgc >= 4) {
        TXT_Atoui(argv[3], &base);
    }

    svr->autoport.port_type = AUTOPORT_TYPE_PID;
    svr->autoport.v1 = PROCESS_GetPid();
    svr->autoport.v2 = base;

    return 0;
}


PLUG_API BS_STATUS TELSVR_CmdSetPortRange(UINT ulArgc, CHAR ** argv, void *env)
{
    UINT v1 = 0;
    UINT v2 = 0;
    TELNET_SVR_S *svr = _svr_GetServie(env);

    TXT_Atoui(argv[2], &v1);
    TXT_Atoui(argv[3], &v2);

    svr->autoport.port_type = AUTOPORT_TYPE_INC;
    svr->autoport.v1 = v1;
    svr->autoport.v2 = v2;

    return 0;
}


PLUG_API BS_STATUS TELSVR_CmdSetPortAny(UINT ulArgc, CHAR ** argv, void *env)
{
    TELNET_SVR_S *svr = _svr_GetServie(env);

    svr->autoport.port_type = AUTOPORT_TYPE_ANY;

    return 0;
}


PLUG_API BS_STATUS TELSVR_CmdEnable(UINT ulArgc, CHAR ** argv, void *env)
{
    int ret;
    TELNET_SVR_S *svr = _svr_GetServie(env);

    if (svr->enabled) {
        return 0;
    }

    ret = _svr_Start(svr);
    if (0 != ret) {
        return ret;
    }

    svr->enabled = 1;

    return 0;
}


PLUG_API BS_STATUS TELSVR_CmdDisable(UINT ulArgc, CHAR ** argv, void *env)
{
    TELNET_SVR_S *svr = _svr_GetServie(env);

    if (svr->enabled == 0) {
        return 0;
    }

    svr->need_to_close = 1;

    PollerComp_Trigger(g_PollerIns);

    return 0;
}

PLUG_API int TELSVR_Init()
{
    return 0;
}

