/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-9-30
* Description: 
* History:     
*******************************************************************************
注意,本模块由外部使用者保证使用和删除之间的互斥性. 确保不使用了才能删除实例.
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_FIBUTL

#include "bs.h"

#include "utl/sprintf_utl.h"
#include "utl/net.h"
#include "utl/hash_utl.h"
#include "utl/fib_utl.h"
#include "utl/exec_utl.h"
#include "utl/mutex_utl.h"
#include "utl/bit_opt.h"
#include "comp/comp_if.h"

#define _FIB_HASH_BUCKET_NUM 1024

typedef struct
{
    HASH_S * hHash;
    UINT uiInstanceFlag;
    MUTEX_S stMutex;
}_FIB_CTRL_S;

typedef struct
{
    UINT uiNextHop; 
    IF_INDEX uiOutIfIndex;
    UINT uiFlag;
}_FIB_NEXT_HOP_S;

typedef struct
{
    HASH_NODE_S stHashNode;
    FIB_KEY_S stFibKey;
    _FIB_NEXT_HOP_S stNextHop;
    _FIB_NEXT_HOP_S astStaticNextHop[FIB_MAX_NEXT_HOP_NUM];  
    UINT uiStaticNextHopCount;
}_FIB_HASH_NODE_S;

static UINT _fib_HashIndex(IN VOID *pstHashNode)
{
    _FIB_HASH_NODE_S *pstFibHashNode = pstHashNode;
    UINT uiDstIp;

    uiDstIp = pstFibHashNode->stFibKey.uiDstOrStartIp;
    uiDstIp = ntohl(uiDstIp);

    return uiDstIp;
}

static VOID  fib_FreeHashNode(IN void * hHashId, IN VOID *pstNode, IN VOID * pUserHandle)
{
    _FIB_HASH_NODE_S *pstFibHashNode = (_FIB_HASH_NODE_S*)pstNode;

    MEM_Free(pstFibHashNode);
}

static INT  fib_HashCmp(IN VOID *pstHashNode, IN VOID *pstNodeToFind)
{
    _FIB_HASH_NODE_S *pstFibHashNode = pstHashNode;
    _FIB_HASH_NODE_S *pstToFind = pstNodeToFind;
    INT iCmpRet;

    iCmpRet = pstFibHashNode->stFibKey.uiDstOrStartIp - pstToFind->stFibKey.uiDstOrStartIp;
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    iCmpRet = pstFibHashNode->stFibKey.uiMaskOrEndIp - pstToFind->stFibKey.uiMaskOrEndIp;
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    return iCmpRet;
}

static int fib_WalkEach(IN HASH_S * hHashId, IN HASH_NODE_S *pstNode, IN VOID * pUserHandle)
{
    _FIB_HASH_NODE_S *pstFibHashNode = (_FIB_HASH_NODE_S*)pstNode;
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_FIB_WALK_FUNC pfWalkFunc = pstUserHandle->ahUserHandle[0];
    FIB_NODE_S stFibNode;
    int ret = 0;
    UINT i;

    stFibNode.stFibKey = pstFibHashNode->stFibKey;
    
    if (pstFibHashNode->stNextHop.uiOutIfIndex != IF_INVALID_INDEX)
    {
        stFibNode.uiNextHop = pstFibHashNode->stNextHop.uiNextHop;
        stFibNode.uiOutIfIndex = pstFibHashNode->stNextHop.uiOutIfIndex;
        stFibNode.uiFlag = pstFibHashNode->stNextHop.uiFlag;
        ret = pfWalkFunc(&stFibNode, pstUserHandle->ahUserHandle[1]);
        if (ret < 0) {
            return ret;
        }
    }

    for (i=0; i<pstFibHashNode->uiStaticNextHopCount; i++)
    {
        stFibNode.uiNextHop = pstFibHashNode->astStaticNextHop[i].uiNextHop;
        stFibNode.uiOutIfIndex = pstFibHashNode->astStaticNextHop[i].uiOutIfIndex;
        stFibNode.uiFlag = pstFibHashNode->astStaticNextHop[i].uiFlag;
        ret = pfWalkFunc(&stFibNode, pstUserHandle->ahUserHandle[1]);
        if (ret < 0) {
            break;
        }
    }

    return ret;
}

