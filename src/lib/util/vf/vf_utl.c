/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-22
* Description: virtual floor. 虚拟平面
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/rcu_utl.h"
#include "utl/txt_utl.h"
#include "utl/mem_cap.h"
#include "utl/vf_utl.h"
#include "utl/hash_utl.h"

typedef struct
{
    HASH_NODE_S stHashNode;
    CHAR szVfName[VF_MAX_NAME_LEN + 1];
    UINT uiVFID;
}VF_NAME_ID_NODE_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT uiPriority;
    PF_VF_EVENT_FUNC pfEventFunc;
    USER_HANDLE_S stUserHandle;
}VF_EVENT_NODE_S;

typedef struct
{
    RCU_NODE_S stRcuNode;
    VF_P_S stParam;
}VF_DATA_S;

typedef struct
{
    BOOL_T bUsed;
    VF_DATA_S *pstData;
}VF_NODE_S;

typedef struct
{
    UINT uiMaxVD;
    UINT uiRegNodeNum; 
    UINT bCreateLock:1;
    DLL_HEAD_S stListenerList;  
    VF_NODE_S *pstVds; 
    HASH_S * hNameIdTbl;  
    MUTEX_S stMutex;
    void *memcap;
}VF_CTRL_S;

static inline VOID vf_Lock(IN VF_CTRL_S *pstCtrl)
{
    if (pstCtrl->bCreateLock)
    {
        MUTEX_P(&pstCtrl->stMutex);
    }
}

static inline VOID vf_UnLock(IN VF_CTRL_S *pstCtrl)
{
    if (pstCtrl->bCreateLock)
    {
        MUTEX_V(&pstCtrl->stMutex);
    }
}

static BS_STATUS vf_EventNotify(IN VF_CTRL_S *pstCtrl, IN VF_P_S *param, IN UINT uiEvent)
{
    VF_EVENT_NODE_S *pstNode;

    DLL_SCAN(&pstCtrl->stListenerList, pstNode)
    {
        if (NULL != pstNode->pfEventFunc)
        {
            pstNode->pfEventFunc(uiEvent, param, &pstNode->stUserHandle);
        }
    }

    return BS_OK;
}

static UINT vf_NameIdIndex(IN VOID *pNode)
{
    CHAR *pcChar;
    UINT uiIndex = 0;
    VF_NAME_ID_NODE_S *pstNode = pNode;

    pcChar = pstNode->szVfName;

    while ((*pcChar) != '\0')
    {
        uiIndex += (*pcChar);
        pcChar ++;
    }

    return uiIndex;
}

VF_HANDLE VF_Create(VF_PARAM_S *p)
{
    VF_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(VF_CTRL_S) + sizeof(VF_EVENT_NODE_S) * p->uiMaxVD);
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->pstVds = MEM_ZMalloc(sizeof(VF_NODE_S) * p->uiMaxVD);
    if (NULL == pstCtrl->pstVds)
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    pstCtrl->hNameIdTbl = HASH_CreateInstance(p->memcap, 1024, vf_NameIdIndex);
    if (NULL == pstCtrl->hNameIdTbl)
    {
        MEM_Free(pstCtrl->pstVds);
        MEM_Free(pstCtrl);
        return NULL;
    }

    if (p->bCreateLock)
    {
        MUTEX_Init(&pstCtrl->stMutex);
        pstCtrl->bCreateLock = p->bCreateLock;
    }

    DLL_INIT(&pstCtrl->stListenerList);
    pstCtrl->uiMaxVD = p->uiMaxVD;

    return pstCtrl;
}

static VOID vf_InsertListener(IN VF_CTRL_S *pstCtrl, IN VF_EVENT_NODE_S *pstNode)
{
    VF_EVENT_NODE_S *pstTmp;
    
    DLL_SCAN(&pstCtrl->stListenerList, pstTmp)
    {
        if (pstTmp->uiPriority >= pstNode->uiPriority)
        {
            DLL_INSERT_BEFORE(&pstCtrl->stListenerList, pstNode, pstTmp);
            return;
        }
    }

    DLL_ADD(&pstCtrl->stListenerList, pstNode);

    return;
}


