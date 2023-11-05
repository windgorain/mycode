/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#include "bs.h"
#include "utl/exec_utl.h"

static int _super_passwd_cmd_hook(int event, void *data, void *ud, void *env)
{
    char *line = data;

    if (event != CMD_EXP_HOOK_EVENT_LINE) {
        return 0;
    }

    void *runner = CmdExp_GetEnvRunner(env);

    line = TXT_Strim(line);

    if (0 != strcmp(line, "passwd")) {
        CmdExp_QuitMode(runner);
    }

    CmdExp_AltEnable(runner, TRUE);
    CmdExp_SetRunnerHook(runner, NULL, NULL);
    CmdExp_SetRunnerHookMode(runner, FALSE);

    EXEC_OutString("\r\n");
    CmdExp_RunnerOutputPrefix(runner);

    return BS_STOLEN;
}

static int _super_exit_confirm(int event, void *data, void *ud, void *env)
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

PLUG_API int Super_EnterView(UINT argc, CHAR **argv, void *env)
{
    void *runner = CmdExp_GetEnvRunner(env);

    CmdExp_SetRunnerHook(runner, _super_passwd_cmd_hook, NULL);
    CmdExp_SetRunnerHookMode(runner, TRUE);
    CmdExp_AltEnable(runner, FALSE);

    return 0;
}

int Super_ExitApp(UINT argc, CHAR **argv, void *env)
{
    if ((argc >= 2) && (0 == strcmp(argv[1], "force"))) {
        SYSRUN_Exit(0);
    }

    void *runner = CmdExp_GetEnvRunner(env);

    CmdExp_SetRunnerHook(runner, _super_exit_confirm, NULL);
    CmdExp_SetRunnerHookMode(runner, TRUE);
    CmdExp_AltEnable(runner, FALSE);

    EXEC_OutString(" Exit the program? y/n:");
    EXEC_Flush();

    return 0;
}


int Super_ShutdownSocket(IN UINT ulArgc, IN CHAR **argv)
{
    UINT uiSocketId;

    if (ulArgc < 3)
    {
        RETURN(BS_ERR);
    }

    if (TXT_Atoui(argv[2], &uiSocketId) != BS_OK)
    {
        RETURN(BS_ERR);
    }

    shutdown(uiSocketId, 2);

    return BS_OK;
}


int Super_CmdInit()
{
    return 0;
}

