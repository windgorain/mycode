/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-3-26
* Description:   服务进程管理
* History:     
******************************************************************************/
#include <sys/types.h> 
#include <sys/wait.h>
#include "bs.h"
#include "bs/loadbs.h"
#include "bs/loadcmd.h"

#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/lstr_utl.h"
#include "utl/cff_utl.h"
#include "utl/url_lib.h"
#include "utl/time_utl.h"
#include "utl/pidfile_utl.h"
#include "utl/mypoll_utl.h"
#include "utl/daemon_utl.h"
#include "utl/timerfd_utl.h"
#include "utl/vclock_utl.h"
#include "utl/process_utl.h"

#include <sys/un.h>

#define _SCM_MAX_ARGC 120

typedef struct {
    char *name;
    char *path;
    char *param;
    UINT restart:1;
    UINT monitor_sigchld:1;
    UINT dog_enable:1;
    char *pidfile;
    LDATA_S dog_beg_greetings;
    char *dog_beg_mode;
    UINT dog_beg_interval;
    UINT dog_beg_limit;
}SCM_NODE_CFG_S;

typedef struct {
    pid_t pid;
    ULONG last_beg_seconds;
    UINT beg_count;
    int beg_fd;
}SCM_NODE_STATE_S;

typedef struct
{
    DLL_NODE_S stNode;

    SCM_NODE_CFG_S cfg;
    SCM_NODE_STATE_S state;
}SCM_NODE_S;

static DLL_HEAD_S g_stScmListHead = DLL_HEAD_INIT_VALUE(&g_stScmListHead);
static BOOL_T g_bScmRcvTerm = FALSE;
static MYPOLL_HANDLE g_scm_mypoll;


static void scm_CloseBegFd(IN SCM_NODE_S *pstNode)
{
    MyPoll_Del(g_scm_mypoll, pstNode->state.beg_fd);
    Socket_Close(pstNode->state.beg_fd);
    pstNode->state.beg_fd = -1;
}

static VOID scm_Execv(IN CHAR *pcPath, IN CHAR *pcParam)
{
    CHAR *apcArgv[_SCM_MAX_ARGC + 2];
    UINT uiCount;

    if (pcParam == NULL) {
        pcParam = "";
    }

    apcArgv[0] = pcPath;
    uiCount = TXT_StrToToken(pcParam, " ", apcArgv + 1, _SCM_MAX_ARGC);
    apcArgv[uiCount + 1] = NULL;

    printf("Starting %s %s\r\n", pcPath, pcParam);

    execv(pcPath, apcArgv);

    exit(0);
}

static int scm_Start(SCM_NODE_S *pstNode)
{
    pid_t pid;

    if (pstNode->cfg.path == NULL) {
        printf("service %s : Can't get path.\r\n", pstNode->cfg.name);
        return -1;
    }

    pid = fork();
    if (pid < 0)
    {
        printf("service %s : Fork failed.\r\n", pstNode->cfg.name);
        return -1;
    }

    if (pid == 0) {
        scm_Execv(pstNode->cfg.path, pstNode->cfg.param);
    }

    memset(&pstNode->state, 0, sizeof(SCM_NODE_STATE_S));

    pstNode->state.pid = pid;
    pstNode->state.last_beg_seconds = TM_SecondsFromInit();

    return 0;
}