UINT VF_RegEventListener
(
    IN VF_HANDLE hVf,
    IN UINT uiPriority,
    IN PF_VF_EVENT_FUNC pfEventFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    VF_CTRL_S *pstCtrl = hVf;
    UINT uiIndex;
    VF_EVENT_NODE_S *pstNode;

    if (NULL == pstCtrl)
    {
        BS_DBGASSERT(0);
        return VF_INVALID_USER_INDEX;
    }

    pstNode = MemCap_ZMalloc(pstCtrl->memcap, sizeof(VF_EVENT_NODE_S));
    if (NULL == pstNode)
    {
        return VF_INVALID_USER_INDEX;
    }

    pstNode->pfEventFunc = pfEventFunc;
    pstNode->uiPriority = uiPriority;
    if (NULL != pstUserHandle)
    {
        pstNode->stUserHandle = *pstUserHandle;
    }

    vf_Lock(pstCtrl);
    vf_InsertListener(pstCtrl, pstNode);
    uiIndex = pstCtrl->uiRegNodeNum;
    pstCtrl->uiRegNodeNum ++;
    vf_UnLock(pstCtrl);

    return uiIndex;
}


UINT VF_CreateVF(IN VF_HANDLE hVf, IN CHAR *pcVfName)
{
    UINT i;
    UINT uiVFID = VF_INVALID_VF;
    VF_CTRL_S *pstCtrl = hVf;
    VF_DATA_S *pstData;
    VF_NAME_ID_NODE_S *pstNameIdNode;

    if (NULL == pstCtrl)
    {
        BS_DBGASSERT(0);
        return VF_INVALID_VF;
    }

    pstNameIdNode = MemCap_ZMalloc(pstCtrl->memcap, sizeof(VF_NAME_ID_NODE_S));
    if (NULL == pstNameIdNode)
    {
        return VF_INVALID_VF;
    }

    pstData = MemCap_ZMalloc(pstCtrl->memcap,
            sizeof(VF_DATA_S) + (sizeof(VOID*) * pstCtrl->uiRegNodeNum));
    if (NULL == pstData)
    {
        MemCap_Free(pstCtrl->memcap, pstNameIdNode);
        return VF_INVALID_VF;
    }

    TXT_Strlcpy(pstNameIdNode->szVfName, pcVfName, VF_MAX_NAME_LEN + 1);
    TXT_Strlcpy(pstData->stParam.szVfName, pcVfName, VF_MAX_NAME_LEN + 1);

    vf_Lock(pstCtrl);
    for (i=0; i<pstCtrl->uiMaxVD; i++)
    {
        if (pstCtrl->pstVds[i].bUsed == FALSE)
        {
            break;
        }
    }

    if (i < pstCtrl->uiMaxVD)
    {
        pstCtrl->pstVds[i].pstData = pstData;
        pstCtrl->pstVds[i].bUsed = TRUE;
        uiVFID = i + 1;
        pstData->stParam.uiVFID = uiVFID;

        pstNameIdNode->uiVFID = uiVFID;
        HASH_Add(pstCtrl->hNameIdTbl, pstNameIdNode);

        vf_EventNotify(pstCtrl, &pstData->stParam, VF_EVENT_CREATE_VF);
    }
    vf_UnLock(pstCtrl);

    if (uiVFID == VF_INVALID_VF)
    {
        MemCap_Free(pstCtrl->memcap, pstNameIdNode);
        MemCap_Free(pstCtrl->memcap, pstData);
    }

    return uiVFID;
}

VOID VF_DestoryVF(IN VF_HANDLE hVf, IN UINT uiVFID)
{
    VF_CTRL_S *pstCtrl = hVf;
    UINT uiVDIndex;
    VF_DATA_S *pstData = NULL;

    if ((NULL == pstCtrl) || (uiVFID == 0))
    {
        BS_DBGASSERT(0);
        return;
    }

    uiVDIndex = uiVFID - 1;

    if (uiVDIndex >= pstCtrl->uiMaxVD)
    {
        BS_DBGASSERT(0);
        return;
    }

    vf_Lock(pstCtrl);
    if (pstCtrl->pstVds[uiVDIndex].bUsed == TRUE)
    {
        vf_EventNotify(pstCtrl, &pstData->stParam, VF_EVENT_DESTORY_VF);

        pstCtrl->pstVds[uiVDIndex].bUsed = FALSE;
        pstData = pstCtrl->pstVds[uiVDIndex].pstData;
        pstCtrl->pstVds[uiVDIndex].pstData = NULL;
        pstData->stParam.uiVFID = VF_INVALID_VF;
        MemCap_Free(pstCtrl->memcap, pstData);
    }
    vf_UnLock(pstCtrl);

    return;
}

