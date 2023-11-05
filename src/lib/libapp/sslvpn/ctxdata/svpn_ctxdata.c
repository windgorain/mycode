/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-7-23
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sdc_utl.h"
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/mutex_utl.h"
#include "utl/passwd_utl.h"
#include "utl/local_info.h"
#include "utl/rwlock_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"

typedef struct
{
    RWLOCK_S stRwLock;
    SDC_HANDLE hSdc;
}SVPN_CTXDATA_NODE_S;

typedef struct
{
    SVPN_CTXDATA_NODE_S astCtxDataNodes[SVPN_CONTEXT_DATA_MAX];
}SVPN_CTXDATA_CTRL_S;

CHAR * g_apcSvpnCtxDataFileName[SVPN_CONTEXT_DATA_MAX] = 
{
    "user.ini",
    "admin.ini",
    "acl.ini",
    "role.ini",
    "web_res.ini",
    "tcp_res.ini",
    "ip_res.ini",
    "ip_pool.ini",
    "inner_dns.ini"
};

static inline SVPN_CTXDATA_CTRL_S * svpn_ctxdata_GetCtrl(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    return SVPN_Context_GetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_CTXDATA);
}

static BS_STATUS svpn_ctxdata_ContextCreate(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    CHAR szFilePath[FILE_MAX_PATH_LEN + 1];
    CHAR *pcContextName;
    SDC_HANDLE hUserGroupSdc;
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    UINT i;

    pstCtxDataCtrl = MEM_ZMalloc(sizeof(SVPN_CTXDATA_CTRL_S));
    if (NULL == pstCtxDataCtrl)
    {
        return BS_NO_MEMORY;
    }

    pcContextName = SVPN_Context_GetName(hSvpnContext);

    for (i=0; i<SVPN_CONTEXT_DATA_MAX; i++)
    {
        snprintf(szFilePath, sizeof(szFilePath), "%s/config/%s/%s",
            LOCAL_INFO_GetSavePath(), pcContextName, g_apcSvpnCtxDataFileName[i]);

        hUserGroupSdc = SDC_INI_Create(szFilePath);
        if (NULL == hUserGroupSdc)
        {
            BS_WARNNING(("Can't crate sdc ini file %s", szFilePath));
            break;
        }

        pstCtxDataCtrl->astCtxDataNodes[i].hSdc = hUserGroupSdc;
        RWLOCK_Init(&pstCtxDataCtrl->astCtxDataNodes[i].stRwLock);
    }

    SVPN_Context_SetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_CTXDATA, pstCtxDataCtrl);

    return BS_OK;
}

static BS_STATUS svpn_ctxdata_ContextDestroy(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    CHAR szFilePath[FILE_MAX_PATH_LEN + 1];
    UINT i;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if (NULL == pstCtxDataCtrl)
    {
        return BS_OK;
    }

    for (i=0; i<SVPN_CONTEXT_DATA_MAX; i++)
    {
        if (NULL != pstCtxDataCtrl->astCtxDataNodes[i].hSdc)
        {
            SDC_Destroy(pstCtxDataCtrl->astCtxDataNodes[i].hSdc);
            RWLOCK_Fini(&pstCtxDataCtrl->astCtxDataNodes[i].stRwLock);
        }
    }

    snprintf(szFilePath, sizeof(szFilePath), "%s/config/%s/",
            LOCAL_INFO_GetHostPath(), SVPN_Context_GetName(hSvpnContext));
    FILE_DelDir(szFilePath);

    MEM_Free(pstCtxDataCtrl);

    SVPN_Context_SetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_CTXDATA, NULL);

    return BS_OK;
}

