/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-3-10
* Description: 数据容器
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/lstr_utl.h"
#include "utl/dc_utl.h"
#include "utl/txt_utl.h"
#include "utl/mutex_utl.h"
#include "comp/comp_dc.h"

#define DC_APP_FLAG_HAVE_LOCK 0x1  

typedef struct
{
    DLL_NODE_S stLinkNode;
    DC_TYPE_E eDcType;
    UINT uiFlag;
    MUTEX_S stLock;
    UINT uiRefCount;  
    LSTR_S stFile;
    HANDLE hDc;
}DC_APP_NODE_S;

static DLL_HEAD_S g_stDcAppXmlList = DLL_HEAD_INIT_VALUE(&g_stDcAppXmlList);
static MUTEX_S g_stDcAppLock;

static inline VOID dcapp_LockNode(IN DC_APP_NODE_S *pstNode)
{
    if (pstNode->uiFlag & DC_APP_FLAG_HAVE_LOCK)
    {
        MUTEX_P(&pstNode->stLock);
    }
}

static inline VOID dcapp_UnLockNode(IN DC_APP_NODE_S *pstNode)
{
    if (pstNode->uiFlag & DC_APP_FLAG_HAVE_LOCK)
    {
        MUTEX_V(&pstNode->stLock);
    }
}

static DC_APP_NODE_S * dcapp_FindXmlNode(IN DC_TYPE_E eDcType, IN CHAR *pcFile)
{
    UINT uiFileLen;
    DC_APP_NODE_S *pstNode;

    uiFileLen = strlen(pcFile);

    DLL_SCAN(&g_stDcAppXmlList, pstNode)
    {
        if (pstNode->stFile.uiLen != uiFileLen)
        {
            continue;
        }

        if (0 == strcmp(pstNode->stFile.pcData, pcFile))
        {
            return pstNode;
        }
    }

    return NULL;
}

static DC_APP_NODE_S * dcapp_CreateXmlNode(IN DC_TYPE_E eDcType, IN CHAR *pcFile)
{
    DC_APP_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(DC_APP_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->stFile.pcData = TXT_Strdup(pcFile);
    if (NULL == pstNode->stFile.pcData)
    {
        MEM_Free(pstNode);
        return NULL;
    }
    pstNode->stFile.uiLen = strlen(pcFile);

    pstNode->hDc = DC_OpenInstance(eDcType, pcFile);
    if (NULL == pstNode->hDc)
    {
        MEM_Free(pstNode->stFile.pcData);
        MEM_Free(pstNode);
    }

    MUTEX_Init(&pstNode->stLock);
    pstNode->uiFlag |= DC_APP_FLAG_HAVE_LOCK;

    DLL_ADD(&g_stDcAppXmlList, pstNode);

    return pstNode;
}

static DC_APP_HANDLE dcapp_OpenXml(IN DC_TYPE_E eDcType, IN VOID *pParam)
{
    DC_APP_NODE_S *pstNode;

    MUTEX_P(&g_stDcAppLock);
    pstNode = dcapp_FindXmlNode(eDcType, pParam);
    if (NULL == pstNode)
    {
        pstNode = dcapp_CreateXmlNode(eDcType, pParam);
    }
    if (NULL != pstNode)
    {
        pstNode->uiRefCount ++;
    }
    MUTEX_V(&g_stDcAppLock);

    return pstNode;
}

static VOID dcapp_CloseXml(IN DC_APP_NODE_S *pstNode)
{
    MUTEX_P(&g_stDcAppLock);

    pstNode->uiRefCount --;

    if (pstNode->uiRefCount == 0)
    {
        DLL_DEL(&g_stDcAppXmlList, pstNode);
        MUTEX_Final(&pstNode->stLock);
        MEM_Free(pstNode->stFile.pcData);
        MEM_Free(pstNode);
    }
    
    MUTEX_V(&g_stDcAppLock);

    return;
}

static DC_APP_HANDLE dcapp_OpenMysql(IN DC_TYPE_E eDcType, IN VOID *pParam)
{
    DC_APP_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(DC_APP_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->eDcType = eDcType;

    pstNode->hDc = DC_OpenInstance(eDcType, pParam);
    if (NULL == pstNode->hDc)
    {
        MEM_Free(pstNode);
        return NULL;
    }

    return pstNode;
}

static VOID dcapp_CloseMysql(IN DC_APP_NODE_S *pstNode)
{
    DC_CloseInstance(pstNode->hDc);
    MEM_Free(pstNode);
}

DC_APP_HANDLE DC_APP_Open(IN DC_TYPE_E eDcType, IN VOID *pParam)
{
    DC_APP_HANDLE hDcApp = NULL;

    switch (eDcType)
    {
        case DC_TYPE_MYSQL:
        {
            hDcApp = dcapp_OpenMysql(eDcType, pParam);
            break;
        }

        case DC_TYPE_XML:
        {
            hDcApp = dcapp_OpenXml(eDcType, pParam);
            break;
        }

        default:
        {
            BS_DBGASSERT(0);
            break;
        }
    }

    return hDcApp;
}

VOID DC_APP_Close(IN DC_APP_HANDLE hDcApp)
{
    DC_APP_NODE_S *pstNode = hDcApp;

    switch (pstNode->eDcType)
    {
        case DC_TYPE_MYSQL:
        {
            dcapp_CloseMysql(pstNode);
            break;
        }

        case DC_TYPE_XML:
        {
            dcapp_CloseXml(pstNode);
            break;
        }

        default:
            break;
    }

    return;
}

PLUG_API BS_STATUS  DC_APP_AddTbl
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName
)
{
    DC_APP_NODE_S *pstNode = hDcApp;
    BS_STATUS eRet;

    dcapp_LockNode(pstNode);
    eRet = DC_AddTbl(pstNode->hDc, pcTableName);
    dcapp_UnLockNode(pstNode);

    return eRet;
}

VOID DC_APP_DelTbl
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName
)
{
    DC_APP_NODE_S *pstNode = hDcApp;

    dcapp_LockNode(pstNode);
    DC_DelTbl(pstNode->hDc, pcTableName);
    dcapp_UnLockNode(pstNode);

    return;
}

