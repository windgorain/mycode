/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-23
* Description: 
* History:     
******************************************************************************/

#ifndef __SYS_UTL_H_
#define __SYS_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    SYS_OS_VER_OTHER,
    SYS_OS_VER_WIN_OLD,  /* 已经废弃的老系统, 如win32,win98 */
    SYS_OS_VER_WIN2000,
    SYS_OS_VER_WINXP,
    SYS_OS_VER_WIN_SERVER2003,
    SYS_OS_VER_WIN_VISTA,
    SYS_OS_VER_WIN7,
    SYS_OS_VER_WIN8,
    SYS_OS_VER_WIN8_1,

    SYS_OS_VER_WIN_LATTER /* 更新的系统 */
}SYS_OS_VER_E;

typedef enum
{
    SYS_OS_BIT_32,  /* 32位系统 */
    SYS_OS_BIT_64,  /* 32位系统 */

    SYS_OS_BIT_UNKNOWN,  /* 不知道 */
}SYS_OS_BIT_E;

BOOL_T SYS_IsInstanceExist(IN VOID *pszName);
SYS_OS_VER_E SYS_GetOsVer();
SYS_OS_BIT_E SYS_GetOsBit();
CHAR * SYS_GetSelfFileName();
/* 不带有文件名的路径 */
CHAR * SYS_GetSelfFilePath();
/* 设置自启动*/
BS_STATUS SYS_SetSelfStart(IN CHAR *pcRegName, IN BOOL_T bSelfStart, IN char *arg);
void ShowCmdWin(int show);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SYS_UTL_H_*/


