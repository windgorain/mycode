/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-9-22
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/mac_table.h"
#include "utl/ippool_utl.h"
#include "utl/mbuf_utl.h"
#include "utl/udp_utl.h"
#include "utl/dhcp_utl.h"
#include "utl/dhcps_utl.h"

#define _DHCPS_MBUF_HEAD_SPACE_LEN 200
#define _DHCPS_OPTIONS_BUFFER_SIZE 256

#define _DHCPS_DECLINE_MAX_TICK 3600  
#define _DHCPS_MAC_OLD_TIME     3600  

typedef struct
{
    IPPOOL_HANDLE hIpPool;
    UINT uiMask;        
    UINT uiGateWayIP;   
    MACTBL_HANDLE hMacTbl;         
    MACTBL_HANDLE hSignedMacTbl;   
    PF_DHCPS_SEND_FUNC pfSendFunc;
    USER_HANDLE_S stUserHandle;
    UINT uiDeclineTick;
}_DHCPS_INSTANCE_S;

typedef struct
{
    UINT uiIP; 
}_DHCPS_DATA_S;


static UINT dhcps_GetIp
(
    IN _DHCPS_INSTANCE_S *pstInstance,
    IN DHCP_HEAD_S *pstDhcpRequest,
    IN UINT uiRequestIp
)
{
    MAC_NODE_S stMacTbl;
    UINT uiIP;
    BOOL_T bAllocIp = FALSE;
    _DHCPS_DATA_S stData;

    MAC_ADDR_COPY(stMacTbl.stMac.aucMac, pstDhcpRequest->aucClientHaddr);

    
    if ((BS_OK == MACTBL_Find(pstInstance->hMacTbl, &stMacTbl, &stData))
        && (stData.uiIP != 0))
    {
        return stData.uiIP;
    }

    
    if (BS_OK == MACTBL_Find(pstInstance->hSignedMacTbl, &stMacTbl, &stData))
    {
        uiIP = stData.uiIP;
    }
    else
    {
        uiIP = IPPOOL_AllocIP(pstInstance->hIpPool, ntohl(uiRequestIp));
        uiIP = htonl(uiIP);
        bAllocIp = TRUE;
    }

    if (0 == uiIP)
    {
        return 0;
    }

    stData.uiIP = uiIP;
    stMacTbl.uiFlag = 0;
    if (BS_OK != MACTBL_Add(pstInstance->hMacTbl, &stMacTbl, &stData, MAC_MODE_SET))
    {
        if (bAllocIp == TRUE)
        {
            IPPOOL_FreeIP(pstInstance->hIpPool, ntohl(uiIP));
        }
        return 0;
    }

    return uiIP;
}


static UINT dhcps_GetMask(IN _DHCPS_INSTANCE_S *pstInstance)
{
    return htonl(pstInstance->uiMask);
}


static UINT dhcps_GetServerIP(IN _DHCPS_INSTANCE_S *pstInstance)
{
    return htonl(pstInstance->uiGateWayIP);
}

static VOID dhcps_BuildDHCPPre
(
    IN _DHCPS_INSTANCE_S *pstInstance,
    IN DHCP_HEAD_S *pstDhcpRequest,
    OUT DHCP_HEAD_S *pstDhcpResponse,
    IN UINT ulYourIp,  
    IN int optlen
)
{
    pstDhcpResponse->ucOp = DHCP_OP_REPLY;
    pstDhcpResponse->ucHType = 1;
    pstDhcpResponse->ucHLen = sizeof (MAC_ADDR_S);
    pstDhcpResponse->ucHops = 0;
    pstDhcpResponse->ulXid = pstDhcpRequest->ulXid;
    pstDhcpResponse->usSecs = 0;
    pstDhcpResponse->usFlags = 0;
    pstDhcpResponse->ulClientIp = 0;

    pstDhcpResponse->ulYourIp = ulYourIp;

    pstDhcpResponse->ulServerIp = dhcps_GetServerIP(pstInstance);
    pstDhcpResponse->ulRelayIp = 0;
    pstDhcpResponse->aucBootFile[0] = '\0';
    pstDhcpResponse->aucServerName[0] = '\0';
    
    MAC_ADDR_COPY(pstDhcpResponse->aucClientHaddr, pstDhcpRequest->aucClientHaddr);
    pstDhcpResponse->ulMagic = htonl (0x63825363);
}

