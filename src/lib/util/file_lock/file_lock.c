/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-12-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sem_utl.h"
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/num_utl.h"

typedef struct
{
    DLL_NODE_S stLinkNode;
    CHAR szFilePath[FILE_MAX_PATH_LEN + 2];
    SEM_HANDLE hSem;
    UINT  ulRef;
    BOOL_T bIsLocked;
}_FM_FILE_PATH_NODE_S;

static DLL_HEAD_S g_stFmFilePath;
static SEM_HANDLE g_hFmSem;

static inline BOOL_T _FM_LockPathCheckFileName(IN CHAR *pszFilePath)
{
    if (NULL == pszFilePath)
    {
        BS_WARNNING(("Null Ptr!"));
        return FALSE;
    }

    if (pszFilePath[0] == '\0')
    {
        return FALSE;
    }

    if (strlen(pszFilePath) > FILE_MAX_PATH_LEN)
    {
        BS_WARNNING(("Too long file path:%s", pszFilePath));
        return FALSE;
    }

    return TRUE;
}

static BS_STATUS _FM_Scan(IN CHAR *pszFilePath)
{
    UINT ulFilePathLen;
    _FM_FILE_PATH_NODE_S *pstNode;
    UINT ulCmpLen;
    UINT ulPathLen;

    ulFilePathLen = strlen(pszFilePath);

    DLL_SCAN(&g_stFmFilePath, pstNode)
    {
        if (pstNode->bIsLocked == FALSE)
        {
            continue;
        }

        ulPathLen = strlen(pstNode->szFilePath);
        ulCmpLen = MIN(ulFilePathLen, ulPathLen);

        if (strncmp(pszFilePath, pstNode->szFilePath, ulCmpLen) == 0)
        {
            pstNode->ulRef++;
            SEM_V(g_hFmSem);
            SEM_P(pstNode->hSem, BS_WAIT, BS_WAIT_FOREVER);
            SEM_P(g_hFmSem, BS_WAIT, BS_WAIT_FOREVER);
            SEM_V(pstNode->hSem);
            pstNode->ulRef--;
            if (pstNode->ulRef == 0)
            {
                SEM_Destory(pstNode->hSem);
                DLL_DEL(&g_stFmFilePath, pstNode);
                MEM_Free(pstNode);
            }
            RETURN(BS_CONTINUE);
        }
    }

    return BS_OK;
}

static _FM_FILE_PATH_NODE_S * _FM_AddNode(IN CHAR *pszFilePath)
{
    _FM_FILE_PATH_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(_FM_FILE_PATH_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    if (0 == (pstNode->hSem = SEM_CCreate("fm sem", 1)))
    {
        MEM_Free(pstNode);
        return NULL;
    }

    TXT_StrCpy(pstNode->szFilePath, pszFilePath);
    pstNode->ulRef++;
    pstNode->bIsLocked = TRUE;

    DLL_ADD(&g_stFmFilePath, pstNode);

    return pstNode;
}


/* 锁定某个路径/文件 
*/
BS_STATUS FM_LockPath(IN CHAR *pszFilePath, IN BOOL_T bIsDir, OUT HANDLE *phFmLockId)
{
    CHAR szFilePath[FILE_MAX_PATH_LEN + 2];
    UINT ulFilePathLen;
    _FM_FILE_PATH_NODE_S *pstNode;

    if (TRUE != _FM_LockPathCheckFileName(pszFilePath))
    {
        RETURN(BS_BAD_PARA);
    }

    TXT_StrCpy(szFilePath, pszFilePath);    
    FILE_PATH_TO_UNIX(szFilePath);

    ulFilePathLen = strlen(szFilePath);

    /* 如果是目录，但是不以'/'结束,则加上'/' */
    if ((bIsDir == TRUE) && (szFilePath[ulFilePathLen - 1] != '/'))
    {
        szFilePath[ulFilePathLen] = '/';
        szFilePath[ulFilePathLen + 1] = '\0';
        ulFilePathLen++;
    }

    SEM_P(g_hFmSem, BS_WAIT, BS_WAIT_FOREVER);

    while (BS_CONTINUE == RETCODE(_FM_Scan(szFilePath)))
    {
        ;
    }
    
    pstNode = _FM_AddNode(szFilePath);

    SEM_V(g_hFmSem);

    if (NULL == pstNode)
    {
        RETURN(BS_ERR);
    }

    *phFmLockId = pstNode;

    return BS_OK;
}

/* 解锁定某个路径/文件 
*/
BS_STATUS FM_UnLockPath(IN HANDLE hFmLockId)
{
    _FM_FILE_PATH_NODE_S *pstNode;

    pstNode = (_FM_FILE_PATH_NODE_S*)hFmLockId;

    if (NULL == pstNode)
    {
        RETURN(BS_NULL_PARA);
    }

    SEM_P(g_hFmSem, BS_WAIT, BS_WAIT_FOREVER);

    pstNode->ulRef--;
    pstNode->bIsLocked = FALSE;
    
    if (pstNode->ulRef == 0)
    {
        SEM_Destory(pstNode->hSem);
        DLL_DEL(&g_stFmFilePath, pstNode);
        MEM_Free(pstNode);
    }

    SEM_V(g_hFmSem);

    return BS_OK;
}

BS_STATUS FM_Init()
{
    if (0 == (g_hFmSem = SEM_CCreate("Fm ctrl sem", 1)))
    {
        BS_WARNNING(("Can't create sem!"));
        RETURN(BS_ERR);
    }

    DLL_INIT(&g_stFmFilePath);

    return BS_OK;
}
