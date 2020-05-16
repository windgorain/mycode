/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-5-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/cmd_exp.h"
#include "utl/sprintf_utl.h"
#include "utl/passwd_utl.h"
#include "utl/cff_utl.h"
#include "utl/rbuf.h"
#include "utl/mutex_utl.h"
#include "utl/ascii_utl.h"
#include "utl/signal_utl.h"
#include "utl/file_utl.h"
#include "utl/bit_opt.h"
#include "utl/stack_utl.h"
#include "utl/exchar_utl.h"

#include "cmd_func.h"

static CMD_EXP_HDL g_cmd_exp = NULL;

VOID CMD_EXP_ExitApp(UINT argc, CHAR **argv)
{
    CmdExp_ExitApp(argc, argv);
}

/* 初始化命令行注册模块 */
int CMD_EXP_Init()
{
    g_cmd_exp = CmdExp_Create();
    if (! g_cmd_exp) {
        RETURN(BS_NO_MEMORY);
    }

    CmdExp_SetFlag(g_cmd_exp, CMD_EXP_FLAG_LOCK);

    return BS_OK;
}

CMD_EXP_HDL CMD_EXP_GetHdl()
{
    return g_cmd_exp;
}

BS_STATUS CMD_EXP_CmdShow(UINT argc, CHAR **argv, VOID *pEnv)
{
    return CmdExp_CmdShow(argc, argv, pEnv);
}

BS_STATUS CMD_EXP_CmdNoDebugAll(IN UINT ulArgc, IN CHAR **pArgv, IN VOID *pEnv)
{
    return CmdExp_CmdNoDebugAll(ulArgc, pArgv, pEnv);
}

VOID CMD_EXP_RegNoDbgFunc(IN CMD_EXP_NO_DBG_NODE_S *pstNode)
{
    return CmdExp_RegNoDbgFunc(g_cmd_exp, pstNode);
}

BS_STATUS CMD_EXP_RegSave(CHAR *pcFileName, CHAR *pcViews,
        PF_CMD_EXP_SAVE pfSaveFunc)
{
    return CmdExp_RegSave(g_cmd_exp, pcFileName, pcViews, pfSaveFunc);
}

BS_STATUS CMD_EXP_UnRegSave(char *filename, char *pcViews)
{
    return CmdExp_UnRegSave(g_cmd_exp, filename, pcViews);
}

BS_STATUS CMD_EXP_RegCmd(IN CMD_EXP_REG_CMD_PARAM_S *pstParam)
{
    return CmdExp_RegCmd(g_cmd_exp, pstParam);
}

BS_STATUS CMD_EXP_UnregCmd(IN CMD_EXP_REG_CMD_PARAM_S *pstParam)
{
    return CmdExp_UnregCmd(g_cmd_exp, pstParam);
}

BS_STATUS CMD_EXP_RegCmdSimple(char *view, char *cmd,
        PF_CMD_EXP_RUN func, void *ud)
{
    CMD_EXP_REG_CMD_PARAM_S stCmdParam = {0};

    stCmdParam.uiType = DEF_CMD_EXP_TYPE_CMD;
    stCmdParam.pcViews = view;
    stCmdParam.pcCmd = cmd;
    stCmdParam.pfFunc = func;
    stCmdParam.hParam = ud;

    return CMD_EXP_RegCmd(&stCmdParam);
}

int CMD_EXP_UnregCmdSimple(char *view, char *cmd)
{
    CMD_EXP_REG_CMD_PARAM_S stCmdParam = {0};

    stCmdParam.uiType = DEF_CMD_EXP_TYPE_CMD;
    stCmdParam.pcViews = view;
    stCmdParam.pcCmd = cmd;

    return CMD_EXP_UnregCmd(&stCmdParam);
}

/* 创建一个用于运行命令的实例 */
HANDLE CMD_EXP_CreateRunner()
{
    HANDLE runner = CmdExp_CreateRunner(g_cmd_exp);
    return runner;
}

PLUG_API int CMD_EXP_DoCmd(char *cmd)
{
    return CmdExp_DoCmd(g_cmd_exp, cmd);
}

PLUG_API int CMD_EXP_CmdSave(UINT ulArgc, CHAR **pArgv, VOID *pEnv)
{
    return CmdExp_CmdSave(ulArgc, pArgv, pEnv);
}

