/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-2-9
* Description: 
* History:     
******************************************************************************/

#include "bs.h"
#include "bs/loadcmd.h"

#include "utl/sys_utl.h"
#include "utl/exec_utl.h"

#ifdef IN_UNIXLIKE

#include "curses.h"
#include "sys/wait.h"
#include "utl/signal_utl.h"


static BOOL_T g_bLoadCmdIsCursesInit = FALSE;
static UINT g_uiLoadcmdIsRunningCmd = 0;     /* 是否正在运行命令行 */
static UINT g_uiLoadcmdShouldRunningCmd; /* 是否应该运行命令行 */


/* 下面三个函数主要用户Linux下的立即输出 */
BS_STATUS loadcmd_lnx_curses_Init()
{
    if (g_bLoadCmdIsCursesInit == FALSE)
    {
        initscr(); /*   对curses包进行初始化，每个程序中只能调用一次   */
    }

    cbreak(); /*   禁用行缓冲, 打开立即输入模式   */  
    nonl(); /*   回车键不产生'\n'   */  
    noecho(); /*   关闭回显模式，默认情况是打开的。   */ 

    g_bLoadCmdIsCursesInit = TRUE;

    return BS_OK;
}

static BS_STATUS loadcmd_lnx_curses_UnInit()
{
    if (g_bLoadCmdIsCursesInit == TRUE)
    {
        g_bLoadCmdIsCursesInit = FALSE;
        endwin();
    }

    return BS_OK;
}

static VOID loadcmd_lnx_Flush()
{
    if (g_bLoadCmdIsCursesInit == TRUE)
    {
        refresh();
    }
    fflush(stdout);
}

static VOID loadcmd_lnx_Sigioerr(IN int iSigno)
{
    g_uiLoadcmdShouldRunningCmd = 0;
    g_uiLoadcmdIsRunningCmd = 0;
}

static VOID loadcmd_lnx_Sigtstp(IN int iSigno)
{
    sigset_t mask;
    int iStatus;

    if (g_uiLoadcmdShouldRunningCmd == 1)
    {
        g_uiLoadcmdIsRunningCmd = 0;
        g_uiLoadcmdShouldRunningCmd = 0;
        sigemptyset(&mask);
        sigaddset(&mask, SIGTSTP);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        signal(SIGTSTP, SIG_DFL);
        if (fork() == 0)
        {
            kill(getppid(), SIGCONT);
            exit(0);
        }
        else
        {
            kill(getpid(), SIGTSTP);
            wait(&iStatus);
            SIGNAL_Set(SIGTSTP, 0, (VOID *)loadcmd_lnx_Sigtstp);
        }
    }
    else
    {
        g_uiLoadcmdShouldRunningCmd = 1;
    }
}

static VOID loadcmd_lnx_ExitNotify(IN INT lExitNum, IN USER_HANDLE_S *pstUserHandle)
{
    loadcmd_lnx_curses_UnInit();
}

static VOID loadcmd_lnx_Print(HANDLE hExec, CHAR *pszInfo)
{
    printf("%s", pszInfo);
    loadcmd_lnx_Flush();
}

static UCHAR loadcmd_lnx_Getch(HANDLE hExec)
{
    return getch();
}

VOID Load_Cmd(IN UINT uiRunForLinux /* 0 - 只准备好但不加载命令行. 1 - 加载命令行*/)
{
    UCHAR ucCmdChar;
    HANDLE hHandle;
    CMD_EXP_RUNNER hCmdRunner;

    if (uiRunForLinux == 1) {
        loadcmd_lnx_curses_Init();
    }

    hCmdRunner = CMD_EXP_CreateRunner();
    if (! hCmdRunner) {
        BS_WARNNING(("Can't create cmd exp handle!"));
        return;
    }
    CmdExp_AltEnable(hCmdRunner, 1);

    hHandle = EXEC_Create(loadcmd_lnx_Print, loadcmd_lnx_Getch);
    if (hHandle == NULL) {
        CMD_EXP_DestroyRunner(hCmdRunner);
        return;
    }
    EXEC_AttachToSelfThread(hHandle);
    CMD_EXP_RunnerStart(hCmdRunner);

    SYSRUN_RegExitNotifyFunc(loadcmd_lnx_ExitNotify, NULL);

    SIGNAL_Set(SIGTTIN, 0, (VOID *)loadcmd_lnx_Sigioerr);
    SIGNAL_Set(SIGTTOU, 0, (VOID *)loadcmd_lnx_Sigioerr);
    SIGNAL_Set(SIGTSTP, 0, (VOID *)loadcmd_lnx_Sigtstp);

    g_uiLoadcmdShouldRunningCmd = uiRunForLinux;
    g_uiLoadcmdIsRunningCmd = uiRunForLinux;

    for (;;)
    {
        while (g_uiLoadcmdShouldRunningCmd == 0)
        {
            Sleep(99999);
        }
        if ((g_uiLoadcmdShouldRunningCmd == 1) && (g_uiLoadcmdIsRunningCmd == 0))
        {
            g_uiLoadcmdIsRunningCmd = 1;
            loadcmd_lnx_curses_Init();
            CMD_EXP_RunnerStart(hCmdRunner);
        }
        ucCmdChar = (UCHAR)getch();
        if (g_uiLoadcmdShouldRunningCmd == 0)
        {
            g_uiLoadcmdIsRunningCmd = 0;
            continue;
        }

        if (BS_STOP == CMD_EXP_Run(hCmdRunner, ucCmdChar)) {
            break;
        }

        loadcmd_lnx_Flush();
    }

    EXEC_Delete(hHandle);
    CMD_EXP_DestroyRunner(hCmdRunner);

    loadcmd_lnx_curses_UnInit();

    return;
}

VOID Load_Cmd_Fini()
{
    loadcmd_lnx_curses_UnInit();
}

#endif


