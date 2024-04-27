/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-30
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_ARPUTL
    
#include "bs.h"

#include "utl/mem_utl.h"
#include "utl/sif_utl.h"
#include "utl/sem_utl.h"
#include "utl/arp_utl.h"
#include "utl/hash_utl.h"
#include "utl/vclock_utl.h"

#include "comp/comp_if.h"

#define _ARP_HASH_BUCKET_NUM 1024

#define _ARP_RETRY_TIME_INTERVAL 1    

typedef struct
{
    SEM_HANDLE hSem;
    MAC_ADDR_S stHostMac;
    HASH_S * hHashId;
    DLL_HEAD_S stResolvingList; 
    VCLOCK_INSTANCE_HANDLE hVclock;
    UINT uiTimeOutTick;  

    
    PF_ARP_SEND_PACKET_FUNC pfSendPacketFunc;
    USER_HANDLE_S stSendPacketUserHandle;
    PF_ARP_IS_HOST_IP_FUNC pfIsHostIp;
    USER_HANDLE_S stIsHostIpUserHandle;
    PF_ARP_GET_HOST_IP_FUNC pfGetHostIp;
    USER_HANDLE_S stGetHostIpUserHandle;

}_ARP_INSTANCE_S;

typedef struct
{
    HASH_NODE_S stHashNode;
    UINT ulIp;     
    MAC_ADDR_S stMacAddr;
    ARP_TYPE_E eType;
    VCLOCK_NODE_S vclock_node;
}_ARP_NODE_S;

typedef struct
{
    DLL_NODE_S stListNode;
    UINT ulIpToResolve;  
    VCLOCK_NODE_S vclock_node;
    MBUF_S *pstPacketList;
}_ARP_RESOLVING_NODE_S;

static UINT _arp_GetHashIndex(IN _ARP_NODE_S *pstArpNode)
{
    return ntohl(pstArpNode->ulIp);    
}

static UINT _arp_CmpHashNode(IN _ARP_NODE_S *pstArpNode1, IN _ARP_NODE_S *pstArpNode2)
{
    return (pstArpNode1->ulIp - pstArpNode2->ulIp);
}

static _ARP_RESOLVING_NODE_S * _arp_GetResolvingNode
(
    IN _ARP_INSTANCE_S *pstArpInstance,
    IN UINT ulIpToResolve
)
{
    _ARP_RESOLVING_NODE_S *pstNode;

    DLL_SCAN(&pstArpInstance->stResolvingList, pstNode)
    {
        if (ulIpToResolve == pstNode->ulIpToResolve)
        {
            return pstNode;
        }
    }

    return NULL;
}

static VOID _arp_ReSendMbufList
(
    IN _ARP_INSTANCE_S *pstArpInstance,
    IN MBUF_S *pstMbufList,
    IN UCHAR *pucDstMac
)
{
    MBUF_S *pstMbuf, *pstMbufNext;

    pstMbuf = pstMbufList;

    while (pstMbuf != NULL)
    {
        pstMbufNext = MBUF_GET_NEXT_MBUF(pstMbuf);
        MBUF_SET_NEXT_MBUF(pstMbuf, NULL);

        MBUF_SET_DESTMAC(pstMbuf, pucDstMac);
        MBUF_SET_ETH_MARKFLAG(pstMbuf, MBUF_L2_FLAG_DST_MAC);

        pstArpInstance->pfSendPacketFunc(pstMbuf, &pstArpInstance->stSendPacketUserHandle);
        
        pstMbuf = pstMbufNext;
    }
}

static VOID _arp_FreeResolvingNode(IN _ARP_INSTANCE_S *pstArpInstance, IN _ARP_RESOLVING_NODE_S *pstNode)
{
    MBUF_S *pstMbuf, *pstMbufNext;

    DLL_DEL(&pstArpInstance->stResolvingList, pstNode);

    pstMbuf = pstNode->pstPacketList;
    pstNode->pstPacketList = NULL;
    
    while (pstMbuf != NULL)
    {
        pstMbufNext = MBUF_GET_NEXT_MBUF(pstMbuf);
        MBUF_Free(pstMbuf);
        pstMbuf = pstMbufNext;
    }

    MEM_Free(pstNode);
}

