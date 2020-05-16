/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
        
#include "utl/ip_utl.h"
#include "utl/tcp_utl.h"
#include "utl/udp_utl.h"
#include "utl/hash_utl.h"
#include "utl/mutex_utl.h"
#include "utl/nat_utl.h"
#include "utl/vclock_utl.h"
#include "utl/in_checksum.h"
    
#include "nat_inner.h"

#define _NAT_UDP_HASH_BUCKET_NUM 1024

#define _NAT_UDP_TIME_OUT_TIME_SYN 5000        /* 处于SYN状态的最大时间 5s. 防攻击用. 第一个UDP报文被认为是SYN,只有收到第二个报文后,才认为不是SYN了 */
#define _NAT_UDP_TIME_OUT_TIME_DFT 30000       /* 处于SYN状态的最大时间 30s */


typedef struct
{
    HASH_NODE_S stPrivateListNode;
    HASH_NODE_S stPubListNode;
    VCLOCK_HANDLE hVclockTimer;
    NAT_NODE_S stNatNode;
}_NAT_UDP_MAP_NODE_S;

static VOID  nat_udp_FreeAllMapNode
(
    IN HASH_HANDLE hHashId,
    IN VOID *pstNode,
    IN VOID * pUserHandle
)
{
    _NAT_UDP_MAP_NODE_S *pstNatNode;
    _NAT_UDP_CTRL_S *pstCtrl = pUserHandle;

    pstNatNode = BS_ENTRY(pstNode, stPrivateListNode, _NAT_UDP_MAP_NODE_S);

    VCLOCK_DestroyTimer(pstCtrl->hVClock, pstNatNode->hVclockTimer);

    MEM_Free(pstNatNode);
}

static inline UINT nat_udp_HashIndex(IN UINT uiIp, IN USHORT usPort)
{
    return ntohl(uiIp) + ntohs(usPort);
}

static UINT nat_udp_PrivateHashIndex(IN VOID *pstHashNode)
{
    _NAT_UDP_MAP_NODE_S *pstNatNode;

    pstNatNode = BS_ENTRY(pstHashNode, stPrivateListNode, _NAT_UDP_MAP_NODE_S);

    return nat_udp_HashIndex(pstNatNode->stNatNode.uiPrivateIp, pstNatNode->stNatNode.usPrivatePort);
}

static UINT nat_udp_PubHashIndex(IN VOID *pstHashNode)
{
    _NAT_UDP_MAP_NODE_S *pstNatNode;

    pstNatNode = BS_ENTRY(pstHashNode, stPubListNode, _NAT_UDP_MAP_NODE_S);

    return nat_udp_HashIndex(pstNatNode->stNatNode.uiPubIp, pstNatNode->stNatNode.usPubPort);
}

static inline VOID nat_udp_Lock(IN _NAT_UDP_CTRL_S *pstCtrl)
{
    if (pstCtrl->bCreateMutex)
    {
        MUTEX_P(&pstCtrl->stMutex);
    }
}

static inline VOID nat_udp_UnLock(IN _NAT_UDP_CTRL_S *pstCtrl)
{
    if (pstCtrl->bCreateMutex)
    {
        MUTEX_V(&pstCtrl->stMutex);
    }
}

static INT nat_udp_Cmp
(
    IN UINT uiIp1,
    IN USHORT usPort1,
    IN UINT uiIp2,
    IN USHORT usPort2
)
{
    INT iCmpResult;

    iCmpResult = uiIp1 - uiIp2;
    if (0 != iCmpResult)
    {
        return iCmpResult;
    }

    iCmpResult = usPort1 - usPort2;
    if (0 != iCmpResult)
    {
        return iCmpResult;
    }

    return 0;
}