static SCM_NODE_S * scm_CreateNode(IN HANDLE hCff, IN char *pcSection)
{
    SCM_NODE_S *pstNode;
    CHAR *pcFilePath = NULL;
    CHAR *pcParam = NULL;
    UINT restart = 0;
    UINT monitor_sigchld = 0;
    UINT dog_enable = 0;
    CHAR *pidfile = NULL;
    UINT dog_beg_interval = 0;
    UINT dog_beg_limit = 0;
    CHAR *dog_beg_greetings = NULL;
    CHAR *dog_beg_mode = NULL;

    pstNode = MEM_ZMalloc(sizeof(SCM_NODE_S));
    if (NULL == pstNode) {
        printf("service %s : No memory.\r\n", pcSection);
        return NULL;
    }

    CFF_GetPropAsString(hCff, pcSection, "path", &pcFilePath);
    CFF_GetPropAsString(hCff, pcSection, "param", &pcParam);
    CFF_GetPropAsUint(hCff, pcSection, "restart", &restart);
    CFF_GetPropAsUint(hCff, pcSection, "monitor_sigchld", &monitor_sigchld);
    CFF_GetPropAsString(hCff, pcSection, "pidfile", &pidfile);
    CFF_GetPropAsUint(hCff, pcSection, "dog_enable", &dog_enable);
    CFF_GetPropAsUint(hCff, pcSection, "dog_beg_interval", &dog_beg_interval);
    CFF_GetPropAsUint(hCff, pcSection, "dog_beg_limit", &dog_beg_limit);
    CFF_GetPropAsString(hCff, pcSection, "dog_beg_greetings", &dog_beg_greetings);
    CFF_GetPropAsString(hCff, pcSection, "dog_beg_mode", &dog_beg_mode);

    pstNode->cfg.name = TXT_Strdup(pcSection);
    pstNode->cfg.path= TXT_Strdup(pcFilePath);
    pstNode->cfg.param= TXT_Strdup(pcParam);
    pstNode->cfg.restart = restart;
    pstNode->cfg.monitor_sigchld = monitor_sigchld;
    pstNode->cfg.pidfile = TXT_Strdup(pidfile);
    pstNode->cfg.dog_enable = dog_enable;
    pstNode->cfg.dog_beg_interval = dog_beg_interval;
    pstNode->cfg.dog_beg_limit = dog_beg_limit;
    pstNode->cfg.dog_beg_greetings.pucData = (void*)TXT_Strdup(dog_beg_greetings);
    if (pstNode->cfg.dog_beg_greetings.pucData) {
        pstNode->cfg.dog_beg_greetings.uiLen = strlen((char*)pstNode->cfg.dog_beg_greetings.pucData);
    }
    pstNode->cfg.dog_beg_mode = TXT_Strdup(dog_beg_mode);

    DLL_ADD(&g_stScmListHead, pstNode);    

    return pstNode;
}

static void scm_FreeNode(SCM_NODE_S *pstNode)
{
    DLL_DEL(&g_stScmListHead, pstNode);

    if (pstNode->cfg.name) 
        MEM_Free(pstNode->cfg.name);
    if (pstNode->cfg.path) 
        MEM_Free(pstNode->cfg.path);
    if (pstNode->cfg.param) 
        MEM_Free(pstNode->cfg.param);
    if (pstNode->cfg.pidfile) 
        MEM_Free(pstNode->cfg.pidfile);
    if (pstNode->cfg.dog_beg_greetings.pucData) 
        MEM_Free(pstNode->cfg.dog_beg_greetings.pucData);
    if (pstNode->cfg.dog_beg_mode) 
        MEM_Free(pstNode->cfg.dog_beg_mode);
    if (pstNode->state.beg_fd > 0) {
        scm_CloseBegFd(pstNode);
    }

    MEM_Free(pstNode);
}

SCM_NODE_S * scm_GetNodeByPID(IN pid_t pid)
{
    SCM_NODE_S *pstNode;

    DLL_SCAN(&g_stScmListHead, pstNode)
    {
        if (pstNode->state.pid == pid)
        {
            return pstNode;
        }
    }

    return NULL;
}

static pid_t scm_GetPID(SCM_NODE_S *pstNode)
{
    pid_t pid;

    if (pstNode->state.pid > 0) {
        return pstNode->state.pid;
    }

    if (pstNode->cfg.pidfile) {
        pid = PIDFile_ReadPID(pstNode->cfg.pidfile);
        if ((pid > 0) && (PROCESS_IsPidExist(pid))){
            return pid;
        }
    }

    return -1;
}

static int scm_Restart(SCM_NODE_S *pstNode)
{
    pid_t pid = scm_GetPID(pstNode);

    if (pid > 0) {
        kill(pstNode->state.pid, SIGKILL);
    } 

    if (pstNode->state.beg_fd > 0) {
        scm_CloseBegFd(pstNode);
    }

    return scm_Start(pstNode);
}

static BS_WALK_RET_E scm_RecvFood(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    SCM_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];

    pstNode->state.beg_count = 0;

    scm_CloseBegFd(pstNode);

    return BS_WALK_CONTINUE;;
}

