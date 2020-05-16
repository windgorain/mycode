/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-4-15
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_LOGUTL

#include "bs.h"
    
#include "utl/sprintf_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/time_utl.h"
#include "utl/log_file.h"

typedef struct
{
    CHAR *pszLogFileName;
    FILE *fp;
}_LogFile_CTRL_S;

HANDLE LogFile_Open(IN CHAR *pszFileName)
{
    UINT ulFileNameLen;
    _LogFile_CTRL_S *pstLogCtrl;

    BS_DBGASSERT(NULL != pszFileName);

    ulFileNameLen = strlen(pszFileName);

    pstLogCtrl = MEM_ZMalloc(sizeof(_LogFile_CTRL_S));
    if (NULL == pstLogCtrl)
    {
        return 0;
    }

    pstLogCtrl->pszLogFileName = MEM_Malloc(ulFileNameLen + 1);
    if (NULL == pstLogCtrl->pszLogFileName)
    {
        MEM_Free(pstLogCtrl);
        return 0;
    }

    TXT_StrCpy(pstLogCtrl->pszLogFileName , pszFileName);

    pstLogCtrl->fp = FILE_Open(pszFileName, TRUE, "a");
    if (NULL == pstLogCtrl->fp)
    {
        MEM_Free(pstLogCtrl->pszLogFileName);
        MEM_Free(pstLogCtrl);
        return 0;
    }

    return pstLogCtrl;
}

VOID LogFile_Close(IN HANDLE hLogHandle)
{
    _LogFile_CTRL_S *pstLogCtrl = (_LogFile_CTRL_S *)hLogHandle;

    if (pstLogCtrl != NULL)
    {
        if (pstLogCtrl->pszLogFileName != NULL)
        {
            MEM_Free(pstLogCtrl->pszLogFileName);
        }

        if (pstLogCtrl->fp != NULL)
        {
            fclose(pstLogCtrl->fp);
        }

        MEM_Free(pstLogCtrl);
    }
}

VOID LogFile_OutString(IN HANDLE hLogHandle, IN CHAR *pszLogFmt, ...)
{
    va_list args;

    va_start(args, pszLogFmt);
    LogFile_OutStringByValist(hLogHandle, pszLogFmt, args);
    va_end(args);
}

VOID LogFile_OutStringByValist(IN HANDLE hLogHandle, IN CHAR *pszLogFmt, IN va_list args)
{
    CHAR szInfo[1024];
    CHAR szTimeString[TM_STRING_TIME_LEN + 1];
    _LogFile_CTRL_S *pstLogCtrl = (_LogFile_CTRL_S *)hLogHandle;

    TM_Utc2SimpleString(TM_NowInSec(), szTimeString);
    BS_Sprintf(szInfo, "[%s] ", szTimeString);
    FILE_WriteStr(pstLogCtrl->fp, szInfo);

    vsnprintf(szInfo, sizeof(szInfo), pszLogFmt, args);

    FILE_WriteStr(pstLogCtrl->fp, szInfo);

    fflush(pstLogCtrl->fp);
}