static INT  nat_udp_CmpForPrivate(IN VOID * pstHashNode1, IN VOID * pstNodeToFind)
{
    _NAT_UDP_MAP_NODE_S *pstNatNode, *pstNatNodeToFind;
    INT iCmpResult;

    pstNatNode = BS_ENTRY(pstHashNode1, stPrivateListNode, _NAT_UDP_MAP_NODE_S);
    pstNatNodeToFind = BS_ENTRY(pstNodeToFind, stPrivateListNode, _NAT_UDP_MAP_NODE_S);

    iCmpResult = nat_udp_Cmp(pstNatNode->stNatNode.uiPrivateIp,
                         pstNatNode->stNatNode.usPrivatePort,
                         pstNatNodeToFind->stNatNode.uiPrivateIp,
                         pstNatNodeToFind->stNatNode.usPrivatePort);
    if (iCmpResult != 0)
    {
        return iCmpResult;
    }

    return pstNatNode->stNatNode.uiDomainId - pstNatNodeToFind->stNatNode.uiDomainId;
}


static _NAT_UDP_MAP_NODE_S *nat_udp_PrivateGetMapNode
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN UINT uiIp/* 网络序 */,
    IN USHORT usPort/* 网络序 */,
    IN UINT uiDomainId
)
{
    _NAT_UDP_MAP_NODE_S *pstNode = NULL;
    _NAT_UDP_MAP_NODE_S stToFind;
    HASH_NODE_S *pstHashNode;

    stToFind.stNatNode.uiPrivateIp = uiIp;
    stToFind.stNatNode.usPrivatePort = usPort;
    stToFind.stNatNode.uiDomainId = uiDomainId;

    pstHashNode = HASH_Find(pstCtrl->hPrivateHashHandle, nat_udp_CmpForPrivate, &stToFind.stPrivateListNode);
    if (NULL != pstHashNode)
    {
        pstNode = BS_ENTRY(pstHashNode, stPrivateListNode, _NAT_UDP_MAP_NODE_S);
    }

    return pstNode;
}

/* 返回网络序Port */
static USHORT nat_udp_GetPort(IN _NAT_UDP_CTRL_S *pstCtrl)
{
    UINT uiIndexFrom1;
    USHORT usPort;
    
    uiIndexFrom1 = BITMAP1_GetAUnsettedBitIndexCycle(&pstCtrl->stUdpPortBitMap);

    if (uiIndexFrom1 == 0)
    {
        return 0;
    }

    BITMAP_SET(&pstCtrl->stUdpPortBitMap, uiIndexFrom1);

    usPort = pstCtrl->usMinPort + uiIndexFrom1 - 1;

    return htons(usPort);
}

static VOID nat_udp_FreePort(IN _NAT_UDP_CTRL_S *pstCtrl, IN USHORT usPort/* 网络序 */)
{
    UINT uiIndexFrom1;

    uiIndexFrom1 = ntohs(usPort) - pstCtrl->usMinPort + 1;

    BITMAP1_CLR(&pstCtrl->stUdpPortBitMap, uiIndexFrom1);
}

static inline UINT nat_udp_GetSynTimeOutTick
(
    IN _NAT_UDP_CTRL_S *pstCtrl
)
{
    return pstCtrl->uiSynUdpTimeOutTick;
}

static VOID nat_udp_TimeOut(IN HANDLE hTimer, IN USER_HANDLE_S *pstUserHandle)
{
    _NAT_UDP_MAP_NODE_S *pstNode;
    _NAT_UDP_CTRL_S *pstCtrl;

    pstCtrl = pstUserHandle->ahUserHandle[0];
    pstNode = pstUserHandle->ahUserHandle[1];

    HASH_Del(pstCtrl->hPrivateHashHandle, &pstNode->stPrivateListNode);
    HASH_Del(pstCtrl->hPubHashHandle, &pstNode->stPubListNode);
    VCLOCK_DestroyTimer(pstCtrl->hVClock, pstNode->hVclockTimer);
    nat_udp_FreePort(pstCtrl, pstNode->stNatNode.usPubPort);
    MEM_Free(pstNode);
}