static VOID fib_ShowEach(IN FIB_NODE_S *pstFibNode)
{
    UINT uiMask;
    CHAR szIfName[IF_MAX_NAME_LEN + 1] = "";
    CHAR szTmp[24];
    CHAR szFlag[16];
    
    uiMask = ntohl(pstFibNode->stFibKey.uiMaskOrEndIp);
    BS_Sprintf(szTmp, "%pI4/%d", &pstFibNode->stFibKey.uiDstOrStartIp, MASK_2_PREFIX(uiMask));
    FIB_GetFlagString(pstFibNode->uiFlag, szFlag);

    IFNET_Ioctl(pstFibNode->uiOutIfIndex, IFNET_CMD_GET_IFNAME, szIfName);

    EXEC_OutInfo(" %-18s %-15pI4 %-6s %s\r\n",
        szTmp, &pstFibNode->uiNextHop, szFlag, szIfName);

    return;
}

static inline VOID fib_Lock(IN _FIB_CTRL_S *pstFibCtrl)
{
    if (pstFibCtrl->uiInstanceFlag & FIB_INSTANCE_FLAG_CREATE_LOCK)
    {
        MUTEX_P(&pstFibCtrl->stMutex);
    }
}

static inline VOID fib_UnLock(IN _FIB_CTRL_S *pstFibCtrl)
{
    if (pstFibCtrl->uiInstanceFlag & FIB_INSTANCE_FLAG_CREATE_LOCK)
    {
        MUTEX_V(&pstFibCtrl->stMutex);
    }
}

static int _fib_ClearAutoIFEach(IN HASH_S * hHashId, IN HASH_NODE_S *pstNode, IN VOID * pUserHandle)
{
    _FIB_HASH_NODE_S *pstFibHashNode = (_FIB_HASH_NODE_S*)pstNode;
    UINT i;
    
    for (i=0; i<pstFibHashNode->uiStaticNextHopCount; i++)
    {
        if (pstFibHashNode->astStaticNextHop[i].uiFlag & FIB_FLAG_AUTO_IF)
        {
            pstFibHashNode->astStaticNextHop[i].uiOutIfIndex = IF_INVALID_INDEX;
        }
    }

    return 0;
}

static VOID _fib_CleareAutoIF(IN _FIB_CTRL_S *pstFibCtrl)
{
    HASH_Walk(pstFibCtrl->hHash, (PF_HASH_WALK_FUNC)_fib_ClearAutoIFEach, NULL);
}

BS_STATUS _fib_Add(IN _FIB_CTRL_S *pstFibCtrl, IN FIB_NODE_S *pstFibNode)
{
    _FIB_HASH_NODE_S *pstNode;
    _FIB_HASH_NODE_S stToFind;

    stToFind.stFibKey = pstFibNode->stFibKey;

    pstNode = (_FIB_HASH_NODE_S*) HASH_Find(pstFibCtrl->hHash, fib_HashCmp, &stToFind);
    if (NULL == pstNode)
    {
        pstNode = MEM_ZMalloc(sizeof(_FIB_HASH_NODE_S));
        if (NULL == pstNode)
        {
            RETURN(BS_NO_MEMORY);
        }
        pstNode->stFibKey = pstFibNode->stFibKey;
        HASH_Add(pstFibCtrl->hHash, (HASH_NODE_S*)pstNode);
    }

    if (! BIT_ISSET(pstFibNode->uiFlag, FIB_FLAG_STATIC))
    {
        pstNode->stNextHop.uiNextHop = pstFibNode->uiNextHop;
        pstNode->stNextHop.uiOutIfIndex = pstFibNode->uiOutIfIndex;
        pstNode->stNextHop.uiFlag = pstFibNode->uiFlag;

        if (pstFibNode->uiFlag & FIB_FLAG_DIRECT)
        {
            _fib_CleareAutoIF(pstFibCtrl);
        }

        return BS_OK;
    }

    if (pstNode->uiStaticNextHopCount >= FIB_MAX_NEXT_HOP_NUM)
    {
        RETURN(BS_FULL);
    }

    pstNode->astStaticNextHop[pstNode->uiStaticNextHopCount].uiNextHop = pstFibNode->uiNextHop;
    pstNode->astStaticNextHop[pstNode->uiStaticNextHopCount].uiOutIfIndex = pstFibNode->uiOutIfIndex;
    pstNode->astStaticNextHop[pstNode->uiStaticNextHopCount].uiFlag = pstFibNode->uiFlag;
    pstNode->uiStaticNextHopCount ++;

    return BS_OK;
}