static BS_STATUS svpn_ctxdata_ContextEvent(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case SVPN_CONTEXT_EVENT_CREATE:
        {
            svpn_ctxdata_ContextCreate(hSvpnContext);
            break;
        }

        case SVPN_CONTEXT_EVENT_DESTROY:
        {
            svpn_ctxdata_ContextDestroy(hSvpnContext);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}

BS_STATUS SVPN_CtxData_Init()
{
    SVPN_Context_RegEvent(svpn_ctxdata_ContextEvent);

    return BS_OK;
}

BS_STATUS SVPN_CtxData_AddObject
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcName
)
{
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    BS_STATUS eRet = BS_OK;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) ||
            (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return BS_NOT_INIT;
    }

    RWLOCK_WriteLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    if (SDC_IsTagExist(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcName))
    {
        eRet = BS_ALREADY_EXIST;
    }
    else
    {
        eRet = SDC_AddTag(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcName);
    }
    RWLOCK_WriteUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);

    return eRet;
}

BS_STATUS SVPN_CtxData_GetNextObject
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcCurrentName,
    OUT CHAR *pcNextName,
    IN UINT uiNextTagMaxSize
)
{
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    CHAR *pcNext;
    BS_STATUS eRet = BS_NOT_FOUND;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return BS_NOT_INIT;
    }

    RWLOCK_ReadLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    pcNext = SDC_GetNextTag(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcCurrentName);
    if (NULL != pcNext)
    {
        eRet = BS_OK;
        TXT_Strlcpy(pcNextName, pcNext, uiNextTagMaxSize);
    }
    RWLOCK_ReadUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);

    return eRet;
}

BS_STATUS SVPN_CtxData_SetProp
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcProp,
    IN CHAR *pcPropValue
)
{
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    BS_STATUS eRet = BS_NOT_FOUND;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return BS_NOT_INIT;
    }

    RWLOCK_WriteLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    eRet = SDC_SetProp(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag, pcProp, pcPropValue);
    RWLOCK_WriteUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);

    return eRet;
}

BS_STATUS SVPN_CtxData_GetProp
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcProp,
    OUT CHAR *pcPropValue,
    IN UINT uiPropValueMaxSize
)
{
    CHAR *pcValue;
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    BS_STATUS eRet = BS_NOT_FOUND;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return BS_NOT_INIT;
    }

    RWLOCK_ReadLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    pcValue = SDC_GetProp(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag, pcProp);
    if (NULL != pcValue)
    {
        eRet = BS_OK;
        TXT_Strlcpy(pcPropValue, pcValue, uiPropValueMaxSize);
    }
    RWLOCK_ReadUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);

    return eRet;
}

HSTRING SVPN_CtxData_GetPropAsHString
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcProp
)
{
    HSTRING hString;
    CHAR *pcValue;
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return NULL;
    }

    hString = STRING_Create();
    if (NULL == hString)
    {
        return NULL;
    }

    RWLOCK_ReadLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    pcValue = SDC_GetProp(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag, pcProp);
    if (NULL != pcValue)
    {
        if (BS_OK != STRING_CpyFromBuf(hString, pcValue))
        {
            STRING_Delete(hString);
            hString = NULL;
        }
    }
    RWLOCK_ReadUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);

    return hString;
}

BOOL_T SVPN_CtxData_IsObjectExist(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN SVPN_CTXDATA_E enDataIndex, IN CHAR *pcTag)
{
    BOOL_T bRet = TRUE;
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return FALSE;
    }

    RWLOCK_ReadLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    bRet = SDC_IsTagExist(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag);
    RWLOCK_ReadUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);

    return bRet;
}

VOID SVPN_CtxData_DelObject(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN SVPN_CTXDATA_E enDataIndex, IN CHAR *pcTag)
{
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return;
    }


    RWLOCK_WriteLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    SDC_DeleteTag(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag);
    RWLOCK_WriteUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
}

UINT SVPN_CtxData_GetObjectCount(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN SVPN_CTXDATA_E enDataIndex)
{
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    UINT uiTagNum;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return 0;
    }

    RWLOCK_ReadLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    uiTagNum = SDC_GetTagNum(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc);
    RWLOCK_ReadUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);

    return uiTagNum;
}