static BS_STATUS dhcps_SendDhcpMsg
(
    IN _DHCPS_INSTANCE_S *pstInstance,
    IN DHCP_HEAD_S *pstDhcpHead,
    IN UINT ulMsgType,
    IN UINT ulYourlIp, 
    IN VOID *pPktContext
)
{
    MBUF_CLUSTER_S *pstCluster;
    MBUF_S *pstReplayMbuf;
    UCHAR *pucData;
    UCHAR *pucOpt;
    UINT ulOptMaxLen;
    UINT ulOptLenCopyed;
    UINT ulServerVnetIp;
    UINT uiMask;

    pstCluster = MBUF_CreateCluster();
    if (NULL == pstCluster)
    {
        return(BS_NO_RESOURCE);
    }

    BS_DBGASSERT(_MBUF_POOL_DFT_CLUSTER_SIZE - _DHCPS_MBUF_HEAD_SPACE_LEN 
        >= sizeof (DHCP_HEAD_S) + _DHCPS_OPTIONS_BUFFER_SIZE);

    pucData = pstCluster->pucData + _DHCPS_MBUF_HEAD_SPACE_LEN;
    pucOpt = pucData + sizeof (DHCP_HEAD_S);
    ulOptMaxLen = _DHCPS_OPTIONS_BUFFER_SIZE;
    ulOptLenCopyed = 0;

    
    ulOptLenCopyed += DHCP_SetOpt8 (pucOpt + ulOptLenCopyed, DHCP_MSG_TYPE,
                              (UCHAR)ulMsgType, ulOptMaxLen - ulOptLenCopyed);

    
    ulServerVnetIp = dhcps_GetServerIP(pstInstance);
    ulOptLenCopyed += DHCP_SetOpt32 (pucOpt + ulOptLenCopyed,
                        DHCP_SERVER_ID, ulServerVnetIp, ulOptMaxLen - ulOptLenCopyed);

    if ((ulMsgType == DHCP_ACK) || (ulMsgType == DHCP_OFFER))
    {
        
        ulOptLenCopyed += DHCP_SetOpt32 (pucOpt + ulOptLenCopyed,
                        DHCP_LEASE_TIME, htonl(3600 * 24 * 365), ulOptMaxLen - ulOptLenCopyed);

        
        uiMask = dhcps_GetMask (pstInstance);
        ulOptLenCopyed += DHCP_SetOpt32 (pucOpt + ulOptLenCopyed,
                        DHCP_NETMASK, uiMask, ulOptMaxLen - ulOptLenCopyed);
    }

    if (ulMsgType == DHCP_ACK)
    {
        
        ulOptLenCopyed += DHCP_SetOpt32 (pucOpt + ulOptLenCopyed, DHCP_GATEWAY,
                            ulServerVnetIp, ulOptMaxLen - ulOptLenCopyed);

        
        ulOptLenCopyed += DHCP_SetOpt32 (pucOpt + ulOptLenCopyed, DHCP_DOMAIN_NAME_SERVER,
                            ulServerVnetIp, ulOptMaxLen - ulOptLenCopyed);
    }

    
    ulOptLenCopyed += DHCP_SetOpt0 (pucOpt + ulOptLenCopyed, DHCP_END, ulOptMaxLen - ulOptLenCopyed);

    pstReplayMbuf = MBUF_CreateByCluster(pstCluster, _DHCPS_MBUF_HEAD_SPACE_LEN, 
                sizeof (DHCP_HEAD_S) + ulOptLenCopyed, 0);

    if (NULL == pstReplayMbuf)
    {
        return(BS_NO_RESOURCE);
    }

    dhcps_BuildDHCPPre (pstInstance, pstDhcpHead, (DHCP_HEAD_S*)pucData, ulYourlIp, ulOptLenCopyed);

    MBUF_SET_DESTMAC(pstReplayMbuf, pstDhcpHead->aucClientHaddr);
    MBUF_SET_ETH_MARKFLAG(pstReplayMbuf, MBUF_L2_FLAG_DST_MAC);

    return pstInstance->pfSendFunc(pstReplayMbuf, &pstInstance->stUserHandle, pPktContext);
}


