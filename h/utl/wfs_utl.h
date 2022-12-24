/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-5-6
* Description: 
* History:     
******************************************************************************/

#ifndef __WFS_UTL_H_
#define __WFS_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

HANDLE WFS_Create(IN CHAR *pszFileName, IN UINT uiSsltcpId);
VOID WFS_Destory(IN HANDLE hWfsHandle);
BS_STATUS WFS_Write(IN HANDLE hWfsHandle);
UINT64 WFS_GetWritedLen(IN HANDLE hWfsHandle);
UINT64 WFS_GetRemainLen(IN HANDLE hWfsHandle);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WFS_UTL_H_*/