int DC_APP_AddObject
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    DC_APP_NODE_S *pstNode = hDcApp;
    BS_STATUS eRet;

    dcapp_LockNode(pstNode);
    eRet = DC_AddObject(pstNode->hDc, pcTableName, pstKey);
    dcapp_UnLockNode(pstNode);

    return eRet;
}

BOOL_T DC_APP_IsObjectExist
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    DC_APP_NODE_S *pstNode = hDcApp;
    BOOL_T bRet;

    dcapp_LockNode(pstNode);
    bRet = DC_IsObjectExist(pstNode->hDc, pcTableName, pstKey);
    dcapp_UnLockNode(pstNode);

    return bRet;
}

VOID DC_APP_DelObject
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    DC_APP_NODE_S *pstNode = hDcApp;

    dcapp_LockNode(pstNode);
    DC_DelObject(pstNode->hDc, pcTableName, pstKey);
    dcapp_UnLockNode(pstNode);

    return;
}

BS_STATUS DC_APP_GetFieldValueAsUint
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT UINT *puiValue
)
{
    DC_APP_NODE_S *pstNode = hDcApp;
    BS_STATUS eRet;

    dcapp_LockNode(pstNode);
    eRet = DC_GetFieldValueAsUint(pstNode->hDc, pcTableName, pstKey, pcFieldName, puiValue);
    dcapp_UnLockNode(pstNode);

    return eRet;
}

BS_STATUS DC_APP_CpyFieldValueAsString
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT CHAR *pcValue,
    IN UINT uiValueMaxSize
)
{
    DC_APP_NODE_S *pstNode = hDcApp;
    BS_STATUS eRet;

    dcapp_LockNode(pstNode);
    eRet = DC_CpyFieldValueAsString(pstNode->hDc, pcTableName, pstKey, pcFieldName, pcValue, uiValueMaxSize);
    dcapp_UnLockNode(pstNode);

    return eRet;
}

BS_STATUS DC_APP_SetFieldValueAsUint
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    IN UINT uiValue
)
{
    DC_APP_NODE_S *pstNode = hDcApp;
    BS_STATUS eRet;

    dcapp_LockNode(pstNode);
    eRet = DC_SetFieldValueAsUint(pstNode->hDc, pcTableName, pstKey, pcFieldName, uiValue);
    dcapp_UnLockNode(pstNode);

    return eRet;
}

BS_STATUS DC_APP_SetFieldValueAsString
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    IN CHAR *pcValue
)
{
    DC_APP_NODE_S *pstNode = hDcApp;
    BS_STATUS eRet;

    dcapp_LockNode(pstNode);
    eRet = DC_SetFieldValueAsString(pstNode->hDc, pcTableName, pstKey, pcFieldName, pcValue);
    dcapp_UnLockNode(pstNode);

    return eRet;
}

UINT DC_APP_GetObjectNum
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName
)
{
    DC_APP_NODE_S *pstNode = hDcApp;
    UINT uiRet;

    dcapp_LockNode(pstNode);
    uiRet = DC_GetObjectNum(pstNode->hDc, pcTableName);
    dcapp_UnLockNode(pstNode);

    return uiRet;
}

VOID DC_APP_WalkTable
(
    IN DC_APP_HANDLE hDcApp,
    IN PF_DC_WALK_TBL_CB_FUNC pfWalkFunc,
    IN HANDLE hUserHandle
)
{
    DC_APP_NODE_S *pstNode = hDcApp;

    dcapp_LockNode(pstNode);
    DC_WalkTable(pstNode->hDc, pfWalkFunc, hUserHandle);
    dcapp_UnLockNode(pstNode);

    return;
}

VOID DC_APP_WalkObject
(
    IN DC_APP_HANDLE hDcApp,
    IN CHAR *pcTableName,
    IN PF_DC_WALK_OBJECT_CB_FUNC pfWalkFunc,
    IN HANDLE hUserHandle
)
{
    DC_APP_NODE_S *pstNode = hDcApp;

    dcapp_LockNode(pstNode);
    DC_WalkObject(pstNode->hDc, pcTableName, pfWalkFunc, hUserHandle);
    dcapp_UnLockNode(pstNode);

    return;
}

BS_STATUS DC_APP_Save(IN DC_APP_HANDLE hDcApp)
{
    DC_APP_NODE_S *pstNode = hDcApp;
    BS_STATUS eRet;

    dcapp_LockNode(pstNode);
    eRet = DC_Save(pstNode->hDc);
    dcapp_UnLockNode(pstNode);

    return eRet;
}

BS_STATUS DC_APP_Init()
{
    MUTEX_Init(&g_stDcAppLock);

    return BS_OK;
}


