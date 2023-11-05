/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-15
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_TRAYUTL

#include "bs.h"

#include "utl/txt_utl.h"

typedef struct
{
    NOTIFYICONDATA stNd;
}_TRAY_CTRL_S;

HANDLE TRAY_Create(IN HANDLE hWnd, IN UINT ulNotifyMsg, IN UINT uiID)
{
    _TRAY_CTRL_S *pstTray;
    HANDLE hTrayId;

    pstTray = malloc(sizeof(_TRAY_CTRL_S));
    if (NULL == pstTray)
    {
        return 0;
    }
    memset(pstTray, 0, sizeof(_TRAY_CTRL_S));

    hTrayId = pstTray;
    
    pstTray->stNd.cbSize = sizeof(NOTIFYICONDATA);
    pstTray->stNd.uFlags = NIF_ICON|NIF_MESSAGE;
    pstTray->stNd.uID = uiID;
    pstTray->stNd.hWnd = (HWND) hWnd;
    pstTray->stNd.uCallbackMessage = ulNotifyMsg;

    return hTrayId;
}

VOID TRAY_Delete(IN HANDLE hTrayId)
{
    _TRAY_CTRL_S *pstTray;

    if (hTrayId == 0)
    {
        return;
    }

    pstTray = (_TRAY_CTRL_S *)hTrayId;

    if (pstTray->stNd.hIcon != NULL)
    {
        pstTray->stNd.hIcon = NULL;
        Shell_NotifyIcon(NIM_DELETE, &pstTray->stNd);
    }

    free(pstTray);
}

BOOL_T TRAY_SetIcon(IN HANDLE hTrayId, IN HANDLE hIcon, IN CHAR *pszTip)
{
    _TRAY_CTRL_S *pstTray;
    UINT ulCmd;

    if (hTrayId == 0)
    {
        return FALSE;
    }

    pstTray = (_TRAY_CTRL_S *)hTrayId;

    if (pstTray->stNd.hIcon == NULL)
    {
        if (hIcon == 0)
        {
            return TRUE;
        }
        else
        {
            ulCmd = NIM_ADD;
        }
    }
    else
    {
        if (hIcon == 0)
        {
            ulCmd = NIM_DELETE;
        }
        else
        {
            ulCmd = NIM_MODIFY;
        }
    }

    if (pszTip != NULL)
    {
        pstTray->stNd.uFlags |= NIF_TIP;
        TXT_Strlcpy(pstTray->stNd.szTip, pszTip, sizeof(pstTray->stNd.szTip));
    }
    else
    {
        pstTray->stNd.uFlags &= ~NIF_TIP;
        pstTray->stNd.szTip[0] = '\0';
    }

    pstTray->stNd.hIcon = (HICON) hIcon;
    
    if (TRUE != Shell_NotifyIcon(ulCmd, &pstTray->stNd))
    {
        return FALSE;
    }

    return TRUE;
}