static BS_STATUS dhcps_SendDhcpOffer
(
    IN _DHCPS_INSTANCE_S *pstInstance,
    IN DHCP_HEAD_S *pstDhcpHead,
    IN UINT ulRequestIp, 
    IN VOID *pPktContext
)
{
    UINT ulIp;

    ulIp = dhcps_GetIp(pstInstance, pstDhcpHead, ulRequestIp);

    if (0 == ulIp)
    {
        return(BS_NO_RESOURCE);
    }

    if (BS_OK != dhcps_SendDhcpMsg(pstInstance, pstDhcpHead, DHCP_OFFER, ulIp, pPktContext))
    {
        return(BS_ERR);
    }

    return BS_OK;
}

static BS_STATUS dhcps_SendDhcpAck
(
    IN _DHCPS_INSTANCE_S *pstInstance,
    IN DHCP_HEAD_S *pstDhcpHead,
    IN UINT ulRequestIp, 
    IN VOID *pPktContext
)
{
    UINT ulIp;
    
    ulIp = dhcps_GetIp(pstInstance, pstDhcpHead, ulRequestIp);
    if (ulIp == 0)
    {
        return(BS_NO_RESOURCE);
    }

    if (ulRequestIp != ulIp)
    {
        return dhcps_SendDhcpMsg (pstInstance, pstDhcpHead, DHCP_NACK, ulIp, pPktContext);
    }

    return dhcps_SendDhcpMsg (pstInstance, pstDhcpHead, DHCP_ACK, ulIp, pPktContext);
}

static BS_STATUS dhcps_DealDecline
(
    IN _DHCPS_INSTANCE_S *pstInstance,
    IN DHCP_HEAD_S *pstDhcpHead,
    IN UINT uiDeclineIp, 
    IN VOID *pPktContext
)
{
    MAC_NODE_S stMacTbl;

    MAC_ADDR_COPY(stMacTbl.stMac.aucMac, pstDhcpHead->aucClientHaddr);

    if (BS_OK != MACTBL_Find(pstInstance->hMacTbl, &stMacTbl, NULL))
    {
        return BS_OK;
    }
    
    MACTBL_Del(pstInstance->hMacTbl, &stMacTbl);
    IPPOOL_Deny(pstInstance->hIpPool, ntohl(uiDeclineIp));

    return BS_OK;
}

static VOID dhcps_MacDel
(
    IN MACTBL_HANDLE hMacTbl,
    IN UINT uiEvent,
    IN MAC_NODE_S *pstNode,
    IN VOID *pData,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _DHCPS_INSTANCE_S *pstInstance = pstUserHandle->ahUserHandle[0];
    _DHCPS_DATA_S *pstData = pData;
    UINT uiIP;

    if (NULL == pstInstance->hIpPool)
    {
        return;
    }

    uiIP = pstData->uiIP;
    if (uiIP == 0)
    {
        return;
    }

    uiIP = ntohl(uiIP);

    IPPOOL_FreeIP(pstInstance->hIpPool, uiIP);
}

DHCPS_HANDLE DHCPS_CreateInstance
(
    IN DHCP_IP_CONF_S * pstIpConf,
    IN PF_DHCPS_SEND_FUNC pfSendFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _DHCPS_INSTANCE_S *pstInstance;
    USER_HANDLE_S stUserHandle;

    pstInstance = MEM_ZMalloc(sizeof(_DHCPS_INSTANCE_S));
    if (NULL == pstInstance)
    {
        return NULL;
    }

    pstInstance->hIpPool = IPPOOL_Create();
    if (NULL == pstInstance->hIpPool)
    {
        DHCPS_DestoryInstance(pstInstance);
        return NULL;
    }

    IPPOOL_AddRange(pstInstance->hIpPool, pstIpConf->uiStartIp, pstIpConf->uiEndIp);

    pstInstance->uiMask = pstIpConf->uiMask;
    pstInstance->uiGateWayIP = pstIpConf->uiGateway;

    pstInstance->hMacTbl = MACTBL_CreateInstance(sizeof(_DHCPS_DATA_S));
    if (NULL == pstInstance->hMacTbl) {
        DHCPS_DestoryInstance(pstInstance);
        return NULL;
    }
    MACTBL_SetOldTick(pstInstance->hMacTbl, _DHCPS_MAC_OLD_TIME);    

    pstInstance->hSignedMacTbl = MACTBL_CreateInstance(sizeof(_DHCPS_DATA_S));
    if (NULL == pstInstance->hSignedMacTbl)
    {
        DHCPS_DestoryInstance(pstInstance);
        return NULL;
    }

    pstInstance->pfSendFunc = pfSendFunc;
    if (pstUserHandle)
    {
        pstInstance->stUserHandle = *pstUserHandle;
    }

    stUserHandle.ahUserHandle[0] = pstInstance;
    MACTBL_SetNotify(pstInstance->hMacTbl, MAC_TBL_EVENT_OLD | MAC_TBL_EVENT_DEL, dhcps_MacDel, &stUserHandle);

    return pstInstance;
}