static VOID _arp_ResolvingTimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    _ARP_INSTANCE_S *pstArpInstance = pstUserHandle->ahUserHandle[0];
    _ARP_RESOLVING_NODE_S *pstNode = pstUserHandle->ahUserHandle[1];

    _arp_FreeResolvingNode(pstArpInstance, pstNode);
}

static _ARP_RESOLVING_NODE_S * _arp_AddResolvingNode
(
    IN _ARP_INSTANCE_S *pstArpInstance,
    IN UINT ulIpToResolve
)
{
    _ARP_RESOLVING_NODE_S *pstNode;
    USER_HANDLE_S stUserHandle;

    pstNode = MEM_ZMalloc(sizeof(_ARP_RESOLVING_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    stUserHandle.ahUserHandle[0] = pstArpInstance;
    stUserHandle.ahUserHandle[1] = pstNode;
    VCLOCK_AddTimer(pstArpInstance->hVclock, &pstNode->vclock_node,
        _ARP_RETRY_TIME_INTERVAL, _ARP_RETRY_TIME_INTERVAL, 0, _arp_ResolvingTimeOut, &stUserHandle);
    pstNode->ulIpToResolve = ulIpToResolve;

    DLL_ADD(&pstArpInstance->stResolvingList, pstNode);

    return pstNode;
}

static MBUF_S * _arp_BuildPacket
(
    IN _ARP_INSTANCE_S *pstArpInstance,
    IN UINT uiIfIndex, 
    IN UINT uiSrcIp,
    IN UINT uiDstIp, 
    IN UCHAR *pucDstArpMac,
    IN USHORT usArpType
)
{
    MBUF_S *pstMbuf;
    MAC_ADDR_S stSrcMac;
    MAC_ADDR_S *pstSrcMac;

    
    IFNET_Ioctl(uiIfIndex, IFNET_CMD_GET_MAC, (HANDLE)&stSrcMac);
    pstSrcMac = &stSrcMac;
    if (MAC_ADDR_IS_ZERO(pstSrcMac->aucMac))
    {
        pstSrcMac = &pstArpInstance->stHostMac;
    }

    pstMbuf = ARP_BuildPacket(uiSrcIp, uiDstIp, pstSrcMac->aucMac, pucDstArpMac, usArpType);

    MBUF_SET_SEND_IF_INDEX(pstMbuf, uiIfIndex);

    return pstMbuf;
}

static VOID _arp_AddPacketToResolvingNode
(
    IN _ARP_RESOLVING_NODE_S *pstResolvingNode,
    IN MBUF_S *pstMbuf
)
{
    BS_DBGASSERT(NULL == MBUF_GET_NEXT_MBUF(pstMbuf));
    MBUF_SET_NEXT_MBUF(pstMbuf, pstResolvingNode->pstPacketList);
    pstResolvingNode->pstPacketList = pstMbuf;
}


static BS_STATUS _arp_ResolveIp
(
    IN _ARP_INSTANCE_S *pstArpInstance,
    IN UINT uiIfIndex,
    IN UINT ulIpToResolve,
    IN MBUF_S *pstMbuf
)
{
    MBUF_S *pstArpMbuf;
    _ARP_RESOLVING_NODE_S *pstResolvingNode;
    UINT uiHostIP;

    
    pstResolvingNode = _arp_GetResolvingNode(pstArpInstance, ulIpToResolve);
    if (NULL != pstResolvingNode)
    {
        _arp_AddPacketToResolvingNode(pstResolvingNode, pstMbuf);
        return BS_OK;
    }

    uiHostIP = pstArpInstance->pfGetHostIp(uiIfIndex, ulIpToResolve, &pstArpInstance->stGetHostIpUserHandle);
    if (uiHostIP == 0)
    {
        return BS_NOT_FOUND;
    }

    
    pstArpMbuf = _arp_BuildPacket(pstArpInstance, uiIfIndex,
                       uiHostIP,
                       ulIpToResolve, NULL, ARP_REQUEST);
    if (NULL == pstArpMbuf)
    {
        RETURN(BS_NO_MEMORY);
    }

    
    pstResolvingNode = _arp_AddResolvingNode(pstArpInstance, ulIpToResolve);
    if (NULL == pstResolvingNode)
    {
        MBUF_Free(pstArpMbuf);
        RETURN(BS_NO_MEMORY);
    }

    _arp_AddPacketToResolvingNode(pstResolvingNode, pstMbuf);

    pstArpInstance->pfSendPacketFunc (pstArpMbuf, &pstArpInstance->stSendPacketUserHandle);

    return BS_OK;
}

static inline BS_STATUS _arp_GetMacByIp
(
    IN _ARP_INSTANCE_S *pstArpInstance,
    IN UINT uiIfIndex,
    IN UINT ulIpToResolve,
    IN MBUF_S *pstMbuf,
    OUT MAC_ADDR_S *pstMacAddr
)
{
    _ARP_NODE_S *pstArpNode;
    _ARP_NODE_S stArpNode;

    stArpNode.ulIp = ulIpToResolve;

    pstArpNode = (_ARP_NODE_S *)HASH_Find(pstArpInstance->hHashId, (PF_HASH_CMP_FUNC)_arp_CmpHashNode, (HASH_NODE_S*)&stArpNode);
    if (NULL != pstArpNode)
    {
        *pstMacAddr = pstArpNode->stMacAddr;
        return BS_OK;
    }

    if (BS_OK != _arp_ResolveIp(pstArpInstance, uiIfIndex, ulIpToResolve, pstMbuf))
    {
        RETURN(BS_ERR);
    }

    return BS_PROCESSED;
}

static VOID _arp_NodeTimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    _ARP_INSTANCE_S *pstArpInstance = pstUserHandle->ahUserHandle[0];
    _ARP_NODE_S *pstNode = pstUserHandle->ahUserHandle[1];

    if (pstNode->eType == ARP_TYPE_STATIC)
    {
        return;
    }

    HASH_Del(pstArpInstance->hHashId, (HASH_NODE_S*)pstNode);
    MEM_Free(pstNode);
}

static inline BS_STATUS _arp_Add
(
    IN _ARP_INSTANCE_S *pstArpInstance,
    IN UINT ulIp ,
    IN MAC_ADDR_S *pstMac,
    IN ARP_TYPE_E eType
)
{
    _ARP_NODE_S *pstNode;
    _ARP_NODE_S stArpNode;
    USER_HANDLE_S stUserHandle;

    stArpNode.ulIp = ulIp;
    pstNode = (_ARP_NODE_S *)HASH_Find(pstArpInstance->hHashId, (PF_HASH_CMP_FUNC)_arp_CmpHashNode, (HASH_NODE_S*)&stArpNode);
    if (NULL != pstNode)
    {
        if ((pstNode->eType == ARP_TYPE_STATIC) && (eType == ARP_TYPE_DYNAMIC))
        {
            return BS_OK;
        }

        pstNode->stMacAddr = *pstMac;
        pstNode->eType = eType;

        

        return BS_OK;
    }

    pstNode = MEM_ZMalloc(sizeof(_ARP_NODE_S));
    if (NULL == pstNode)
    {
        RETURN(BS_NO_MEMORY);
    }

    if (eType == ARP_TYPE_DYNAMIC)
    {
        stUserHandle.ahUserHandle[0] = pstArpInstance;
        stUserHandle.ahUserHandle[1] = pstNode;
        VCLOCK_AddTimer(pstArpInstance->hVclock, &pstNode->vclock_node,
                pstArpInstance->uiTimeOutTick, pstArpInstance->uiTimeOutTick, 0, _arp_NodeTimeOut, &stUserHandle);
    }

    pstNode->ulIp = ulIp;
    pstNode->stMacAddr = *pstMac;
    pstNode->eType = eType;

    HASH_Add(pstArpInstance->hHashId, (HASH_NODE_S*)pstNode);

    return BS_OK;
}

static inline BS_STATUS _arp_InnerDealArpRequest(IN _ARP_INSTANCE_S *pstArpInstance, IN MBUF_S *pstMbuf)
{
    ARP_HEADER_S *pstArpHeader;
    MBUF_S *pstReply;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(ARP_HEADER_S)))
    {
        MBUF_Free(pstMbuf);
        RETURN(BS_ERR);
    }

    pstArpHeader = MBUF_MTOD(pstMbuf);

    if (FALSE == pstArpInstance->pfIsHostIp(MBUF_GET_RECV_IF_INDEX(pstMbuf), pstArpHeader, &pstArpInstance->stIsHostIpUserHandle))
    {
        MBUF_Free(pstMbuf);
        RETURN(BS_NO_SUCH);
    }

    SEM_LOCK_IF_EXIST(pstArpInstance->hSem);
    _arp_Add(pstArpInstance, pstArpHeader->ulSenderIp, (MAC_ADDR_S*)(pstArpHeader->aucSenderHA), ARP_TYPE_DYNAMIC);
    SEM_UNLOCK_IF_EXIST(pstArpInstance->hSem);    

    pstReply = _arp_BuildPacket(pstArpInstance, MBUF_GET_RECV_IF_INDEX(pstMbuf),
                    pstArpHeader->ulDstIp, pstArpHeader->ulSenderIp,
                    pstArpHeader->aucSenderHA,
                    ARP_REPLY);

    MBUF_Free(pstMbuf);

    return pstArpInstance->pfSendPacketFunc (pstReply, &pstArpInstance->stSendPacketUserHandle);
}

