/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-18
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_RPCC_UDP

#include "bs.h"
    
#include "utl/socket_utl.h"
#include "utl/rpc_utl.h"
    
#include "rpc_inner.h"
#include "rpcc_type.h"
#include "rpc_udp.h"

static RPC_MSG_S * _RPCC_UDP_Send(IN HANDLE hHandle, IN RPC_MSG_S *pstRpcMsg);


RPCC_FUNS_TBL_S g_stRpcUdpFuncTbl = 
{
    _RPCC_UDP_Send
};


static inline INT _RPCC_UDP_CreateSocket(IN UINT ulIp, IN USHORT usPort)
{
    INT iSocketId;
    
    if ((iSocketId = Socket_Create(AF_INET, SOCK_DGRAM)) < 0)
    {
        return 0;
    }

    (VOID) Socket_Connect(iSocketId, ulIp, usPort);

    return iSocketId;
}

static inline BS_STATUS _RPCC_UDP_SendMsg(IN UINT ulFileId, IN RPC_MSG_S *pstRpcMsg)
{
    UCHAR *pucData;
    INT lSize;

    pucData = RPC_CreateDataByMsg(pstRpcMsg);

    RPC_FreeMsg(pstRpcMsg);

    if (NULL == pucData)
    {
        RETURN(BS_ERR);
    }

    lSize = Socket_Write(ulFileId, pucData, RPC_GetTotalSizeOfData(pucData), 0);
    RPC_FreeData(pucData);

    if (lSize < 0)
    {
        BS_WARNNING(("Socket write error, ret=%d", lSize));
        RETURN(BS_ERR);
    }

    return BS_OK;
}

static inline RPC_MSG_S * _RPCC_UDP_RecvMsg(IN UINT ulFileId)
{
    UCHAR aucData[_RPCC_UDP_MAX_DATA_LEN];
	BS_STATUS eRet;
	UINT uiReadLen;

    eRet = Socket_Read2(ulFileId, aucData, _RPCC_UDP_MAX_DATA_LEN, &uiReadLen, 0);
    if (eRet != BS_OK)
    {
        return NULL;
    }

    return RPC_CreateMsgByData(aucData);
}

HANDLE RPCC_UDP_Create(IN UINT ulIp/* 主机序 */, IN USHORT usPort/* 主机序*/)
{
    RPCC_HANDLE_S *pstHandle;
    UINT ulFileId;

    pstHandle = MEM_ZMalloc(sizeof(RPCC_HANDLE_S));
    pstHandle->pstFuncTbl = &g_stRpcUdpFuncTbl;

    ulFileId = _RPCC_UDP_CreateSocket(ulIp, usPort);
    if (0 == ulFileId)
    {
        MEM_Free(pstHandle);
        return 0;
    }

    pstHandle->ulFileId = ulFileId;

    return pstHandle;
}

static RPC_MSG_S * _RPCC_UDP_Send(IN HANDLE hRpcHandle, IN RPC_MSG_S *pstRpcMsg)
{
    RPCC_HANDLE_S *pstHandle = (RPCC_HANDLE_S *)hRpcHandle;
    RPC_MSG_S *pstMsg;

    if (BS_OK != _RPCC_UDP_SendMsg(pstHandle->ulFileId, pstRpcMsg))
    {
        BS_WARNNING(("Rpcc udp can't send msg!"));
        return NULL;
    }

    pstMsg = _RPCC_UDP_RecvMsg(pstHandle->ulFileId);
    if (NULL == pstMsg)
    {
        BS_WARNNING(("Rpcc udp can't recv msg!"));
        return NULL;
    }

    return pstMsg;
}

