/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/rpc_utl.h"

#include "rpc_inner.h"
#include "rpcc_type.h"

RPC_MSG_S * RPCC_Send(IN HANDLE hRpcHandle, IN RPC_MSG_S *pstRpcMsg)
{
    RPCC_HANDLE_S *pstHandle = (RPCC_HANDLE_S *)hRpcHandle;

    return pstHandle->pstFuncTbl->pfSendFunc(hRpcHandle, pstRpcMsg);
}