static _NAT_UDP_MAP_NODE_S *nat_udp_AddMapNode
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN UINT uiPrivateIp/* 网络序 */,
    IN USHORT usPrivatePort/* 网络序 */,
    IN UINT uiDomainId
)
{
    _NAT_UDP_MAP_NODE_S *pstNode;
    USER_HANDLE_S stUserHandle;
    USHORT usPubPort;
    UINT uiTimeOutTick = 0;

    if (pstCtrl->auiPubIp[0] == 0)
    {
        return NULL;
    }

    usPubPort = nat_udp_GetPort(pstCtrl);
    if (0 == usPubPort)
    {
        return NULL;
    }

    pstNode = MEM_ZMalloc(sizeof(_NAT_UDP_MAP_NODE_S));
    if (NULL == pstNode)
    {
        nat_udp_FreePort(pstCtrl, usPubPort);
        return NULL;
    }

    uiTimeOutTick = nat_udp_GetSynTimeOutTick(pstCtrl);

    stUserHandle.ahUserHandle[0] = pstCtrl;
    stUserHandle.ahUserHandle[1] = pstNode;
    pstNode->hVclockTimer = VCLOCK_CreateTimer(pstCtrl->hVClock, uiTimeOutTick, uiTimeOutTick, TIMER_FLAG_CYCLE, nat_udp_TimeOut, &stUserHandle);
    if (NULL == pstNode->hVclockTimer)
    {
        nat_udp_FreePort(pstCtrl, usPubPort);
        MEM_Free(pstNode);
        return NULL;
    }

    pstNode->stNatNode.uiPrivateIp = uiPrivateIp;
    pstNode->stNatNode.usPrivatePort = usPrivatePort;
    pstNode->stNatNode.uiPubIp = pstCtrl->auiPubIp[0];
    pstNode->stNatNode.usPubPort = usPubPort;
    pstNode->stNatNode.uiDomainId = uiDomainId;
    pstNode->stNatNode.ucStatus = NAT_UDP_STATUS_SYN_SEND;
    pstNode->stNatNode.ucType = IP_PROTO_UDP;
    
    HASH_Add(pstCtrl->hPrivateHashHandle, &pstNode->stPrivateListNode);
    HASH_Add(pstCtrl->hPubHashHandle, &pstNode->stPubListNode);

    return pstNode;
}

static inline VOID nat_udp_StepPrivateStatus
(
    IN _NAT_UDP_MAP_NODE_S *pstNatMap,
    IN UDP_HEAD_S *pstUdpHead
)
{
    if (pstNatMap->stNatNode.ucStatus == NAT_UDP_STATUS_SYN_RECEIVED)
    {
        pstNatMap->stNatNode.ucStatus = NAT_UDP_STATUS_ESTABLISHED;
    }
}

static inline _NAT_UDP_MAP_NODE_S * nat_udp_ProcessPrivatePkt
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN _NAT_UDP_MAP_NODE_S *pstNatMap,
    IN UDP_HEAD_S *pstUdpHead
)
{
    nat_udp_StepPrivateStatus(pstNatMap, pstUdpHead);

    if (pstNatMap->stNatNode.ucStatus >= NAT_UDP_STATUS_ESTABLISHED)
    {
        VCLOCK_RestartWithTick(pstCtrl->hVClock, pstNatMap->hVclockTimer, pstCtrl->uiUdpTimeOutTick, 0);
    }
    else
    {
        VCLOCK_Refresh(pstCtrl->hVClock, pstNatMap->hVclockTimer);
    }

    return pstNatMap;
}