static BS_WALK_RET_E scm_Connected(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    SCM_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];

    if (uiEvent & MYPOLL_EVENT_ERR) {
        scm_CloseBegFd(pstNode);
        return BS_WALK_CONTINUE;
    }

    if (Socket_Write(iSocketId, pstNode->cfg.dog_beg_greetings.pucData, pstNode->cfg.dog_beg_greetings.uiLen, 0) < 0) {
        scm_CloseBegFd(pstNode);
        return BS_WALK_CONTINUE;
    }

    MyPoll_SetEvent(g_scm_mypoll, iSocketId, MYPOLL_EVENT_IN, scm_RecvFood, pstUserHandle);

    return BS_WALK_CONTINUE;
}

static void scm_BegTcp(IN SCM_NODE_S *pstNode, URL_FIELD_S *fields)
{
    int fd;
    USER_HANDLE_S user_data;
    char host[128];
    USHORT port;
    UINT ip;

    LSTR_Strlcpy(&fields->host, sizeof(host), host);
    port = LSTR_A2ui(&fields->port);

    ip = Socket_NameToIpHost(host);

    fd = Socket_Create(AF_INET, SOCK_STREAM);
    if (fd < 0) {
        return;
    }

    Socket_SetNoBlock(fd, TRUE);

    user_data.ahUserHandle[0] = pstNode;
    MyPoll_SetEvent(g_scm_mypoll, fd, MYPOLL_EVENT_OUT, scm_Connected, &user_data);
    pstNode->state.beg_fd = fd;

    if (Socket_Connect(fd, ip, port) < 0) {
        scm_CloseBegFd(pstNode);
    }

    return;
}

#if 0 /* 因为编译告警此函数没有用到 */
static void scm_BegUnixStream(IN SCM_NODE_S *pstNode, URL_FIELD_S *fields)
{
    int fd;
    USER_HANDLE_S user_data;
	struct sockaddr_un	un;
    int len;

    fd = Socket_Create(AF_UNIX, SOCK_STREAM);
    if (fd < 0) {
        return;
    }

    Socket_SetNoBlock(fd, 1);

    user_data.ahUserHandle[0] = pstNode;
    MyPoll_SetEvent(g_scm_mypoll, fd, MYPOLL_EVENT_OUT, scm_Connected, &user_data);
    pstNode->state.beg_fd = fd;

	/* fill socket address structure with our address */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	sprintf(un.sun_path, "/var/tmp/%s%05d", pstNode->cfg.name, getpid());
	len = BS_OFFSET(struct sockaddr_un, sun_path) + strlen(un.sun_path);

	unlink(un.sun_path);		/* in case it already exists */
	if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        scm_CloseBegFd(pstNode);
        return;
	}
	if (chmod(un.sun_path, S_IRWXU) < 0) {
        scm_CloseBegFd(pstNode);
        return;
	}

	/* fill socket address structure with server's address */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	memcpy(un.sun_path, fields->host.pcData, fields->host.uiLen);
	len = BS_OFFSET(struct sockaddr_un, sun_path) + fields->host.uiLen;

	if (connect(fd, (struct sockaddr *)&un, len) < 0) {
        scm_CloseBegFd(pstNode);
        return;
	}

    return;
}
#endif

static void scm_Beg(IN SCM_NODE_S *pstNode)
{
    URL_FIELD_S fields;

    /* 关闭上次还未响应的beg请求 */
    if (pstNode->state.beg_fd > 0) {
        scm_CloseBegFd(pstNode);
    }

    if (BS_OK != URL_LIB_ParseUrl(pstNode->cfg.dog_beg_mode, strlen(pstNode->cfg.dog_beg_mode), &fields)) {
        return;
    }

    if (LSTR_StrCaseCmp(&fields.protocol, "tcp") == 0) {
        scm_BegTcp(pstNode, &fields);
    } else if (LSTR_StrCaseCmp(&fields.protocol, "unix_stream") == 0) {
    }

    return;
}