static VOID _fib_DelStaticFib(IN _FIB_HASH_NODE_S *pstFibHashNode, IN FIB_NODE_S *pstFibNode)
{
    UINT i;

    if (pstFibNode->uiNextHop == 0)
    {
        pstFibHashNode->uiStaticNextHopCount = 0;
    }
    else
    {
        for (i=0; i<pstFibHashNode->uiStaticNextHopCount; i++)
        {
            if (pstFibHashNode->astStaticNextHop[i].uiNextHop == pstFibNode->uiNextHop)
            {
                pstFibHashNode->uiStaticNextHopCount --;
                pstFibHashNode->astStaticNextHop[i].uiNextHop
                    = pstFibHashNode->astStaticNextHop[pstFibHashNode->uiStaticNextHopCount].uiNextHop;
                break;
            }
        }
    }

    return;
}

static inline VOID _fib_DelNormalFib(IN _FIB_CTRL_S *pstFibCtrl, IN _FIB_HASH_NODE_S *pstFibHashNode, IN FIB_NODE_S *pstFibNode)
{
    UINT uiFlag;

    uiFlag = pstFibHashNode->stNextHop.uiFlag;
    
    pstFibHashNode->stNextHop.uiNextHop = 0;
    pstFibHashNode->stNextHop.uiOutIfIndex = IF_INVALID_INDEX;
    pstFibHashNode->stNextHop.uiFlag = 0;

    if (uiFlag & FIB_FLAG_DIRECT)
    {
        _fib_CleareAutoIF(pstFibCtrl);
    }

    return;
}

static VOID _fib_Del(IN _FIB_CTRL_S *pstFibCtrl, IN FIB_NODE_S *pstFibNode)
{
    _FIB_HASH_NODE_S *pstFibHashNode;
    _FIB_HASH_NODE_S stToFind;

    stToFind.stFibKey = pstFibNode->stFibKey;

    pstFibHashNode = (_FIB_HASH_NODE_S*) HASH_Find(pstFibCtrl->hHash, fib_HashCmp, &stToFind);
    if (NULL == pstFibHashNode)
    {
        return;
    }

    if (pstFibNode->uiFlag & FIB_FLAG_STATIC)
    {
        _fib_DelStaticFib(pstFibHashNode, pstFibNode);
    }
    else
    {
        _fib_DelNormalFib(pstFibCtrl, pstFibHashNode, pstFibNode);
    }

    if ((pstFibHashNode->uiStaticNextHopCount == 0) && (pstFibHashNode->stNextHop.uiOutIfIndex == IF_INVALID_INDEX))
    {
        HASH_Del(pstFibCtrl->hHash, (HASH_NODE_S*)pstFibHashNode);
        MEM_Free(pstFibHashNode);
    }

    return;
}

static VOID _fib_AutoFindIf(IN _FIB_CTRL_S *pstFibCtrl, IN FIB_NODE_S *pstFibNode)
{
    _FIB_HASH_NODE_S *pstFound = NULL;
    _FIB_HASH_NODE_S stFibToFind;
    UINT i;
    UINT uiMask;

    if (! (pstFibNode->uiFlag & FIB_FLAG_AUTO_IF))
    {
        return;
    }

    for (i=0; i<=32; i++)
    {
        uiMask = PREFIX_2_MASK(32 - i);
        uiMask = htonl(uiMask);
        stFibToFind.stFibKey.uiDstOrStartIp = (pstFibNode->uiNextHop & uiMask);
        stFibToFind.stFibKey.uiMaskOrEndIp = uiMask;
        pstFound = (_FIB_HASH_NODE_S*) HASH_Find(pstFibCtrl->hHash, fib_HashCmp, (HASH_NODE_S*)&stFibToFind);
        if ((NULL != pstFound) && (pstFound->stNextHop.uiFlag & FIB_FLAG_DIRECT))
        {
            pstFibNode->uiOutIfIndex = pstFound->stNextHop.uiOutIfIndex;
            break;
        }
    }

    return;
}

