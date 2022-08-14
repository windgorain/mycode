/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-4-20
* Description: 区间fib,不是以地址掩码来进行组织的,而是区间来组织
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/net.h"
#include "utl/hash_utl.h"
#include "utl/fib_utl.h"
#include "utl/exec_utl.h"
#include "utl/range_fib.h"
#include "utl/mutex_utl.h"
#include "comp/comp_if.h"

typedef struct
{
    DLL_NODE_S stLinkNode;
    FIB_NODE_S stFibNode;
}_RANGE_FIB_NODE_S;

typedef struct
{
    DLL_HEAD_S stListHead;
    BOOL_T bCreateLock;
    MUTEX_S stMutex;
}_RANGE_FIB_CTRL_S;


static VOID  _rangefib_FreeNode(IN VOID *pstNode)
{
    MEM_Free(pstNode);
}

static INT  _rangefib_Cmp(IN DLL_NODE_S *pstNode1, IN DLL_NODE_S *pstNode2, IN HANDLE hUserHandle)
{
    _RANGE_FIB_NODE_S *pstFibHashNode = (_RANGE_FIB_NODE_S*)pstNode1;
    _RANGE_FIB_NODE_S *pstFibToFind = (_RANGE_FIB_NODE_S*)pstNode2;
    INT iCmpRet;

    iCmpRet = pstFibHashNode->stFibNode.stFibKey.uiDstOrStartIp - pstFibToFind->stFibNode.stFibKey.uiDstOrStartIp;
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    iCmpRet = pstFibHashNode->stFibNode.stFibKey.uiMaskOrEndIp - pstFibToFind->stFibNode.stFibKey.uiMaskOrEndIp;
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    return iCmpRet;
}

static inline VOID _rangefib_Lock(IN _RANGE_FIB_CTRL_S *pstFibCtrl)
{
    if (TRUE == pstFibCtrl->bCreateLock)
    {
        MUTEX_P(&pstFibCtrl->stMutex);
    }
}

static inline VOID _rangefib_UnLock(IN _RANGE_FIB_CTRL_S *pstFibCtrl)
{
    if (TRUE == pstFibCtrl->bCreateLock)
    {
        MUTEX_V(&pstFibCtrl->stMutex);
    }
}

static BS_WALK_RET_E _rangefib_ShowEach(IN VOID *pNode, IN VOID * pUserHandle)
{
    _RANGE_FIB_NODE_S *pstFibHashNode = (_RANGE_FIB_NODE_S*)pNode;
    UINT uiStartIP, uiEndIP;
    CHAR szIfName[IF_MAX_NAME_LEN + 1] = "";

    IFNET_Ioctl(pstFibHashNode->stFibNode.uiOutIfIndex, IFNET_CMD_GET_IFNAME, szIfName);

    uiStartIP = htonl(pstFibHashNode->stFibNode.stFibKey.uiDstOrStartIp);
    uiEndIP = htonl(pstFibHashNode->stFibNode.stFibKey.uiMaskOrEndIp);

    EXEC_OutInfo(" %-15pI4 %-15pI4 %-15pI4 %s\r\n",
        &uiStartIP, &uiEndIP,
        &pstFibHashNode->stFibNode.uiNextHop,
        szIfName);

    return BS_WALK_CONTINUE;
}

RANGE_FIB_HANDLE RangeFib_Create(IN BOOL_T bCreateLock)
{
    _RANGE_FIB_CTRL_S *pstFibCtrl;

    pstFibCtrl = MEM_ZMalloc(sizeof(_RANGE_FIB_CTRL_S));
    if (NULL == pstFibCtrl)
    {
        return NULL;
    }

    DLL_INIT(&pstFibCtrl->stListHead);

    if (bCreateLock == TRUE)
    {
        MUTEX_Init(&pstFibCtrl->stMutex);
    }
    pstFibCtrl->bCreateLock = bCreateLock;

    return pstFibCtrl;
}

VOID RangeFib_Destory(IN RANGE_FIB_HANDLE hFibHandle)
{
    _RANGE_FIB_CTRL_S *pstFibCtrl = hFibHandle;

    if (NULL != pstFibCtrl)
    {
        DLL_FREE(&pstFibCtrl->stListHead, _rangefib_FreeNode);
        if (TRUE == pstFibCtrl->bCreateLock)
        {
            MUTEX_Final(&pstFibCtrl->stMutex);
        }
        MEM_Free(pstFibCtrl);
    }
}

