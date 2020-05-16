/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-11-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/local_info.h"
#include "utl/sys_utl.h"

/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_SYSINFO

static UINT g_uiSysinfoArgc = 0;
static CHAR **g_ppcSysInfoArgv = NULL;
static CHAR g_szSysInfoInitWorkDir[FILE_MAX_PATH_LEN + 1] = ""; /* 初始工作目录 */

/* 获取可执行文件所在目录 */
CHAR * SYSINFO_GetExePath()
{
    return SYS_GetSelfFilePath();
}

/* 初始工作目录 */
CHAR *SYSINFO_GetInitWorkDir()
{
    return g_szSysInfoInitWorkDir;
}

BS_STATUS SYSINFO_Show(IN UINT ulArgc, IN UCHAR **argv)
{
    CHAR szWorkDir[200] = "";

    FILE_GET_CURRENT_DIRECTORY(szWorkDir, sizeof(szWorkDir));
    
    EXEC_OutInfo(
        "-------------------------------------------------------------\r\n"
        "Init Work Directory: %s\r\n"
        "Work Directory: %s\r\n"
        "Exe Directory: %s\r\n"
        "-------------------------------------------------------------\r\n",
        g_szSysInfoInitWorkDir, szWorkDir, SYS_GetSelfFilePath());

    return BS_OK;
}

BS_STATUS SYSINFO_Init0()
{
    FILE_GET_CURRENT_DIRECTORY(g_szSysInfoInitWorkDir, sizeof(g_szSysInfoInitWorkDir) - 1);

    return BS_OK;
}

BS_STATUS SYSINFO_Init1()
{
    LOCAL_INFO_SetHost("lib/libbs.dll");
    LOCAL_INFO_SetConfPath(".");
    LOCAL_INFO_SetSavePath("./save");
    FUNCTBL_AddFunc(SYSINFO_GetExePath, FUNCTBL_RET_SEQUENCE, "");

    return BS_OK;
}

BS_STATUS SYSINFO_SetArgv(IN UINT uiArgc, IN CHAR **ppcArgv)
{
    g_uiSysinfoArgc = uiArgc;
    g_ppcSysInfoArgv = ppcArgv;

	return BS_OK;
}

