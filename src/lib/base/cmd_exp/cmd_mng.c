/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-5
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/stack_utl.h"

#include "cmd_func.h"

/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_CMD_MNG

#define _CMD_SAVE_FILE "config.cfg"

static VOID _CMD_MNG_NoExecOut(HANDLE hExec, CHAR *pszInfo)
{
    return;
}

/* 配置恢复 */
BS_STATUS CMD_MNG_CmdRestoreByFile(IN CHAR *pszFileName)
{
    FILE *fp = NULL;
    HANDLE hExecId, hExecIdSaved;
    HANDLE hCmdRunner;
    UCHAR c;

    fp = FOPEN(pszFileName, "rb");
    if (NULL == fp)
    {
        RETURN(BS_CAN_NOT_OPEN);
    }

    if (TRUE == FILE_IsHaveUtf8Bom(fp))
    {
        fseek(fp, 3, SEEK_SET);
    }

    /* 配置恢复，无需显示信息，将现在任务的EXEC ID覆盖掉, 
            配置恢复完成后，再恢复原来的EXEC ID */
    hExecIdSaved = THREAD_GetExec();
    hExecId = EXEC_Create(_CMD_MNG_NoExecOut, NULL);
    if (NULL == hExecId)
    {
        fclose(fp);
        RETURN(BS_ERR);
    }
    EXEC_AttachToSelfThread(hExecId);

    hCmdRunner = CMD_EXP_CreateRunner();
    CMD_EXP_RunnerStart(hCmdRunner);

    c = (UCHAR)fgetc(fp);

    while (!feof(fp)) {
        CMD_EXP_Run(hCmdRunner, c);
        c = (UCHAR)fgetc(fp);
    }

    /* 加上一个换行,使配置文件最后一条命令没有换行的情况也能执行 */
    CMD_EXP_Run(hCmdRunner, '\n');

    CMD_EXP_DestroyRunner(hCmdRunner);
    EXEC_Delete(hExecId);
    THREAD_SetExec(hExecIdSaved);

    fclose(fp);

    return BS_OK;
}

/*
static VOID _CMD_MNG_ExitHandler(IN INT lExitNum, IN USER_HANDLE_S *pstUserHandle)
{
    CMD_EXP_CmdSave(0, 0, NULL);
}
*/

/* 恢复系统配置 */
BS_STATUS CMD_MNG_CmdRestoreSysCmd()
{
    /* 恢复配置 */
    CMD_MNG_CmdRestoreByFile(_CMD_SAVE_FILE);

    /* 注册关闭触发保存函数 */
/*    SYSRUN_RegExitNotifyFunc(_CMD_MNG_ExitHandler, NULL);  */

    return BS_OK;
}