BS_STATUS RangeFib_Add(IN RANGE_FIB_HANDLE hFibHandle, IN FIB_NODE_S *pstFibNode)
{
    _RANGE_FIB_CTRL_S *pstFibCtrl = hFibHandle;
    _RANGE_FIB_NODE_S *pstFibNodeNew;
    _RANGE_FIB_NODE_S *pstFibNodeOld;
    _RANGE_FIB_NODE_S stFibToFind;

    if (NULL == pstFibCtrl)
    {
        BS_DBGASSERT(0);
        return (BS_BAD_PARA);
    }

    pstFibNodeNew = MEM_ZMalloc(sizeof(_RANGE_FIB_NODE_S));
    if (NULL == pstFibNodeNew)
    {
        return (BS_NO_MEMORY);
    }

    pstFibNodeNew->stFibNode = *pstFibNode;
    stFibToFind.stFibNode.stFibKey = pstFibNode->stFibKey;

    _rangefib_Lock(pstFibCtrl);
    pstFibNodeOld = DLL_Find(&pstFibCtrl->stListHead, _rangefib_Cmp, (VOID*)&stFibToFind, NULL);
    if (NULL != pstFibNodeOld)
    {
        DLL_DEL(&pstFibCtrl->stListHead, pstFibNodeOld);
    }
    DLL_ADD_TO_HEAD(&pstFibCtrl->stListHead, (VOID*)pstFibNodeNew);
    _rangefib_UnLock(pstFibCtrl);

    if (NULL != pstFibNodeOld)
    {
        MEM_Free(pstFibNodeOld);
    }

    return BS_OK;
}

VOID RangeFib_Del(IN RANGE_FIB_HANDLE hFibHandle, IN FIB_KEY_S *pstFibKey)
{
    _RANGE_FIB_CTRL_S *pstFibCtrl = hFibHandle;
    _RANGE_FIB_NODE_S *pstFibNode;
    _RANGE_FIB_NODE_S stFibToFind;

    if (NULL == pstFibCtrl)
    {
        BS_DBGASSERT(0);
        return;
    }

    stFibToFind.stFibNode.stFibKey = *pstFibKey;

    _rangefib_Lock(pstFibCtrl);
    pstFibNode = DLL_Find(&pstFibCtrl->stListHead, _rangefib_Cmp, (VOID*)&stFibToFind, NULL);
    if (NULL != pstFibNode)
    {
        DLL_DEL(&pstFibCtrl->stListHead, pstFibNode);
    }
    _rangefib_UnLock(pstFibCtrl);

    if (NULL != pstFibNode)
    {
        MEM_Free(pstFibNode);
    }

    return;
}

BS_STATUS RangeFib_Match(IN RANGE_FIB_HANDLE hFibHandle, IN UINT uiDstIp /* 主机序 */, OUT FIB_NODE_S *pstFibNode)
{
    _RANGE_FIB_CTRL_S *pstFibCtrl = hFibHandle;
    _RANGE_FIB_NODE_S *pstNode = NULL;

    if (NULL == pstFibCtrl)
    {
        BS_DBGASSERT(0);
        return (BS_BAD_PARA);
    }

    _rangefib_Lock(pstFibCtrl);
    DLL_SCAN(&pstFibCtrl->stListHead, pstNode)
    {
        if ((uiDstIp >= pstNode->stFibNode.stFibKey.uiDstOrStartIp)
            && (uiDstIp <= pstNode->stFibNode.stFibKey.uiMaskOrEndIp))
        {
            *pstFibNode = pstNode->stFibNode;
            break;
        }
    }
    _rangefib_UnLock(pstFibCtrl);

    if (NULL == pstNode)
    {
        return (BS_NO_SUCH);
    }

    return BS_OK;
}

BS_STATUS RangeFib_Show (IN RANGE_FIB_HANDLE hFibHandle)
{
    _RANGE_FIB_CTRL_S *pstFibCtrl = hFibHandle;

    if (NULL == pstFibCtrl)
    {
        BS_DBGASSERT(0);
        return (BS_BAD_PARA);
    }

    if (DLL_COUNT(&pstFibCtrl->stListHead) == 0)
    {
        return BS_OK;
    }

    EXEC_OutString(" StartIP         EndIP           Nexthop         Ifnet\r\n"
                              "-------------------------------------------------------------------\r\n");

    _rangefib_Lock(pstFibCtrl);
    DLL_WALK(&pstFibCtrl->stListHead, _rangefib_ShowEach, NULL);
    _rangefib_UnLock(pstFibCtrl);

    EXEC_OutString("\r\n");
    
    return BS_OK;
}