static BS_STATUS _arp_DealArpRequest(IN ARP_HANDLE hArpInstance, IN MBUF_S *pstMbuf)
{
    BS_STATUS eRet;
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    eRet = _arp_InnerDealArpRequest(pstArpInstance, pstMbuf);

    return eRet;
}

static inline BS_STATUS _arp_InnerDealArpReplay
(
    IN _ARP_INSTANCE_S *pstArpInstance,
    IN ARP_HEADER_S *pstArpHeader,
    IN MBUF_S **pstSendMbufList
)
{
    UINT ulIp;
    _ARP_RESOLVING_NODE_S *pstNode;

    ulIp = pstArpHeader->ulSenderIp;

    pstNode = _arp_GetResolvingNode(pstArpInstance, ulIp);
    if (NULL == pstNode)
    {
        return BS_OK;
    }

    VCLOCK_DelTimer(pstArpInstance->hVclock, &pstNode->vclock_node);

    *pstSendMbufList = pstNode->pstPacketList;
    pstNode->pstPacketList = NULL;

    _arp_FreeResolvingNode(pstArpInstance, pstNode);

    _arp_Add(pstArpInstance, ulIp, (MAC_ADDR_S*)(pstArpHeader->aucSenderHA), ARP_TYPE_DYNAMIC);

    return BS_OK;
}

