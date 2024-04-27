/******************************************************************************
* Copyright (C) LiXingang
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

#define _NAT_TCP_HASH_BUCKET_NUM 1024

#define _NAT_TCP_TIME_OUT_TIME_SYN 10000       
#define _NAT_TCP_TIME_OUT_TIME_DFT 300000      


typedef struct
{
    HASH_NODE_S stPrivateListNode;
    HASH_NODE_S stPubListNode;
    VCLOCK_HANDLE hVclockTimer;
    NAT_NODE_S stNatNode;
}_NAT_TCP_MAP_NODE_S;


static VOID nat_tcp_FreeAllMapNode(void * hHashId, VOID *pstNode, VOID * pUserHandle)
{
    _NAT_TCP_MAP_NODE_S *pstNatNode;
    _NAT_TCP_CTRL_S *pstCtrl = pUserHandle;

    pstNatNode = BS_ENTRY(pstNode, stPrivateListNode, _NAT_TCP_MAP_NODE_S);

    VCLOCK_DestroyTimer(pstCtrl->hVClock, pstNatNode->hVclockTimer);

    MEM_Free(pstNatNode);
}

static inline UINT nat_tcp_HashIndex(IN UINT uiIp, IN USHORT usPort)
{
    return ntohl(uiIp) + ntohs(usPort);
}

static UINT nat_tcp_PrivateHashIndex(IN VOID *pstHashNode)
{
    _NAT_TCP_MAP_NODE_S *pstNatNode;

    pstNatNode = BS_ENTRY(pstHashNode, stPrivateListNode, _NAT_TCP_MAP_NODE_S);

    return nat_tcp_HashIndex(pstNatNode->stNatNode.uiPrivateIp, pstNatNode->stNatNode.usPrivatePort);
}

static UINT nat_tcp_PubHashIndex(IN VOID *pstHashNode)
{
    _NAT_TCP_MAP_NODE_S *pstNatNode;

    pstNatNode = BS_ENTRY(pstHashNode, stPubListNode, _NAT_TCP_MAP_NODE_S);

    return nat_tcp_HashIndex(pstNatNode->stNatNode.uiPubIp, pstNatNode->stNatNode.usPubPort);
}

static inline VOID nat_tcp_Lock(IN _NAT_TCP_CTRL_S *pstCtrl)
{
    if (pstCtrl->bCreateMutex)
    {
        MUTEX_P(&pstCtrl->stMutex);
    }
}

static inline VOID nat_tcp_UnLock(IN _NAT_TCP_CTRL_S *pstCtrl)
{
    if (pstCtrl->bCreateMutex)
    {
        MUTEX_V(&pstCtrl->stMutex);
    }
}

static INT nat_tcp_Cmp
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


static INT nat_tcp_CmpForPrivate(IN VOID * pstHashNode1, IN VOID * pstNodeToFind)
{
    _NAT_TCP_MAP_NODE_S *pstNatNode, *pstNatNodeToFind;
    INT iCmpResult;

    pstNatNode = BS_ENTRY(pstHashNode1, stPrivateListNode, _NAT_TCP_MAP_NODE_S);
    pstNatNodeToFind = BS_ENTRY(pstNodeToFind, stPrivateListNode, _NAT_TCP_MAP_NODE_S);

    iCmpResult = nat_tcp_Cmp(pstNatNode->stNatNode.uiPrivateIp,
                         pstNatNode->stNatNode.usPrivatePort,
                         pstNatNodeToFind->stNatNode.uiPrivateIp,
                         pstNatNodeToFind->stNatNode.usPrivatePort);
    if (iCmpResult != 0)
    {
        return iCmpResult;
    }

    return pstNatNode->stNatNode.uiDomainId - pstNatNodeToFind->stNatNode.uiDomainId;
}


static _NAT_TCP_MAP_NODE_S *nat_tcp_PrivateGetMapNode
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN UINT uiIp,
    IN USHORT usPort,
    IN UINT uiDomainId
)
{
    _NAT_TCP_MAP_NODE_S *pstNode = NULL;
    _NAT_TCP_MAP_NODE_S stToFind;
    HASH_NODE_S *pstHashNode;

    stToFind.stNatNode.uiPrivateIp = uiIp;
    stToFind.stNatNode.usPrivatePort = usPort;
    stToFind.stNatNode.uiDomainId = uiDomainId;

    pstHashNode = HASH_Find(pstCtrl->hPrivateHashHandle, nat_tcp_CmpForPrivate, &stToFind.stPrivateListNode);
    if (NULL != pstHashNode)
    {
        pstNode = BS_ENTRY(pstHashNode, stPrivateListNode, _NAT_TCP_MAP_NODE_S);
    }

    return pstNode;
}

static inline BOOL_T nat_tcp_IsPermitAddTcpMap
(
    IN TCP_HEAD_S *pstTcpHead
)
{
    if (pstTcpHead->ucFlag & TCP_FLAG_SYN)
    {
        return TRUE;
    }

    return FALSE;
}


static USHORT nat_tcp_GetPort(IN _NAT_TCP_CTRL_S *pstCtrl)
{
    UINT uiIndexFrom1;
    USHORT usPort;
    uiIndexFrom1 = BITMAP1_GetFreeCycle(&pstCtrl->stTcpPortBitMap);

    if (uiIndexFrom1 == 0)
    {
        return 0;
    }

    BITMAP_SET(&pstCtrl->stTcpPortBitMap, uiIndexFrom1);

    usPort = pstCtrl->usMinPort + uiIndexFrom1 - 1;

    return htons(usPort);
}

static VOID nat_tcp_FreePort(IN _NAT_TCP_CTRL_S *pstCtrl, IN USHORT usPort)
{
    UINT uiIndexFrom1;

    uiIndexFrom1 = ntohs(usPort) - pstCtrl->usMinPort + 1;

    BITMAP1_CLR(&pstCtrl->stTcpPortBitMap, uiIndexFrom1);
}

static inline UINT nat_tcp_GetSynTimeOutTick
(
    IN _NAT_TCP_CTRL_S *pstCtrl
)
{
    return pstCtrl->uiSynTcpTimeOutTick;
}

static VOID nat_tcp_TimeOut(IN HANDLE hTimer, IN USER_HANDLE_S *pstUserHandle)
{
    _NAT_TCP_MAP_NODE_S *pstNode;
    _NAT_TCP_CTRL_S *pstCtrl;

    pstCtrl = pstUserHandle->ahUserHandle[0];
    pstNode = pstUserHandle->ahUserHandle[1];

    HASH_Del(pstCtrl->hPrivateHashHandle, &pstNode->stPrivateListNode);
    HASH_Del(pstCtrl->hPubHashHandle, &pstNode->stPubListNode);
    VCLOCK_DestroyTimer(pstCtrl->hVClock, pstNode->hVclockTimer);
    nat_tcp_FreePort(pstCtrl, pstNode->stNatNode.usPubPort);
    MEM_Free(pstNode);
}

static _NAT_TCP_MAP_NODE_S * nat_tcp_AddMapNode
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN UINT uiPrivateIp,
    IN USHORT usPrivatePort,
    IN UINT uiDomainId
)
{
    _NAT_TCP_MAP_NODE_S *pstNode;
    USER_HANDLE_S stUserHandle;
    USHORT usPubPort;
    UINT uiTimeOutTick = 0;

    if (pstCtrl->auiPubIp[0] == 0)
    {
        return NULL;
    }

    usPubPort = nat_tcp_GetPort(pstCtrl);
    if (0 == usPubPort)
    {
        return NULL;
    }

    pstNode = MEM_ZMalloc(sizeof(_NAT_TCP_MAP_NODE_S));
    if (NULL == pstNode)
    {
        nat_tcp_FreePort(pstCtrl, usPubPort);
        return NULL;
    }

    uiTimeOutTick = nat_tcp_GetSynTimeOutTick(pstCtrl);

    stUserHandle.ahUserHandle[0] = pstCtrl;
    stUserHandle.ahUserHandle[1] = pstNode;
    pstNode->hVclockTimer = VCLOCK_CreateTimer(pstCtrl->hVClock, uiTimeOutTick, uiTimeOutTick, TIMER_FLAG_CYCLE, nat_tcp_TimeOut, &stUserHandle);
    if (NULL == pstNode->hVclockTimer)
    {
        nat_tcp_FreePort(pstCtrl, usPubPort);
        MEM_Free(pstNode);
        return NULL;
    }

    pstNode->stNatNode.uiPrivateIp = uiPrivateIp;
    pstNode->stNatNode.usPrivatePort = usPrivatePort;
    pstNode->stNatNode.uiPubIp = pstCtrl->auiPubIp[0];
    pstNode->stNatNode.usPubPort = usPubPort;
    pstNode->stNatNode.uiDomainId = uiDomainId;
    pstNode->stNatNode.ucStatus = NAT_TCP_STATUS_SYN_SEND;
    pstNode->stNatNode.ucType = IP_PROTO_TCP;
    
    HASH_Add(pstCtrl->hPrivateHashHandle, &pstNode->stPrivateListNode);
    HASH_Add(pstCtrl->hPubHashHandle, &pstNode->stPubListNode);

    return pstNode;
}

static inline VOID nat_tcp_StepPrivateStatus
(
    IN _NAT_TCP_MAP_NODE_S *pstNatMap,
    IN TCP_HEAD_S *pstTcpHead
)
{
    if (pstTcpHead->ucFlag & TCP_FLAG_RST)
    {
        pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_CLOSED;
    }

    if (pstTcpHead->ucFlag & TCP_FLAG_ACK)
    {
        if (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_SYN_RECEIVED)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_ESTABLISHED;
        }
        else if (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_I_TIME_WAIT)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_TIME_WAIT;
        }
        else if (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_O_FIN_WAIT_1)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_O_FIN_WAIT_2;
        }
    }

    if (pstTcpHead->ucFlag & TCP_FLAG_FIN)
    {
        if (pstNatMap->stNatNode.ucStatus <= NAT_TCP_STATUS_ESTABLISHED)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_I_FIN_WAIT_1;
        }
        else if (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_O_FIN_WAIT_2)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_O_TIME_WAIT;
        }
    }
}

static VOID nat_tcp_FreeMapNode(IN _NAT_TCP_CTRL_S *pstCtrl, IN _NAT_TCP_MAP_NODE_S *pstNode)
{
    HASH_Del(pstCtrl->hPrivateHashHandle, &pstNode->stPrivateListNode);
    HASH_Del(pstCtrl->hPubHashHandle, &pstNode->stPubListNode);
    VCLOCK_DestroyTimer(pstCtrl->hVClock, pstNode->hVclockTimer);
    nat_tcp_FreePort(pstCtrl, pstNode->stNatNode.usPubPort);
    MEM_Free(pstNode);
}

static inline _NAT_TCP_MAP_NODE_S * nat_tcp_ProcessPrivatePkt
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN _NAT_TCP_MAP_NODE_S *pstNatMap,
    IN TCP_HEAD_S *pstTcpHead
)
{
    nat_tcp_StepPrivateStatus(pstNatMap, pstTcpHead);

    if ((pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_CLOSED)
        || (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_TIME_WAIT))
    {
        nat_tcp_FreeMapNode(pstCtrl, pstNatMap);
        return NULL;
    }

    if (pstNatMap->stNatNode.ucStatus >= NAT_TCP_STATUS_ESTABLISHED)
    {
        VCLOCK_RestartWithTick(pstCtrl->hVClock, pstNatMap->hVclockTimer, pstCtrl->uiTcpTimeOutTick, 0);
    }
    else
    {
        VCLOCK_Refresh(pstCtrl->hVClock, pstNatMap->hVclockTimer);
    }

    return pstNatMap;
}

static BS_STATUS nat_tcp_PrivatePktIn
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN IP_HEAD_S  *pstIpHead,
    IN TCP_HEAD_S *pstTcpHead,
    IN UINT uiDomainId
)
{
    UINT uiIp;
    USHORT usPort;
    _NAT_TCP_MAP_NODE_S *pstNatMap;
    BS_STATUS eRet;

    uiIp = pstIpHead->unSrcIp.uiIp;
    usPort = pstTcpHead->usSrcPort;

    nat_tcp_Lock(pstCtrl);

    pstNatMap = nat_tcp_PrivateGetMapNode(pstCtrl, uiIp, usPort, uiDomainId);
    if (pstNatMap == NULL)
    {
        if (TRUE == nat_tcp_IsPermitAddTcpMap(pstTcpHead))
        {
            pstNatMap = nat_tcp_AddMapNode(pstCtrl, uiIp, usPort, uiDomainId);
        }
    }

    if (NULL == pstNatMap)
    {
        eRet = BS_NO_SUCH;
    }
    else
    {
        eRet = BS_OK;

        pstIpHead->unSrcIp.uiIp = pstNatMap->stNatNode.uiPubIp;
        pstTcpHead->usSrcPort = pstNatMap->stNatNode.usPubPort;
        
        pstNatMap = nat_tcp_ProcessPrivatePkt(pstCtrl, pstNatMap, pstTcpHead);
    }

    nat_tcp_UnLock(pstCtrl);

    return eRet;
}

static INT  nat_tcp_CmpForPub(IN VOID * pstHashNode1, IN VOID * pstNodeToFind)
{
    _NAT_TCP_MAP_NODE_S *pstNatNode, *pstNatNodeToFind;

    pstNatNode = BS_ENTRY(pstHashNode1, stPubListNode, _NAT_TCP_MAP_NODE_S);
    pstNatNodeToFind = BS_ENTRY(pstNodeToFind, stPubListNode, _NAT_TCP_MAP_NODE_S);

    return nat_tcp_Cmp(pstNatNode->stNatNode.uiPubIp,
                   pstNatNode->stNatNode.usPubPort,
                   pstNatNodeToFind->stNatNode.uiPubIp,
                   pstNatNodeToFind->stNatNode.usPubPort);
}

static _NAT_TCP_MAP_NODE_S *nat_tcp_PubGetMapNode
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN UINT uiIp,
    IN USHORT usPort
)
{
    _NAT_TCP_MAP_NODE_S *pstNode;
    _NAT_TCP_MAP_NODE_S stToFind;
    HASH_NODE_S *pstHashNode;

    stToFind.stNatNode.uiPubIp = uiIp;
    stToFind.stNatNode.usPubPort = usPort;

    pstHashNode = HASH_Find(pstCtrl->hPubHashHandle, nat_tcp_CmpForPub, &stToFind.stPubListNode);
    if (NULL == pstHashNode)
    {
        return NULL;
    }

    pstNode = BS_ENTRY(pstHashNode, stPubListNode, _NAT_TCP_MAP_NODE_S);

    return pstNode;
}

static VOID nat_tcp_StepPubStatus
(
    IN _NAT_TCP_MAP_NODE_S *pstNatMap,
    IN TCP_HEAD_S *pstTcpHead
)
{
    if (pstTcpHead->ucFlag & TCP_FLAG_RST)
    {
        pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_CLOSED;
    }

    if (pstTcpHead->ucFlag & TCP_FLAG_SYN)
    {
        if (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_SYN_SEND)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_SYN_RECEIVED;
        }
    }

    if (pstTcpHead->ucFlag & TCP_FLAG_ACK)
    {
        if (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_I_FIN_WAIT_1)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_I_FIN_WAIT_2;
        }
        else if (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_O_TIME_WAIT)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_TIME_WAIT;
        }
    }

    if (pstTcpHead->ucFlag & TCP_FLAG_FIN)
    {
        if (pstNatMap->stNatNode.ucStatus <= NAT_TCP_STATUS_ESTABLISHED)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_O_FIN_WAIT_1;
        }
        else if (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_I_FIN_WAIT_2)
        {
            pstNatMap->stNatNode.ucStatus = NAT_TCP_STATUS_I_TIME_WAIT;
        }
    }
}



static inline _NAT_TCP_MAP_NODE_S * nat_tcp_ProcessPubPkt
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN _NAT_TCP_MAP_NODE_S *pstNatMap,
    IN TCP_HEAD_S *pstTcpHead
)
{
    nat_tcp_StepPubStatus(pstNatMap, pstTcpHead);

    if ((pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_CLOSED)
        || (pstNatMap->stNatNode.ucStatus == NAT_TCP_STATUS_TIME_WAIT))
    {
        nat_tcp_FreeMapNode(pstCtrl, pstNatMap);
        return NULL;
    }

    if (pstNatMap->stNatNode.ucStatus >= NAT_TCP_STATUS_ESTABLISHED)
    {
        VCLOCK_RestartWithTick(pstCtrl->hVClock, pstNatMap->hVclockTimer, pstCtrl->uiTcpTimeOutTick, 0);
    }
    else
    {
        VCLOCK_Refresh(pstCtrl->hVClock, pstNatMap->hVclockTimer);
    }

    return pstNatMap;
}

static BS_STATUS nat_tcp_PubPktIn
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN IP_HEAD_S  *pstIpHead,
    IN TCP_HEAD_S *pstTcpHead,
    OUT UINT *puiDomainId
)
{
    UINT uiIp;
    USHORT usPort;
    _NAT_TCP_MAP_NODE_S *pstNatMap;
    USHORT usDstPort;
    BS_STATUS eRet;

    usDstPort = ntohs(pstTcpHead->usDstPort);
    if ((usDstPort < pstCtrl->usMinPort) || (usDstPort > pstCtrl->usMaxPort))
    {
        return BS_NO_PERMIT;
    }
    
    uiIp = pstIpHead->unDstIp.uiIp;
    usPort = pstTcpHead->usDstPort;

    nat_tcp_Lock(pstCtrl);
    
    pstNatMap = nat_tcp_PubGetMapNode(pstCtrl, uiIp, usPort);
    if (NULL == pstNatMap)
    {
        eRet = BS_NO_SUCH;
    }
    else
    {
        eRet = BS_OK;

        pstIpHead->unDstIp.uiIp = pstNatMap->stNatNode.uiPrivateIp;
        pstTcpHead->usDstPort = pstNatMap->stNatNode.usPrivatePort;
        *puiDomainId = pstNatMap->stNatNode.uiDomainId;

        pstNatMap = nat_tcp_ProcessPubPkt(pstCtrl, pstNatMap, pstTcpHead);
    }

    nat_tcp_UnLock(pstCtrl);

    return eRet;
}

static int nat_tcp_WalkEach(IN void * hHashId, IN VOID *pstNode, IN VOID * pUserHandle)
{
    _NAT_TCP_MAP_NODE_S *pstNatNode = (_NAT_TCP_MAP_NODE_S*)pstNode;
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_NAT_WALK_CALL_BACK_FUNC pfFunc;

    pfFunc = pstUserHandle->ahUserHandle[0];

    return pfFunc (&pstNatNode->stNatNode, pstUserHandle->ahUserHandle[1]);
}

static VOID nat_tcp_Walk
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN PF_NAT_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    HASH_Walk(pstCtrl->hPrivateHashHandle, nat_tcp_WalkEach, &stUserHandle);
}

BS_STATUS _NAT_TCP_Init
(
    IN _NAT_TCP_CTRL_S *pstTcpCtrl,
    IN USHORT usMinPort,   
    IN USHORT usMaxPort,   
    IN UINT   uiMsInTick,  
    IN BOOL_T bCreateMutex
)
{
    pstTcpCtrl->usMinPort = usMinPort;
    pstTcpCtrl->usMaxPort = usMaxPort;

    if (BS_OK != BITMAP_Create(&pstTcpCtrl->stTcpPortBitMap, usMaxPort - usMinPort + 1))
    {
        return BS_ERR;
    }

    pstTcpCtrl->hPrivateHashHandle = HASH_CreateInstance(NULL, _NAT_TCP_HASH_BUCKET_NUM, nat_tcp_PrivateHashIndex);
    if (NULL == pstTcpCtrl->hPrivateHashHandle)
    {
        _NAT_TCP_Fini(pstTcpCtrl);
        return BS_ERR;
    }

    pstTcpCtrl->hPubHashHandle = HASH_CreateInstance(NULL, _NAT_TCP_HASH_BUCKET_NUM, nat_tcp_PubHashIndex);
    if (NULL == pstTcpCtrl->hPubHashHandle)
    {
        _NAT_TCP_Fini(pstTcpCtrl);
        return BS_ERR;
    }

    pstTcpCtrl->hVClock = VCLOCK_CreateInstance(FALSE);
    if (NULL == pstTcpCtrl->hVClock)
    {
        _NAT_TCP_Fini(pstTcpCtrl);
        return BS_ERR;
    }

    if (bCreateMutex)
    {
        MUTEX_Init(&pstTcpCtrl->stMutex);
    }

    pstTcpCtrl->bCreateMutex = TRUE;

    pstTcpCtrl->uiTcpTimeOutTick = _NAT_GET_TICK_BY_TIME(_NAT_TCP_TIME_OUT_TIME_DFT, uiMsInTick);
    pstTcpCtrl->uiSynTcpTimeOutTick = _NAT_GET_TICK_BY_TIME(_NAT_TCP_TIME_OUT_TIME_SYN, uiMsInTick);

    return BS_OK;
}

VOID _NAT_TCP_Fini(IN _NAT_TCP_CTRL_S *pstTcpCtrl)
{
    BITMAP_Destory(&pstTcpCtrl->stTcpPortBitMap);

    if (NULL != pstTcpCtrl->hPrivateHashHandle)
    {
        HASH_DelAll(pstTcpCtrl->hPrivateHashHandle, nat_tcp_FreeAllMapNode, pstTcpCtrl);
        HASH_DestoryInstance(pstTcpCtrl->hPrivateHashHandle);
        pstTcpCtrl->hPrivateHashHandle = NULL;
    }

    if (NULL != pstTcpCtrl->hPubHashHandle)
    {
        HASH_DestoryInstance(pstTcpCtrl->hPubHashHandle);
        pstTcpCtrl->hPubHashHandle = NULL;
    }

    if (NULL != pstTcpCtrl->hVClock)
    {
        VCLOCK_DeleteInstance(pstTcpCtrl->hVClock);
        pstTcpCtrl->hVClock = NULL;
    }

    if (TRUE == pstTcpCtrl->bCreateMutex)
    {
        MUTEX_Final(&pstTcpCtrl->stMutex);
    }

    return;
}

BS_STATUS _NAT_TCP_PktIn
(
    IN _NAT_TCP_CTRL_S *pstTcpCtrl,
    IN IP_HEAD_S  *pstIpHead,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    IN BOOL_T bFromPub,
    INOUT UINT *puiDomainId
)
{
    TCP_HEAD_S *pstTcpHead = NULL;
    BS_STATUS eRet;

    pstTcpHead = TCP_GetTcpHeader(pucData, uiDataLen, NET_PKT_TYPE_IP);
    if (NULL == pstTcpHead)
    {
        return BS_ERR;
    }

    if (! bFromPub)
    {
        eRet = nat_tcp_PrivatePktIn(pstTcpCtrl, pstIpHead, pstTcpHead, *puiDomainId);
    }
    else
    {
        eRet = nat_tcp_PubPktIn(pstTcpCtrl, pstIpHead, pstTcpHead, puiDomainId);
    }

    if (eRet != BS_OK)
    {
        return eRet;
    }

    pstTcpHead->usCrc = 0;
    pstTcpHead->usCrc = TCP_CheckSum((UCHAR*)pstTcpHead, uiDataLen- IP_HEAD_LEN(pstIpHead),
                        (UCHAR*)&pstIpHead->unSrcIp.uiIp, (UCHAR*)&pstIpHead->unDstIp.uiIp);

    return BS_OK;
}

BS_STATUS _NAT_TCP_SetPubIp
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN UINT auiPubIp[NAT_MAX_PUB_IP_NUM] 
)
{
    UINT i;

    nat_tcp_Lock(pstCtrl);
    for (i=0; i<NAT_MAX_PUB_IP_NUM; i++)
    {
        pstCtrl->auiPubIp[i] = auiPubIp[i];
    }

    HASH_DelAll(pstCtrl->hPrivateHashHandle, nat_tcp_FreeAllMapNode, pstCtrl);
    HASH_DelAll(pstCtrl->hPubHashHandle, NULL, NULL);
    nat_tcp_UnLock(pstCtrl);

    return BS_OK;
}

VOID _NAT_TCP_TimerStep(IN _NAT_TCP_CTRL_S *pstCtrl)
{
    nat_tcp_Lock(pstCtrl);
    VCLOCK_Step(pstCtrl->hVClock);
    nat_tcp_UnLock(pstCtrl);
}

VOID _NAT_TCP_Walk
(
    IN _NAT_TCP_CTRL_S *pstCtrl,
    IN PF_NAT_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    nat_tcp_Lock(pstCtrl);
    nat_tcp_Walk(pstCtrl, pfFunc, hUserHandle);
    nat_tcp_UnLock(pstCtrl);
}