VOID DHCPS_DestoryInstance(IN DHCPS_HANDLE hDhcpInstance)
{
    _DHCPS_INSTANCE_S *pstInstance = hDhcpInstance;

    if (NULL == pstInstance)
    {
        return;
    }

    if (pstInstance->hMacTbl)
    {
        MACTBL_DestoryInstance(pstInstance->hMacTbl);
        pstInstance->hMacTbl = NULL;
    }

    if (pstInstance->hSignedMacTbl)
    {
        MACTBL_DestoryInstance(pstInstance->hSignedMacTbl);
        pstInstance->hSignedMacTbl = NULL;
    }

    if (pstInstance->hIpPool)
    {
        IPPOOL_Destory(pstInstance->hIpPool);
        pstInstance->hIpPool = NULL;
    }

    MEM_Free(pstInstance);
}


BS_STATUS DHCPS_SignIP
(
    IN DHCPS_HANDLE hDhcpInstance,
    IN MAC_ADDR_S *pstMacAddr,
    IN UINT uiIp    
)
{
    _DHCPS_INSTANCE_S *pstInstance = hDhcpInstance;
    MAC_NODE_S stMacNode;
    _DHCPS_DATA_S stData = {0};

    if (NULL == pstInstance)
    {
        return BS_NULL_PARA;
    }

    stMacNode.stMac = *pstMacAddr;
    
    if (BS_OK == MACTBL_Find(pstInstance->hSignedMacTbl, &stMacNode, &stData))
    {
        IPPOOL_FreeIP(pstInstance->hIpPool, ntohl(stData.uiIP));
        MACTBL_Del(pstInstance->hSignedMacTbl, &stMacNode);
    }

    IPPOOL_AllocSpecIP(pstInstance->hIpPool, ntohl(uiIp));

    stMacNode.uiFlag = MAC_NODE_FLAG_STATIC;
    stData.uiIP = uiIp;

    if (BS_OK != MACTBL_Add(pstInstance->hSignedMacTbl, &stMacNode, &stData, MAC_MODE_SET))
    {
        IPPOOL_FreeIP(pstInstance->hIpPool, ntohl(stData.uiIP));
        return BS_ERR;
    }

    return BS_OK;
}


BS_STATUS DHCPS_UnSignIP
(
    IN DHCPS_HANDLE hDhcpInstance,
    IN MAC_ADDR_S *pstMacAddr
)
{
    _DHCPS_INSTANCE_S *pstInstance = hDhcpInstance;
    MAC_NODE_S stMacNode;
    _DHCPS_DATA_S stData;

    if (NULL == pstInstance)
    {
        return BS_NULL_PARA;
    }

    stMacNode.stMac = *pstMacAddr;
    
    if (BS_OK == MACTBL_Find(pstInstance->hSignedMacTbl, &stMacNode, &stData))
    {
        IPPOOL_FreeIP(pstInstance->hIpPool, ntohl(stData.uiIP));
        MACTBL_Del(pstInstance->hSignedMacTbl, &stMacNode);
    }

    return BS_OK;
}