static BS_STATUS _fib_PrefixMatch(IN _FIB_CTRL_S *pstFibCtrl, IN UINT uiDstIp , OUT FIB_NODE_S *pstFibNode)
{
    _FIB_HASH_NODE_S *pstFound = NULL;
    _FIB_HASH_NODE_S stFibToFind;
    BS_STATUS eRet = BS_NOT_FOUND;
    UINT i;
    UINT uiMask;

    for (i=0; i<=32; i++)
    {
        uiMask = PREFIX_2_MASK(32 - i);
        uiMask = htonl(uiMask);
        stFibToFind.stFibKey.uiDstOrStartIp = (uiDstIp & uiMask);
        stFibToFind.stFibKey.uiMaskOrEndIp = uiMask;
        pstFound = (_FIB_HASH_NODE_S*) HASH_Find(pstFibCtrl->hHash, fib_HashCmp, (HASH_NODE_S*)&stFibToFind);
        if (NULL != pstFound) {
            break;
        }
    }

    if (pstFound == NULL)
    {
        return BS_NOT_FOUND;
    }

    pstFibNode->stFibKey = pstFound->stFibKey;

    if (pstFound->stNextHop.uiOutIfIndex != IF_INVALID_INDEX)
    {
        pstFibNode->uiNextHop = pstFound->stNextHop.uiNextHop;
        pstFibNode->uiOutIfIndex = pstFound->stNextHop.uiOutIfIndex;
        pstFibNode->uiFlag = pstFound->stNextHop.uiFlag;
        eRet = BS_OK;
    }
    else
    {
        for (i=0; i<pstFound->uiStaticNextHopCount; i++)
        {
            pstFibNode->uiNextHop = pstFound->astStaticNextHop[i].uiNextHop;
            pstFibNode->uiOutIfIndex = pstFound->astStaticNextHop[i].uiOutIfIndex;
            pstFibNode->uiFlag = pstFound->astStaticNextHop[i].uiFlag;
            if (pstFibNode->uiOutIfIndex == IF_INVALID_INDEX)
            {
                _fib_AutoFindIf(pstFibCtrl, pstFibNode);
            }
            if (pstFibNode->uiOutIfIndex != IF_INVALID_INDEX)
            {
                pstFound->astStaticNextHop[i].uiOutIfIndex = pstFibNode->uiOutIfIndex;
                eRet = BS_OK;
                break;
            }
        }
    }

    return eRet;
}

FIB_HANDLE FIB_Create(IN UINT uiInstanceFlag )
{
    _FIB_CTRL_S *pstFibCtrl;
    HASH_S * hHash;

    pstFibCtrl = MEM_ZMalloc(sizeof(_FIB_CTRL_S));
    if (NULL == pstFibCtrl)
    {
        return NULL;
    }

    hHash = HASH_CreateInstance(NULL, _FIB_HASH_BUCKET_NUM, _fib_HashIndex);
    if (NULL == hHash)
    {
        MEM_Free(pstFibCtrl);
        return NULL;
    }

    if (uiInstanceFlag & FIB_INSTANCE_FLAG_CREATE_LOCK)
    {
        MUTEX_Init(&pstFibCtrl->stMutex);
    }
    pstFibCtrl->uiInstanceFlag = uiInstanceFlag;

    pstFibCtrl->hHash = hHash;

    return pstFibCtrl;
}

