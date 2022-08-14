/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2011-7-28
* Description: 
* History:     
******************************************************************************/

#ifndef __CMD_FUNC_H_
#define __CMD_FUNC_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define _DEF_CMD_EXP_MAX_VIEW_NAME_LEN           31
#define _DEF_CMD_EXP_MAX_MODE_NAME_LEN           31

PLUG_API int CMD_EXP_EnterSupper(UINT argc, CHAR **argv, void *env);
PLUG_API int CMD_EXP_ExitApp(UINT argc, CHAR **argv, void *env);
PLUG_API BS_STATUS CMD_EXP_CmdShow(UINT argc, CHAR **argv, VOID *pEnv);
PLUG_API BS_STATUS CMD_EXP_CmdNoDebugAll(IN UINT ulArgc, IN CHAR **pArgv, IN VOID *pEnv);
PLUG_API CMD_EXP_HDL CMD_EXP_GetHdl();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__CMD_FUNC_H_*/


