/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-18
* Description: 
* History:     
******************************************************************************/

#ifndef __RPCC_TYPE_H_
#define __RPCC_TYPE_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef RPC_MSG_S* (*PF_RPCC_SEND_FUNC)(IN HANDLE hRpccHandle, IN RPC_MSG_S *pstRpcMsg);

typedef struct
{
    PF_RPCC_SEND_FUNC pfSendFunc;
}RPCC_FUNS_TBL_S;

typedef struct
{
    RPCC_FUNS_TBL_S *pstFuncTbl;
    UINT ulFileId;
}RPCC_HANDLE_S;



#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__RPCC_TYPE_H_*/