BS_STATUS DHCPS_PktInput
(
    IN DHCPS_HANDLE hDhcpInstance,
    IN MBUF_S *pstMbuf,  
    IN VOID *pPktContext
)
{
    BS_STATUS eRet;
    DHCP_HEAD_S *pstDhcpHead;
    UINT ulOptLen;
    INT iDhcpMsgType;
    UINT ulRequestIp;  
    _DHCPS_INSTANCE_S *pstInstance = hDhcpInstance;

    if (MBUF_TOTAL_DATA_LEN(pstMbuf) < sizeof(DHCP_HEAD_S))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    pstDhcpHead = MBUF_MTOD(pstMbuf);

    ulOptLen = MBUF_TOTAL_DATA_LEN(pstMbuf) - sizeof(DHCP_HEAD_S);

    iDhcpMsgType = DHCP_GetDHCPMsgType (pstDhcpHead, ulOptLen);
    if (iDhcpMsgType < 0)
    {
        MBUF_Free (pstMbuf);
        return(BS_ERR);
    }

    
    if (pstDhcpHead->ucOp != DHCP_OP_REQUEST)
    {
        MBUF_Free (pstMbuf);
        return(BS_ERR);
    }

    ulRequestIp = DHCP_GetRequestIpByDhcpRequest (pstDhcpHead, ulOptLen);

    eRet = BS_ERR;
    if (iDhcpMsgType == DHCP_DISCOVER)
    {
        eRet = dhcps_SendDhcpOffer(pstInstance, pstDhcpHead, ulRequestIp, pPktContext);
    }
    else if (iDhcpMsgType == DHCP_REQUEST)
    {
        eRet = dhcps_SendDhcpAck(pstInstance, pstDhcpHead, ulRequestIp, pPktContext);
    }
    else if (iDhcpMsgType == DHCP_DECLINE)
    {
        eRet = dhcps_DealDecline(pstInstance, pstDhcpHead, ulRequestIp, pPktContext);
    }

    MBUF_Free(pstMbuf);

    return eRet;
}

BS_STATUS DHCPS_DelClient(IN DHCPS_HANDLE hDhcpInstance, IN MAC_ADDR_S *pstClientMac)
{
    _DHCPS_INSTANCE_S *pstInstance = hDhcpInstance;
    MAC_NODE_S stMacNode;

    stMacNode.stMac = *pstClientMac;

    MACTBL_Del(pstInstance->hMacTbl, &stMacNode);

    return BS_OK;
}


UINT DHCPS_GetServerIP(IN DHCPS_HANDLE hDhcpInstance)
{
    _DHCPS_INSTANCE_S *pstInstance = hDhcpInstance;

    return dhcps_GetServerIP(pstInstance);
}


UINT DHCPS_GetMask(IN DHCPS_HANDLE hDhcpInstance)
{
    _DHCPS_INSTANCE_S *pstInstance = hDhcpInstance;

    return dhcps_GetMask(pstInstance);
}

static VOID dhcps_WalkClients
(
    IN MACTBL_HANDLE hMacTblId,
    IN MAC_NODE_S *pstNode,
    IN VOID *pData,
    IN VOID *pUserHandle
)
{
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_DHCPS_WALK_CLIENTS_FUNC pfFunc = pstUserHandle->ahUserHandle[0];
    _DHCPS_DATA_S *pstData = pData;

    pfFunc(&pstNode->stMac, pstData->uiIP, pstUserHandle->ahUserHandle[1]);
}

VOID DHCPS_WalkClients
(
    IN DHCPS_HANDLE hDhcpInstance,
    IN PF_DHCPS_WALK_CLIENTS_FUNC pfFunc,
    IN VOID *pUserHandle
)
{
    _DHCPS_INSTANCE_S *pstInstance = hDhcpInstance;
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = pUserHandle;
    
    MACTBL_Walk(pstInstance->hMacTbl, dhcps_WalkClients, &stUserHandle);
}

VOID DHCPS_TickStep(IN DHCPS_HANDLE hDhcpInstance)
{
    _DHCPS_INSTANCE_S *pstInstance = hDhcpInstance;

    MACTBL_TickStep(pstInstance->hMacTbl);

    pstInstance->uiDeclineTick ++;
    if (pstInstance->uiDeclineTick >= _DHCPS_DECLINE_MAX_TICK)
    {
        pstInstance->uiDeclineTick = 0;
        IPPOOL_PermitAll(pstInstance->hIpPool);
    }
}

