/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-2-9
* Description: 
* History:     
******************************************************************************/

#include "bs.h"
#include "bs/loadcmd.h"

#include "utl/sys_utl.h"

#ifdef IN_WINDOWS

static VOID loadcmd_win_Print(HANDLE hExec, CHAR *pszInfo)
{
    printf("%s", pszInfo);
}

static UCHAR loadcmd_win_Getch(HANDLE hExec)
{
    return getch();
}

VOID Load_Cmd(IN UINT uiRunForLinux )
{
    UCHAR ucCmdChar;
    HANDLE hHandle;
    HANDLE hCmdRunner;

    hCmdRunner= CMD_EXP_CreateRunner(CMD_EXP_RUNNER_TYPE_CMD);
    if (hCmdRunner == NULL) {
        BS_WARNNING(("Can't create cmd exp handle!"));
        return;
    }

    CmdExp_AltEnable(hCmdRunner, 1);

    hHandle = EXEC_Create(loadcmd_win_Print, loadcmd_win_Getch);
    if (hHandle == NULL)
    {
        BS_WARNNING(("Can't create cmd exp handle!"));
        return;
    }
    EXEC_Attach(hHandle);
    CmdExp_RunnerOutputPrefix(hCmdRunner);

    for (;;)
    {
        ucCmdChar = (UCHAR)getch();

        if (BS_STOP == RETCODE(CmdExp_Run(hCmdRunner, ucCmdChar))) {
            break;
        }
    }

    EXEC_Delete(hHandle);
    CMD_EXP_DestroyRunner(hCmdRunner);

    return;
}


#endif