static VOID scm_ProcessSection(IN HANDLE hIni, IN CHAR *pcSection)
{
    SCM_NODE_S *pstNode;
    pid_t pid;

    pstNode = scm_CreateNode(hIni, pcSection);
    if (NULL == pstNode) {
        return;
    }

    pid = scm_Start(pstNode);
    if (pid < 0) {
        scm_FreeNode(pstNode);
        return;
    }

    return;
}

static void scm_RecvSigChld(pid_t pid)
{
    SCM_NODE_S *pstNode;

    pstNode = scm_GetNodeByPID(pid);
    if (NULL == pstNode) {
        return;
    }

    if ((! pstNode->cfg.monitor_sigchld) || (! pstNode->cfg.restart)){
        pstNode->state.pid = -1;
        return;
    }

    scm_Start(pstNode);

    return;
}

static BS_WALK_RET_E scm_ProcessSignal(int signo)
{
    pid_t pid;

    if (g_bScmRcvTerm == TRUE) {
        return BS_WALK_STOP;
    }

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        scm_RecvSigChld(pid);
    }

    return BS_WALK_CONTINUE;
}

static BS_WALK_RET_E scm_Timeout(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    char buf[256];
    SCM_NODE_S *pstNode, *pstTmp;
    ULONG now;

    read(iSocketId, buf, sizeof(buf));

    now = TM_SecondsFromInit();

    DLL_SAFE_SCAN(&g_stScmListHead, pstNode, pstTmp) {
        if (! pstNode->cfg.dog_enable) {
            continue;
        }
        if (pstNode->state.beg_count >= pstNode->cfg.dog_beg_limit) {
            scm_Restart(pstNode);
            continue;
        }
        if ((now - pstNode->state.last_beg_seconds) >= pstNode->cfg.dog_beg_interval) {
            pstNode->state.last_beg_seconds = now;
            pstNode->state.beg_count ++;
            scm_Beg(pstNode);
        }
    }

    return BS_WALK_CONTINUE;
}

static int scm_Init()
{
    int timerfd;

    g_scm_mypoll = MyPoll_Create();
    if (NULL == g_scm_mypoll) {
        return -1;
    }

    MyPoll_SetSignalProcessor(g_scm_mypoll, SIGCHLD, scm_ProcessSignal);

    timerfd = TimerFd_Create(1000, TIMER_FD_FLAG_NOBLOCK);
    if (timerfd < 0) {
        return -1;
    }

    MyPoll_SetEvent(g_scm_mypoll, timerfd, MYPOLL_EVENT_IN, scm_Timeout, NULL);

    return 0;
}

static VOID scm_Schedule()
{
    MyPoll_Run(g_scm_mypoll);
}

static VOID scm_KillAll()
{
    SCM_NODE_S *pstNode;

    DLL_SCAN(&g_stScmListHead, pstNode) {
        if (pstNode->state.pid > 0) {
            kill(pstNode->state.pid, SIGKILL);
            waitpid(pstNode->state.pid, NULL , 0);
        }
    }
}

static VOID scm_SigTerm(IN INT sig)
{
    g_bScmRcvTerm = TRUE;
}

static VOID scm_SetSignal()
{
    sigset_t sigs;
    struct sigaction sa;

    (void) sigfillset(&sigs);
    (void) sigemptyset(&sa.sa_mask);

    sa.sa_flags = 0;
    sa.sa_handler = scm_SigTerm;
    (void) sigdelset(&sigs, SIGTERM);
    (void) sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char* argv[])
{
    CFF_HANDLE hCff;
    CHAR *pcSection;
    char *scm_config_file = "scm.ini";

    if (argc == 1) {
        DAEMON_Init(1,0);
    }

    scm_SetSignal();

    hCff = CFF_INI_Open(scm_config_file, CFF_FLAG_READ_ONLY);
    if (NULL == hCff)
    {
        printf("Can't open scm.ini\r\n");
        return -1;
    }

    scm_Init();

    CFF_SCAN_TAG_START(hCff, pcSection)
    {
        scm_ProcessSection(hCff, pcSection);
    }CFF_SCAN_END();

    CFF_Close(hCff);

//    LoadBs_SetArgv(argc, argv);
//    LoadBs_Init();

    scm_Schedule();

    scm_KillAll();

    return 0;
}