static BS_STATUS _arp_DealArpReply(IN ARP_HANDLE hArpInstance, IN ARP_HEADER_S *pstArpHeader)
{
    BS_STATUS eRet;
    MBUF_S *pstMbufList = NULL;
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    SEM_LOCK_IF_EXIST(pstArpInstance->hSem);
    eRet = _arp_InnerDealArpReplay(pstArpInstance, pstArpHeader, &pstMbufList);
    SEM_UNLOCK_IF_EXIST(pstArpInstance->hSem);

    if (pstMbufList != NULL)
    {
        _arp_ReSendMbufList(pstArpInstance, pstMbufList, pstArpHeader->aucSenderHA);
    }

    return eRet;
}

static VOID _arp_FreeAllNode(IN void * hHashId, IN VOID *pstNode, IN VOID * pUserHandle)
{
    _ARP_NODE_S *pstArpNode = (_ARP_NODE_S*)pstNode;

    MEM_Free(pstArpNode);
}

static int _arp_WalkEach(IN HASH_S * hHashId, IN HASH_NODE_S *pstNode, IN VOID * pUserHandle)
{
    _ARP_NODE_S *pstArpNode = (_ARP_NODE_S*)pstNode;
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    ARP_NODE_S stArp;
    PF_ARP_WALK_CALL_BACK_FUNC pfFunc;

    pfFunc = pstUserHandle->ahUserHandle[0];

    stArp.uiIp = pstArpNode->ulIp;
    stArp.stMac = pstArpNode->stMacAddr;
    stArp.eType = pstArpNode->eType;

    return pfFunc (&stArp, pstUserHandle->ahUserHandle[1]);
}

