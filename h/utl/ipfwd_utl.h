/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-10
* Description: 
* History:     
******************************************************************************/

#ifndef __IPFWD_UTL_H_
#define __IPFWD_UTL_H_

#include "utl/fib_utl.h"
#include "utl/ipfwd_service.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* Debug 选项 */
#define IP_FWD_DBG_PACKET 0x1


typedef HANDLE IPFWD_HANDLE;

typedef BS_STATUS (*PF_IPFWD_LINK_OUTPUT)(IN UINT uiIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 网络序 */);
typedef BS_STATUS (*PF_IPFWD_DeliverUp)(IN IP_HEAD_S *pstIpHead, IN MBUF_S *pstMbuf);

typedef enum
{
    IPFWD_HOW_DEBUG_SET = 0,    /* 设置DebugFalg */
    IPFWD_HOW_DEBUG_ADD,        /* 添加DebugFalg */
    IPFWD_HOW_DEBUG_DEL         /* 删除DebugFalg */
}IPFWD_HOW_DEBUG_E;


IPFWD_HANDLE IPFWD_Create
(
    IN FIB_HANDLE hFib,
    IN IPFWD_SERVICE_HANDLE hIpFwdService,
    IN PF_IPFWD_LINK_OUTPUT pfLinkOutput,
    IN PF_IPFWD_DeliverUp pfDeliverUp
);

VOID IPFWD_Destory(IN IPFWD_HANDLE hIpFwd);

/* 相比于OutPut, 不再填写IP头的东西,认为上面已经填写好了 */
BS_STATUS IPFWD_Send
(
    IN IPFWD_HANDLE hIpFwd,
    IN MBUF_S *pstMbuf /* 带IP头 */
);

BS_STATUS IPFWD_Output
(
    IN IPFWD_HANDLE hIpFwd,
    IN MBUF_S *pstMbuf, /* 不带IP头 */
    IN UINT uiDstIp,    /* 网络序 */
    IN UINT uiSrcIp,    /* 网络序 */
    IN UCHAR ucProto
);

BS_STATUS IPFWD_Input (IN IPFWD_HANDLE hIpFwd, IN MBUF_S *pstMbuf);

VOID IPFWD_SetDebug(IN IPFWD_HANDLE hIpFwd, IN IPFWD_HOW_DEBUG_E eHow, IN UINT uiDbgFlag);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IPFWD_UTL_H_*/