VOID FIB_Destory(IN FIB_HANDLE hFibHandle)
{
    _FIB_CTRL_S *pstFibCtrl = hFibHandle;

    if (NULL != pstFibCtrl)
    {
        HASH_DelAll(pstFibCtrl->hHash, fib_FreeHashNode, NULL);
        HASH_DestoryInstance(pstFibCtrl->hHash);
        if (pstFibCtrl->uiInstanceFlag & FIB_INSTANCE_FLAG_CREATE_LOCK)
        {
            MUTEX_Final(&pstFibCtrl->stMutex);
        }
        MEM_Free(pstFibCtrl);
    }
}

BS_STATUS FIB_Add(IN FIB_HANDLE hFibHandle, IN FIB_NODE_S *pstFibNode)
{
    _FIB_CTRL_S *pstFibCtrl = hFibHandle;
    BS_STATUS eRet = BS_OK;

    if (NULL == pstFibCtrl)
    {
        BS_DBGASSERT(0);
        RETURN(BS_BAD_PARA);
    }

    pstFibNode->stFibKey.uiDstOrStartIp &= pstFibNode->stFibKey.uiMaskOrEndIp;

    fib_Lock(pstFibCtrl);
    _fib_AutoFindIf(pstFibCtrl, pstFibNode);
    eRet = _fib_Add(pstFibCtrl, pstFibNode);
    fib_UnLock(pstFibCtrl);

    return eRet;
}


VOID FIB_Del(IN FIB_HANDLE hFibHandle, IN FIB_NODE_S *pstFibNode)
{
    _FIB_CTRL_S *pstFibCtrl = hFibHandle;

    if (NULL == pstFibCtrl)
    {
        BS_DBGASSERT(0);
        return;
    }

    pstFibNode->stFibKey.uiDstOrStartIp &= pstFibNode->stFibKey.uiMaskOrEndIp;

    fib_Lock(pstFibCtrl);
    _fib_Del(pstFibCtrl, pstFibNode);
    fib_UnLock(pstFibCtrl);

    return;
}

VOID FIB_DelAll(IN FIB_HANDLE hFibHandle)
{
    _FIB_CTRL_S *pstFibCtrl = hFibHandle;

    if (NULL == pstFibCtrl)
    {
        BS_DBGASSERT(0);
        return;
    }

    fib_Lock(pstFibCtrl);
    HASH_DelAll(pstFibCtrl->hHash, fib_FreeHashNode, NULL);
    fib_UnLock(pstFibCtrl);
}

BS_STATUS FIB_PrefixMatch(IN FIB_HANDLE hFibHandle, IN UINT uiDstIp , OUT FIB_NODE_S *pstFibNode)
{
    _FIB_CTRL_S *pstFibCtrl = hFibHandle;
    BS_STATUS eRet;

    if (NULL == pstFibCtrl)
    {
        BS_DBGASSERT(0);
        RETURN(BS_BAD_PARA);
    }

    fib_Lock(pstFibCtrl);
    eRet = _fib_PrefixMatch(pstFibCtrl, uiDstIp, pstFibNode);
    fib_UnLock(pstFibCtrl);

    return eRet;
}

VOID FIB_Walk(IN FIB_HANDLE hFibHandle, IN PF_FIB_WALK_FUNC pfWalkFunc, IN HANDLE hUserHandle)
{
    _FIB_CTRL_S *pstFibCtrl = hFibHandle;
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfWalkFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    fib_Lock(pstFibCtrl);
    HASH_Walk(pstFibCtrl->hHash, (PF_HASH_WALK_FUNC)fib_WalkEach, &stUserHandle);
    fib_UnLock(pstFibCtrl);
}

static INT _fib_GetNextCmp(IN FIB_NODE_S *pstNode1, IN FIB_NODE_S *pstNode2)
{
    INT iCmpRet;

    if ((pstNode1 == NULL) && (pstNode2 == NULL))
    {
        return 0;
    }

    if (pstNode1 == NULL)
    {
        return -1;
    }

    if (pstNode2 == NULL)
    {
        return 1;
    }

    iCmpRet = NUM_Cmp(ntohl(pstNode1->stFibKey.uiDstOrStartIp), ntohl(pstNode2->stFibKey.uiDstOrStartIp));
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    iCmpRet = NUM_Cmp(pstNode1->stFibKey.uiMaskOrEndIp, pstNode2->stFibKey.uiMaskOrEndIp);
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    iCmpRet = NUM_Cmp(pstNode1->uiNextHop, pstNode2->uiNextHop);
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    iCmpRet = NUM_Cmp(pstNode1->uiFlag, pstNode2->uiFlag);
    if (0 != iCmpRet)
    {
        return iCmpRet;
    }

    return iCmpRet;
}