static VOID _arp_Walk
(
    IN _ARP_INSTANCE_S *pstArpInstance,
    IN PF_ARP_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    HASH_Walk(pstArpInstance->hHashId, (PF_HASH_WALK_FUNC)_arp_WalkEach, &stUserHandle);
}

ARP_HANDLE ARP_CreateInstance
(
    IN UINT uiTimeOutTick,  
    IN BOOL_T bIsCreateSem
)
{
    _ARP_INSTANCE_S *pstArpInstance;

    pstArpInstance = MEM_ZMalloc(sizeof(_ARP_INSTANCE_S));
    if (NULL == pstArpInstance)
    {
        return 0;
    }

    if (bIsCreateSem == TRUE)
    {
        if (0 == (pstArpInstance->hSem = SEM_CCreate("ArpSem", 1)))
        {
            MEM_Free(pstArpInstance);
            return 0;
        }
    }

    pstArpInstance->hHashId = HASH_CreateInstance(NULL, _ARP_HASH_BUCKET_NUM, (PF_HASH_INDEX_FUNC)_arp_GetHashIndex);
    if (0 == pstArpInstance->hHashId)
    {
        if (bIsCreateSem == TRUE)
        {
            SEM_Destory(pstArpInstance->hSem);
        }
        MEM_Free(pstArpInstance);
        return 0;
    }

    pstArpInstance->hVclock = VCLOCK_CreateInstance(FALSE);
    if (NULL == pstArpInstance->hVclock)
    {
        HASH_DestoryInstance(pstArpInstance->hHashId);
        
        if (bIsCreateSem == TRUE)
        {
            SEM_Destory(pstArpInstance->hSem);
        }
        MEM_Free(pstArpInstance);
        return 0;
    }

    pstArpInstance->uiTimeOutTick = uiTimeOutTick;
    
    DLL_INIT(&pstArpInstance->stResolvingList);

    return pstArpInstance;
}

VOID ARP_DestoryInstance(IN ARP_HANDLE hArpInstance)
{
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    if (NULL != pstArpInstance)
    {
        HASH_DelAll(pstArpInstance->hHashId, _arp_FreeAllNode, NULL);
        HASH_DestoryInstance(pstArpInstance->hHashId);
        VCLOCK_DeleteInstance(pstArpInstance->hVclock);
        if (0 != pstArpInstance->hSem)
        {
            SEM_Destory(pstArpInstance->hSem);
        }
        MEM_Free(pstArpInstance);
    }
}

VOID ARP_SetSendPacketFunc
(
    IN ARP_HANDLE hArpInstance,
    IN PF_ARP_SEND_PACKET_FUNC pfSendPacketFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    pstArpInstance->pfSendPacketFunc = pfSendPacketFunc;
    if (NULL != pstUserHandle)
    {
        pstArpInstance->stSendPacketUserHandle = *pstUserHandle;
    }
}