static BS_STATUS nat_udp_PrivatePktIn
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN IP_HEAD_S  *pstIpHead,
    IN UDP_HEAD_S *pstUdpHead,
    IN UINT uiDomainId
)
{
    UINT uiIp;
    USHORT usPort;
    _NAT_UDP_MAP_NODE_S *pstNatMap;
    BS_STATUS eRet;

    uiIp = pstIpHead->unSrcIp.uiIp;
    usPort = pstUdpHead->usSrcPort;

    nat_udp_Lock(pstCtrl);
    pstNatMap = nat_udp_PrivateGetMapNode(pstCtrl, uiIp, usPort, uiDomainId);

    if (pstNatMap == NULL)
    {
        pstNatMap = nat_udp_AddMapNode(pstCtrl, uiIp, usPort, uiDomainId);
    }

    if (NULL == pstNatMap)
    {
        eRet = BS_NO_SUCH;
    }
    else
    {
        eRet = BS_OK;

        pstIpHead->unSrcIp.uiIp = pstNatMap->stNatNode.uiPubIp;
        pstUdpHead->usSrcPort = pstNatMap->stNatNode.usPubPort;
        
        pstNatMap = nat_udp_ProcessPrivatePkt(pstCtrl, pstNatMap, pstUdpHead);
    }

    nat_udp_UnLock(pstCtrl);

    return eRet;
}

static INT  nat_udp_CmpForPub(IN VOID * pstHashNode1, IN VOID * pstNodeToFind)
{
    _NAT_UDP_MAP_NODE_S *pstNatNode, *pstNatNodeToFind;

    pstNatNode = BS_ENTRY(pstHashNode1, stPubListNode, _NAT_UDP_MAP_NODE_S);
    pstNatNodeToFind = BS_ENTRY(pstNodeToFind, stPubListNode, _NAT_UDP_MAP_NODE_S);

    return nat_udp_Cmp(pstNatNode->stNatNode.uiPubIp,
                   pstNatNode->stNatNode.usPubPort,
                   pstNatNodeToFind->stNatNode.uiPubIp,
                   pstNatNodeToFind->stNatNode.usPubPort);
}

static _NAT_UDP_MAP_NODE_S * nat_udp_PubGetMapNode
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN UINT uiIp/* 网络序 */,
    IN USHORT usPort/* 网络序 */
)
{
    _NAT_UDP_MAP_NODE_S *pstNode;
    _NAT_UDP_MAP_NODE_S stToFind;
    HASH_NODE_S *pstHashNode;

    stToFind.stNatNode.uiPubIp = uiIp;
    stToFind.stNatNode.usPubPort = usPort;

    pstHashNode = HASH_Find(pstCtrl->hPubHashHandle, nat_udp_CmpForPub, &stToFind.stPubListNode);
    if (NULL == pstHashNode)
    {
        return NULL;
    }

    pstNode = BS_ENTRY(pstHashNode, stPubListNode, _NAT_UDP_MAP_NODE_S);

    return pstNode;
}

static inline VOID nat_udp_StepPubStatus
(
    IN _NAT_UDP_MAP_NODE_S *pstNatMap,
    IN UDP_HEAD_S *pstUdpHead
)
{
    if (pstNatMap->stNatNode.ucStatus == NAT_UDP_STATUS_SYN_SEND)
    {
        pstNatMap->stNatNode.ucStatus = NAT_UDP_STATUS_SYN_RECEIVED;
    }
}

static inline _NAT_UDP_MAP_NODE_S * nat_udp_ProcessPubPkt
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN _NAT_UDP_MAP_NODE_S *pstNatMap,
    IN UDP_HEAD_S *pstUdpHead
)
{
    nat_udp_StepPubStatus(pstNatMap, pstUdpHead);

    if (pstNatMap->stNatNode.ucStatus >= NAT_UDP_STATUS_ESTABLISHED)
    {
        VCLOCK_RestartWithTick(pstCtrl->hVClock, pstNatMap->hVclockTimer, pstCtrl->uiUdpTimeOutTick, 0);
    }
    else
    {
        VCLOCK_Refresh(pstCtrl->hVClock, pstNatMap->hVclockTimer);
    }

    return pstNatMap;
}

