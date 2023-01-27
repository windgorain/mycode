/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-6-4
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sys_utl.h"
#include "utl/file_utl.h"
#include "utl/txt_utl.h"

#include "sys_os_utl.h"

#ifdef IN_WINDOWS

char * _SYS_OS_GetSelfFileName(void)
{
    static char szFileName[FILE_MAX_PATH_LEN + 1] = "";
    static BOOL_T bExist = FALSE;
    UINT uiLen;

    if (bExist == TRUE)
    {
        return szFileName;
    }

    uiLen = GetModuleFileName(NULL, szFileName, sizeof(szFileName));

    if (uiLen == sizeof(szFileName))
    {
        szFileName[0] = '\0';
        return NULL;
    }

    bExist = TRUE;

    return szFileName;
}

char * _SYS_OS_GetSelfFilePath(void)
{
    static CHAR szFileName[FILE_MAX_PATH_LEN + 1] = "";
    static BOOL_T bExist = FALSE;
    UINT uiLen;
    CHAR *pcSplit;

    if (bExist == TRUE)
    {
        return szFileName;
    }

    uiLen = GetModuleFileName(NULL, szFileName, sizeof(szFileName));

    if (uiLen == sizeof(szFileName))
    {
        szFileName[0] = '\0';
        return NULL;
    }

    pcSplit = TXT_ReverseStrchr(szFileName, '\\');
    if (NULL == pcSplit)
    {
        szFileName[0] = '\0';
        return NULL;
    }

    *pcSplit = '\0';
    bExist = TRUE;

    return szFileName;
}

#endif

