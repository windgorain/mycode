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


#define RETCODE_FILE_NUM RETCODE_FILE_NUM_CMD_MNG

#define _CMD_SAVE_FILE "config.cfg"

static VOID _CMD_MNG_NoExecOut(HANDLE hExec, CHAR *pszInfo)
{
    return;
}

static void cmd_mng_RestoreByFp(HANDLE hCmdRunner, FILE *fp)
{
    char *line;
    char buf[2048];
    int ret = 0;
    int last_failed_level = 100000;

    while ((line = fgets(buf, sizeof(buf), fp)) != NULL) {
        char * find = (void*) TXT_FindFirstNonBlank((void*)line, strlen(line));
        if (find == NULL) {
            continue;
        }
        int cmd_tree_level = (find - line); 
        if (cmd_tree_level > last_failed_level) { 
            continue;
        }
        last_failed_level = 100000;
        ret = CmdExp_RunLine(hCmdRunner, line);
        if (ret != 0) {
            last_failed_level = cmd_tree_level;
        }
    }

    
    CmdExp_Run(hCmdRunner, '\n');
}


BS_STATUS CMD_MNG_CmdRestoreByFile(int muc_id, IN CHAR *pszFileName)
{
    FILE *fp = NULL;
    HANDLE hExecId, hExecIdSaved;
    HANDLE hCmdRunner;

    fp = FOPEN(pszFileName, "rb");
    if (NULL == fp)
    {
        RETURN(BS_CAN_NOT_OPEN);
    }

    if (TRUE == FILE_IsHaveUtf8Bom(fp))
    {
        fseek(fp, 3, SEEK_SET);
    }

    
    hExecIdSaved = EXEC_GetExec();
    hExecId = EXEC_Create(_CMD_MNG_NoExecOut, NULL);
    if (NULL == hExecId)
    {
        fclose(fp);
        RETURN(BS_ERR);
    }
    EXEC_Attach(hExecId);

    hCmdRunner = CMD_EXP_CreateRunner(CMD_EXP_RUNNER_TYPE_NONE);
    if (muc_id > 0) {
        CmdExp_SetRunnerMucID(hCmdRunner, muc_id);
        CmdExp_SetRunnerLevel(hCmdRunner, CMD_EXP_LEVEL_MUC);
    }

    cmd_mng_RestoreByFp(hCmdRunner, fp);

    CMD_EXP_DestroyRunner(hCmdRunner);
    EXEC_Delete(hExecId);
    EXEC_Attach(hExecIdSaved);

    fclose(fp);

    return BS_OK;
}




BS_STATUS CMD_MNG_CmdRestoreSysCmd()
{
    
    CMD_MNG_CmdRestoreByFile(0, _CMD_SAVE_FILE);

    


    return BS_OK;
}