static BS_STATUS nat_udp_PubPktIn
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN IP_HEAD_S  *pstIpHead,
    IN UDP_HEAD_S *pstUdpHead,
    IN UINT *puiDomainId
)
{
    UINT uiIp;
    USHORT usPort;
    _NAT_UDP_MAP_NODE_S *pstNatMap;
    USHORT usDstPort;
    BS_STATUS eRet;

    usDstPort = ntohs(pstUdpHead->usDstPort);
    if ((usDstPort < pstCtrl->usMinPort) || (usDstPort > pstCtrl->usMaxPort))
    {
        return BS_NO_PERMIT;
    }

    uiIp = pstIpHead->unDstIp.uiIp;
    usPort = pstUdpHead->usDstPort;

    nat_udp_Lock(pstCtrl);

    pstNatMap = nat_udp_PubGetMapNode(pstCtrl, uiIp, usPort);

    if (NULL == pstNatMap)
    {
        eRet = BS_NO_SUCH;
    }
    else
    {
        eRet = BS_OK;

        pstIpHead->unDstIp.uiIp = pstNatMap->stNatNode.uiPrivateIp;
        pstUdpHead->usDstPort = pstNatMap->stNatNode.usPrivatePort;
        *puiDomainId = pstNatMap->stNatNode.uiDomainId;

        pstNatMap = nat_udp_ProcessPubPkt(pstCtrl, pstNatMap, pstUdpHead);
    }
    
    nat_udp_UnLock(pstCtrl);

    return eRet;
}

static BS_WALK_RET_E nat_udp_WalkEach(IN HASH_HANDLE hHashId, IN VOID *pstNode, IN VOID * pUserHandle)
{
    _NAT_UDP_MAP_NODE_S *pstNatNode = (_NAT_UDP_MAP_NODE_S*)pstNode;
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_NAT_WALK_CALL_BACK_FUNC pfFunc;

    pfFunc = pstUserHandle->ahUserHandle[0];

    return pfFunc (&pstNatNode->stNatNode, pstUserHandle->ahUserHandle[1]);
}

static VOID nat_udp_Walk
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN PF_NAT_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    HASH_Walk(pstCtrl->hPrivateHashHandle, nat_udp_WalkEach, &stUserHandle);
}

BS_STATUS _NAT_UDP_Init
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN USHORT usMinPort,   /* 主机序 ,对外可转换的端口号最小值 */
    IN USHORT usMaxPort,   /* 主机序 ,对外可转换的端口号最大值 */
    IN UINT   uiMsInTick,  /* 多少ms为一个Tick */
    IN BOOL_T bCreateMutex
)
{
    pstCtrl->usMinPort = usMinPort;
    pstCtrl->usMaxPort = usMaxPort;

    if (BS_OK != BITMAP_Create(&pstCtrl->stUdpPortBitMap, usMaxPort - usMinPort + 1))
    {
        return BS_ERR;
    }

    pstCtrl->hPrivateHashHandle = HASH_CreateInstance(_NAT_UDP_HASH_BUCKET_NUM, nat_udp_PrivateHashIndex);
    if (NULL == pstCtrl->hPrivateHashHandle)
    {
        _NAT_UDP_Fini(pstCtrl);
        return BS_ERR;
    }

    pstCtrl->hPubHashHandle = HASH_CreateInstance(_NAT_UDP_HASH_BUCKET_NUM, nat_udp_PubHashIndex);
    if (NULL == pstCtrl->hPubHashHandle)
    {
        _NAT_UDP_Fini(pstCtrl);
        return BS_ERR;
    }

    pstCtrl->hVClock = VCLOCK_CreateInstance(FALSE);
    if (NULL == pstCtrl->hVClock)
    {
        _NAT_UDP_Fini(pstCtrl);
        return BS_ERR;
    }

    if (bCreateMutex)
    {
        MUTEX_Init(&pstCtrl->stMutex);
    }

    pstCtrl->bCreateMutex = TRUE;
    pstCtrl->uiUdpTimeOutTick = _NAT_GET_TICK_BY_TIME(_NAT_UDP_TIME_OUT_TIME_DFT, uiMsInTick);
    pstCtrl->uiSynUdpTimeOutTick = _NAT_GET_TICK_BY_TIME(_NAT_UDP_TIME_OUT_TIME_SYN, uiMsInTick);

    return BS_OK;
}