VOID VF_SetData(IN VF_HANDLE hVf, IN UINT uiVFID, IN UINT uiUserDataIndex, IN VOID *pData)
{
    VF_CTRL_S *pstCtrl = hVf;
    UINT uiIndex;
    UINT uiVDIndex;

    if ((NULL == pstCtrl) || (uiVFID == VF_INVALID_USER_INDEX) || (uiUserDataIndex == VF_INVALID_USER_INDEX))
    {
        BS_DBGASSERT(0);
        return;
    }

    uiVDIndex = uiVFID - 1;
    uiIndex = uiUserDataIndex;

    if ((uiVDIndex >= pstCtrl->uiMaxVD) || (uiIndex >= pstCtrl->uiRegNodeNum))
    {
        BS_DBGASSERT(0);
        return;
    }

    vf_Lock(pstCtrl);
    if (pstCtrl->pstVds[uiVDIndex].bUsed == TRUE)
    {
        pstCtrl->pstVds[uiVDIndex].pstData->stParam.apUserData[uiIndex] = pData;
    }
    vf_UnLock(pstCtrl);
    
    return;
}

VOID * VF_GetData(IN VF_HANDLE hVf, IN UINT uiVFID, IN UINT uiUserDataIndex)
{
    VF_CTRL_S *pstCtrl = hVf;
    UINT uiIndex;
    UINT uiVDIndex;
    VOID *pUserData = NULL;

    if ((NULL == pstCtrl) || (uiVFID == 0))
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    uiVDIndex = uiVFID - 1;
    uiIndex = uiUserDataIndex;

    if ((uiVDIndex >= pstCtrl->uiMaxVD) || (uiIndex >= pstCtrl->uiRegNodeNum))
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    vf_Lock(pstCtrl);
    if (pstCtrl->pstVds[uiVDIndex].bUsed == TRUE)
    {
        pUserData = pstCtrl->pstVds[uiVDIndex].pstData->stParam.apUserData[uiIndex];
    }
    vf_UnLock(pstCtrl);

    return pUserData;
}

static INT  vf_CmpName(IN VOID * pstHashNode, IN VOID * pstNodeToFind)
{
    VF_NAME_ID_NODE_S *pstNode = pstHashNode;
    VF_NAME_ID_NODE_S *pstNode2Find = pstNodeToFind;

    return strcmp(pstNode->szVfName, pstNode2Find->szVfName);
}

static UINT vf_GetVfIdByName(IN VF_CTRL_S *pstCtrl, IN CHAR *pcName)
{
    VF_NAME_ID_NODE_S stNode;
    VF_NAME_ID_NODE_S *pstFind;

    TXT_Strlcpy(stNode.szVfName, pcName, sizeof(stNode.szVfName));
    
    pstFind = HASH_Find(pstCtrl->hNameIdTbl, vf_CmpName, &stNode);
    if (NULL == pstFind)
    {
        return VF_INVALID_VF;
    }

	return pstFind->uiVFID;
}

UINT VF_GetIDByName(IN VF_HANDLE hVf, IN CHAR *pcName)
{
    VF_CTRL_S *pstCtrl = hVf;
    UINT uiVFID;
    
    vf_Lock(pstCtrl);
    uiVFID = vf_GetVfIdByName(pstCtrl, pcName);
    vf_UnLock(pstCtrl);

    return uiVFID;
}

CHAR * VF_GetNameByID(IN VF_HANDLE hVf, IN UINT uiID)
{
    VF_CTRL_S *pstCtrl = hVf;
    UINT uiVDIndex;

    if ((NULL == pstCtrl) || (uiID == 0))
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    uiVDIndex = uiID - 1;

    if (uiVDIndex >= pstCtrl->uiMaxVD)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    if (pstCtrl->pstVds[uiVDIndex].bUsed != TRUE)
    {
        return NULL;
    }

    return pstCtrl->pstVds[uiVDIndex].pstData->stParam.szVfName;
}

VOID VF_TimerStep(IN VF_HANDLE hVf)
{
    VF_CTRL_S *pstCtrl = hVf;
    UINT i;

    vf_Lock(pstCtrl);
    for (i=0; i<pstCtrl->uiMaxVD; i++)
    {
        if (pstCtrl->pstVds[i].bUsed == FALSE)
        {
            continue;
        }

        vf_EventNotify(pstCtrl, &pstCtrl->pstVds[i].pstData->stParam, VF_EVENT_TIMER);
    }
    vf_UnLock(pstCtrl);
}

CHAR * VF_GetEventName(IN UINT uiEvent)
{
    static CHAR *apcName[VF_EVENT_MAX] = 
    {
        "Create", "Destory", "Timer"
    };

    if (uiEvent >= VF_EVENT_MAX)
    {
        return "";
    }

    return apcName[uiEvent];
}

UINT VF_GetNext(IN VF_HANDLE hVf, IN UINT uiCurrent)
{
    UINT i;
    VF_CTRL_S *pstCtrl = hVf;

    for (i=uiCurrent; i<pstCtrl->uiMaxVD; i++)
    {
        if (pstCtrl->pstVds[i].bUsed == TRUE)
        {
            return i + 1;
        }
    }

    return VF_INVALID_VF;
}