static BS_STATUS svpn_ctxdata_AddPropElement
(
    IN SDC_HANDLE hSdc,
    IN CHAR *pcTag,
    IN CHAR *pcPropName,
    IN CHAR *pcPropElement,
    IN CHAR cSplit
)
{
    CHAR *pcValue;
    LSTR_S stLstr;
    LSTR_S stEle;
    CHAR *pcTmp;
    BS_STATUS eRet;

    pcValue = SDC_GetProp(hSdc, pcTag, pcPropName);

    if (NULL == pcValue)
    {
        return SDC_SetProp(hSdc, pcTag, pcPropName, pcPropElement);
    }

    stLstr.pcData = pcValue;
    stLstr.uiLen = strlen(pcValue);
    stEle.pcData = pcPropElement;
    stEle.uiLen = strlen(pcPropElement);

    if (NULL != LSTR_FindElement(&stLstr, cSplit, &stEle))
    {
        return BS_OK;
    }

    pcTmp = MEM_Malloc(stLstr.uiLen + stEle.uiLen + 2);
    if (NULL == pcTmp)
    {
        return BS_NO_MEMORY;
    }

    snprintf(pcTmp, (stLstr.uiLen + stEle.uiLen + 2), "%s%c%s", pcValue, cSplit, pcPropElement);

    eRet = SDC_SetProp(hSdc, pcTag, pcPropName, pcTmp);

    MEM_Free(pcTmp);

    return eRet;
}

BS_STATUS SVPN_CtxData_AddPropElement
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcPropName,
    IN CHAR *pcPropElement,
    IN CHAR cSplit
)
{
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    BS_STATUS eRet = BS_NOT_FOUND;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return BS_NOT_INIT;
    }

    RWLOCK_WriteLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    eRet = svpn_ctxdata_AddPropElement(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag, pcPropName, pcPropElement, cSplit);
    RWLOCK_WriteUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);

    return eRet;
}

VOID SVPN_CtxData_DelPropElement
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcTag,
    IN CHAR *pcPropName,
    IN CHAR *pcPropElement,
    IN CHAR cSplit
)
{
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    CHAR *pcValue;
    LSTR_S stLstr;
    LSTR_S stEle;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return;
    }

    stEle.pcData = pcPropElement;
    stEle.uiLen = strlen(pcPropElement);

    RWLOCK_WriteLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    pcValue = SDC_GetProp(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag, pcPropName);
    if (NULL != pcValue)
    {
        stLstr.pcData = pcValue;
        stLstr.uiLen = strlen(pcValue);
        LSTR_DelElement(&stLstr, cSplit, &stEle);
        stLstr.pcData[stLstr.uiLen] = '\0';
        SDC_SetProp(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag, pcPropName, stLstr.pcData);
    }
    RWLOCK_WriteUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);

    return;
}


VOID SVPN_CtxData_AllDelPropElement
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcPropName,
    IN CHAR *pcPropElement,
    IN CHAR cSplit
)
{
    SVPN_CTXDATA_CTRL_S *pstCtxDataCtrl;
    CHAR *pcTag = NULL;
    CHAR *pcValue;
    LSTR_S stLstr;
    LSTR_S stEle;

    pstCtxDataCtrl = svpn_ctxdata_GetCtrl(hSvpnContext);
    if ((NULL == pstCtxDataCtrl) || (pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc == NULL))
    {
        return;
    }

    stEle.pcData = pcPropElement;
    stEle.uiLen = strlen(pcPropElement);

    RWLOCK_WriteLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
    while (NULL != (pcTag = SDC_GetNextTag(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag)))
    {
        pcValue = SDC_GetProp(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag, pcPropName);
        if (NULL != pcValue)
        {
            stLstr.pcData = pcValue;
            stLstr.uiLen = strlen(pcValue);
            LSTR_DelElement(&stLstr, cSplit, &stEle);
            stLstr.pcData[stLstr.uiLen] = '\0';
            SDC_SetProp(pstCtxDataCtrl->astCtxDataNodes[enDataIndex].hSdc, pcTag, pcPropName, stLstr.pcData);
        }
    }
    RWLOCK_WriteUnLock(&pstCtxDataCtrl->astCtxDataNodes[enDataIndex].stRwLock);
}

