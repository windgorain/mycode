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
#include "utl/txt_utl.h"
#include "utl/stack_utl.h"
#include "utl/exchar_utl.h"
#include "utl/exec_utl.h"

#include "cmd_func.h"

static CMD_EXP_HDL g_cmd_exp = NULL;

#ifndef IN_DEBUG
static int _cmd_exp_PasswdCmdHook(int event, void *data, void *ud, void *env)
{
    char *line = data;

    if (event != CMD_EXP_HOOK_EVENT_LINE) {
        return 0;
    }

    void *runner = CmdExp_GetEnvRunner(env);

    line = TXT_Strim(line);

    if (0 != strcmp(line, "psee-passwd")) {
        CmdExp_QuitMode(runner);
    }

    CmdExp_AltEnable(runner, TRUE);
    CmdExp_SetRunnerHook(runner, NULL, NULL);
    CmdExp_SetRunnerHookMode(runner, FALSE);

    EXEC_OutString("\r\n");
    CmdExp_RunnerOutputPrefix(runner);

    return BS_STOLEN;
}
#endif

int CMD_EXP_EnterSupper(UINT argc, CHAR **argv, void *env)
{
#ifndef IN_DEBUG
    void *runner = CmdExp_GetEnvRunner(env);

    CmdExp_SetRunnerHook(runner, _cmd_exp_PasswdCmdHook, NULL);
    CmdExp_SetRunnerHookMode(runner, TRUE);
    CmdExp_AltEnable(runner, FALSE);
#endif

    return 0;
}

static int _cmd_exp_ExitConfirm(int event, void *data, void *ud, void *env)
{
    char *char_data = data;
    void *runner = CmdExp_GetEnvRunner(env);

    if (event != CMD_EXP_HOOK_EVENT_CHAR) {
        return 0;
    }

    if ((*char_data == 'Y') || (*char_data == 'y')) {
        SYSRUN_Exit(0);
    }

    CmdExp_AltEnable(runner, TRUE);
    CmdExp_SetRunnerHook(runner, NULL, NULL);
    CmdExp_SetRunnerHookMode(runner, FALSE);

    EXEC_OutString("\r\n");
    CmdExp_RunnerOutputPrefix(runner);

    return BS_STOLEN;
}

int CMD_EXP_ExitApp(UINT argc, CHAR **argv, void *env)
{
    if ((argc >= 2) && (0 == strcmp(argv[1], "force"))) {
        SYSRUN_Exit(0);
    }

    void *runner = CmdExp_GetEnvRunner(env);

    CmdExp_SetRunnerHook(runner, _cmd_exp_ExitConfirm, NULL);
    CmdExp_SetRunnerHookMode(runner, TRUE);
    CmdExp_AltEnable(runner, FALSE);

    EXEC_OutString(" Exit the program? y/n:");
    EXEC_Flush();

    return 0;
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

BS_STATUS CMD_EXP_RegSave(char *save_path, CMD_EXP_REG_CMD_PARAM_S *param)
{
    return CmdExp_RegSave(g_cmd_exp, save_path, param);
}

BS_STATUS CMD_EXP_RegEnter(CMD_EXP_REG_CMD_PARAM_S *param)
{
    return CmdExp_RegEnter(g_cmd_exp, param);
}

BS_STATUS CMD_EXP_UnRegSave(char *save_path, char *pcViews)
{
    return CmdExp_UnRegSave(g_cmd_exp, save_path, pcViews);
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
HANDLE CMD_EXP_CreateRunner(UINT type)
{
    return CmdExp_CreateRunner(g_cmd_exp, type);
}

PLUG_API int CMD_EXP_DoCmd(char *cmd)
{
    return CmdExp_DoCmd(g_cmd_exp, cmd);
}

PLUG_API int CMD_EXP_CmdSave(UINT ulArgc, CHAR **pArgv, VOID *pEnv)
{
    return CmdExp_CmdSave(ulArgc, pArgv, pEnv);
}