static int _fib_GetNextEach(IN FIB_NODE_S *pstFibNode, IN HANDLE hUserHandle)
{
    USER_HANDLE_S *pstUserHandle = hUserHandle;
    FIB_NODE_S *pstFibCurrent = pstUserHandle->ahUserHandle[0];
    FIB_NODE_S *pstFibNext = pstUserHandle->ahUserHandle[1];
    BOOL_T *pbFound = pstUserHandle->ahUserHandle[2];

    if (pstFibCurrent != NULL)
    {
        if (_fib_GetNextCmp(pstFibNode, pstFibCurrent) >= 0)
        {
            return 0;
        }
    }

    if (_fib_GetNextCmp(pstFibNode, pstFibNext) <= 0)
    {
        return 0;
    }

    *pstFibNext = *pstFibNode;
    *pbFound = TRUE;

    return 0;
}

BS_STATUS FIB_GetNext
(
    IN FIB_HANDLE hFibHandle,
    IN FIB_NODE_S *pstFibCurrent,
    OUT FIB_NODE_S *pstFibNext
)
{
    USER_HANDLE_S stUserHandle;
    FIB_NODE_S stFibNext;
    BOOL_T bFound = FALSE;

    memset(&stFibNext, 0, sizeof(stFibNext));

    stUserHandle.ahUserHandle[0] = pstFibCurrent;
    stUserHandle.ahUserHandle[1] = &stFibNext;
    stUserHandle.ahUserHandle[2] = &bFound;
  
    FIB_Walk(hFibHandle, _fib_GetNextEach, &stUserHandle);

    if (!bFound)
    {
        return BS_NOT_FOUND;
    }

    *pstFibNext = stFibNext;

    return BS_OK;
}

VOID FIB_ShowFlagHelp()
{
    EXEC_OutString(
        " Flag:\r\n"
        " U:Deliver-Up   D:Direct   S:Static   A:Auto-Interface \r\n\r\n");
}

CHAR * FIB_GetFlagString(IN UINT uiFlag, OUT CHAR *pcFlagString)
{
    UINT uiIndex = 0;

    if (uiFlag & FIB_FLAG_DELIVER_UP)
    {
        pcFlagString[uiIndex] = 'U';
        uiIndex ++;
    }

    if (uiFlag & FIB_FLAG_DIRECT)
    {
        pcFlagString[uiIndex] = 'D';
        uiIndex ++;
    }

    if (uiFlag & FIB_FLAG_STATIC)
    {
        pcFlagString[uiIndex] = 'S';
        uiIndex ++;
    }

    if (uiFlag & FIB_FLAG_AUTO_IF)
    {
        pcFlagString[uiIndex] = 'A';
        uiIndex ++;
    }

    pcFlagString[uiIndex] = '\0';

    return pcFlagString;
}

BS_STATUS FIB_Show (IN FIB_HANDLE hFibHandle)
{
    _FIB_CTRL_S *pstFibCtrl = hFibHandle;
    FIB_NODE_S stFibNode;
    FIB_NODE_S *pstCurrent = NULL;

    if (NULL == pstFibCtrl)
    {
        BS_DBGASSERT(0);
        RETURN(BS_BAD_PARA);
    }

    EXEC_OutString(" IP                 Nexthop         Flag   Ifnet\r\n"
                              "-------------------------------------------------------------------\r\n");

    while (BS_OK == FIB_GetNext(pstFibCtrl, pstCurrent, &stFibNode))
    {
        fib_ShowEach(&stFibNode);
        pstCurrent = &stFibNode;
    }

    EXEC_OutString("\r\n");

    FIB_ShowFlagHelp();
    
    return BS_OK;
}