VOID _NAT_UDP_Fini(IN _NAT_UDP_CTRL_S *pstCtrl)
{
    BITMAP_Destory(&pstCtrl->stUdpPortBitMap);

    if (NULL != pstCtrl->hPrivateHashHandle)
    {
        HASH_DelAll(pstCtrl->hPrivateHashHandle, nat_udp_FreeAllMapNode, pstCtrl);
        HASH_DestoryInstance(pstCtrl->hPrivateHashHandle);
        pstCtrl->hPrivateHashHandle = NULL;
    }

    if (NULL != pstCtrl->hPubHashHandle)
    {
        HASH_DestoryInstance(pstCtrl->hPubHashHandle);
        pstCtrl->hPubHashHandle = NULL;
    }

    if (NULL != pstCtrl->hVClock)
    {
        VCLOCK_DeleteInstance(pstCtrl->hVClock);
        pstCtrl->hVClock = NULL;
    }

    if (TRUE == pstCtrl->bCreateMutex)
    {
        MUTEX_Final(&pstCtrl->stMutex);
    }

    return;
}

BS_STATUS _NAT_UDP_SetPubIp
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN UINT auiPubIp[NAT_MAX_PUB_IP_NUM] /* 网络序，提供的对外公网IP */
)
{
    UINT i;

    nat_udp_Lock(pstCtrl);
    for (i=0; i<NAT_MAX_PUB_IP_NUM; i++)
    {
        pstCtrl->auiPubIp[i] = auiPubIp[i];
    }

    HASH_DelAll(pstCtrl->hPrivateHashHandle, nat_udp_FreeAllMapNode, pstCtrl);
    HASH_DelAll(pstCtrl->hPubHashHandle, NULL, NULL);
    nat_udp_UnLock(pstCtrl);

    return BS_OK;
}

BS_STATUS _NAT_UDP_PktIn
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN IP_HEAD_S  *pstIpHead,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    IN BOOL_T bFromPub,
    INOUT UINT *puiDomainId
)
{
	UDP_HEAD_S *pstUdpHead = NULL;
    BS_STATUS eRet;
    USHORT usRawSum;

    pstUdpHead = UDP_GetUDPHeader(pucData, uiDataLen, NET_PKT_TYPE_IP);
    if (NULL == pstUdpHead)
    {
        return BS_ERR;
    }

    /* 计算去掉IP/Port后的原始IP */
    usRawSum = IN_CHKSUM_UnWrap(pstUdpHead->usCrc);
    usRawSum = IN_CHKSUM_DelRaw(usRawSum, (UCHAR*)&pstIpHead->unSrcIp, 8);
    usRawSum = IN_CHKSUM_DelRaw(usRawSum, (UCHAR*)&pstUdpHead->usSrcPort, 4);

    if (! bFromPub)
    {
        eRet = nat_udp_PrivatePktIn(pstCtrl, pstIpHead, pstUdpHead, *puiDomainId);
    }
    else
    {
        eRet = nat_udp_PubPktIn(pstCtrl, pstIpHead, pstUdpHead, puiDomainId);
    }

    if (eRet != BS_OK)
    {
        return eRet;
    }

    usRawSum = IN_CHKSUM_AddRaw(usRawSum, (UCHAR*)&pstIpHead->unSrcIp, 8);
    usRawSum = IN_CHKSUM_AddRaw(usRawSum, (UCHAR*)&pstUdpHead->usSrcPort, 4);
    
    pstUdpHead->usCrc = IN_CHKSUM_Wrap(usRawSum);

    return BS_OK;
}

VOID _NAT_UDP_TimerStep(IN _NAT_UDP_CTRL_S *pstCtrl)
{
    nat_udp_Lock(pstCtrl);
    VCLOCK_Step(pstCtrl->hVClock);
    nat_udp_UnLock(pstCtrl);
}

VOID _NAT_UDP_Walk
(
    IN _NAT_UDP_CTRL_S *pstCtrl,
    IN PF_NAT_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    nat_udp_Lock(pstCtrl);
    nat_udp_Walk(pstCtrl, pfFunc, hUserHandle);
    nat_udp_UnLock(pstCtrl);
}