VOID ARP_SetIsHostIpFunc
(
    IN ARP_HANDLE hArpInstance,
    IN PF_ARP_IS_HOST_IP_FUNC pfIsHostIp,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    pstArpInstance->pfIsHostIp = pfIsHostIp;
    if (NULL != pstUserHandle)
    {
        pstArpInstance->stIsHostIpUserHandle = *pstUserHandle;
    }
}

VOID ARP_SetGetHostIpFunc
(
    IN ARP_HANDLE hArpInstance,
    IN PF_ARP_GET_HOST_IP_FUNC pfGetHostIp,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    pstArpInstance->pfGetHostIp = pfGetHostIp;
    if (NULL != pstUserHandle)
    {
        pstArpInstance->stGetHostIpUserHandle = *pstUserHandle;
    }
}

BS_STATUS ARP_SetHostMac(IN ARP_HANDLE hArpInstance, IN MAC_ADDR_S *pstHostMac)
{
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    pstArpInstance->stHostMac = *pstHostMac;

	return BS_OK;
}

BS_STATUS ARP_AddStaticARP
(
    IN ARP_HANDLE hArpInstance,
    IN UINT uiIP, 
    IN MAC_ADDR_S *pstMac
)
{
    BS_STATUS eRet;
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    SEM_LOCK_IF_EXIST(pstArpInstance->hSem);
    eRet = _arp_Add(pstArpInstance, uiIP, pstMac, ARP_TYPE_STATIC);
    SEM_UNLOCK_IF_EXIST(pstArpInstance->hSem);

    return eRet;
}


BS_STATUS ARP_GetMacByIp
(
    IN ARP_HANDLE hArpInstance,
    IN UINT uiIfIndex,
    IN UINT ulIpToResolve ,
    IN MBUF_S *pstMbuf,
    OUT MAC_ADDR_S *pstMacAddr
)
{
    BS_STATUS eRet;
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    SEM_LOCK_IF_EXIST(pstArpInstance->hSem);
    eRet = _arp_GetMacByIp(pstArpInstance, uiIfIndex, ulIpToResolve, pstMbuf, pstMacAddr);
    SEM_UNLOCK_IF_EXIST(pstArpInstance->hSem);

    return eRet;
}

BS_STATUS ARP_PacketInput(IN ARP_HANDLE hArpInstance, IN MBUF_S *pstArpPacket)
{
    ARP_HEADER_S *pstArpHeader;

    if (BS_OK != MBUF_MakeContinue(pstArpPacket, sizeof(ARP_HEADER_S)))
    {
        MBUF_Free(pstArpPacket);
        RETURN(BS_ERR);
    }

    pstArpHeader = MBUF_MTOD(pstArpPacket);

    switch (ntohs(pstArpHeader->usOperation))
    {
        case ARP_REQUEST:
            
            (VOID) _arp_DealArpRequest(hArpInstance, pstArpPacket);
            break;

        case ARP_REPLY:
            (VOID) _arp_DealArpReply(hArpInstance, pstArpHeader);
            MBUF_Free(pstArpPacket);
            break;

        default:
            MBUF_Free(pstArpPacket);
            break;
    }

    return BS_OK;    
}

VOID ARP_Walk
(
    IN ARP_HANDLE hArpInstance,
    IN PF_ARP_WALK_CALL_BACK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    SEM_LOCK_IF_EXIST(pstArpInstance->hSem);
    _arp_Walk(pstArpInstance, pfFunc, hUserHandle);
    SEM_UNLOCK_IF_EXIST(pstArpInstance->hSem);
}

VOID ARP_TimerStep(IN ARP_HANDLE hArpInstance)
{
    _ARP_INSTANCE_S *pstArpInstance = (_ARP_INSTANCE_S*)hArpInstance;

    SEM_LOCK_IF_EXIST(pstArpInstance->hSem);
    VCLOCK_Step(pstArpInstance->hVclock);
    SEM_UNLOCK_IF_EXIST(pstArpInstance->hSem);
}



