/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-6-18
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_NODE_H_
#define __VNETC_NODE_H_

#include "utl/hash_utl.h"
#include "utl/mbuf_utl.h"
#include "utl/vclock_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define VNETC_NODE_FLAG_STATIC 0x1  /* 静态位 */
#define VNETC_NODE_FLAG_INNER  0x2  /* 内部NODE. VNIC和PCAP类型的Node是内部Node */


enum
{
    VNETC_NODE_DIRECT_INIT = 0,   /* 未开始直连检测 */
    VNETC_NODE_DIRECT_DETECTING,  /* 直连检测中 */
    VNETC_NODE_DIRECT_OK,         /* 直连检测成功 */
    VNETC_NODE_DIRECT_FAILED      /* 直连检测失败 */
};

typedef struct
{
    HASH_NODE_S stNode;
    UINT uiNID;  /* 主机序 */
    UINT uiIfIndex;
    UINT uiFlag;
    UCHAR ucReserved;
    UCHAR ucDirectStatus;   /* 直连状态 */
    UINT uiSesID;
    UINT uiPRI; /* 数值越小,优先级越高 */
    UINT uiAge;
    VCLOCK_HANDLE hOldTimer;
}VNETC_NID_S;

BS_STATUS VNETC_NODE_Init();
BS_STATUS VNETC_NODE_Set
(
    IN UINT uiNID,
    IN UINT uiIfIndex,
    IN UINT uiSesID,
    IN UINT uiFlag
);
BS_STATUS VNETC_NODE_Learn
(
    IN UINT uiNID,
    IN UINT uiIfIndex,
    IN UINT uiSesID
);
VOID VNETC_NODE_SesClosed(IN UINT uiNID, IN UINT uiSesID);
VOID VNETC_NODE_DirectDetectFailed(IN UINT uiNID);
VOID VNETC_NODE_Del(IN UINT uiNID);
VOID VNETC_NODE_SetSesInfo(IN UINT uiNID, IN UINT uiIfIndex, IN UINT uiSesID);
VNETC_NID_S * VNETC_NODE_GetNode(IN UINT uiNID);
VOID VNETC_NODE_SetSelf(IN UINT uiSelfNodeID);
VOID VNETC_NODE_SetSelfCookie(IN UINT uiCookie);
VOID VNETC_NODE_SetSelfCookieString(IN CHAR *pcCookieString);
CHAR * VNETC_NODE_GetSelfCookieString();
UINT VNETC_NODE_Self();
BS_STATUS VNETC_NODE_PktOutput(IN UINT uiDstNID, IN MBUF_S *pstMbuf, IN USHORT usProto);
BS_STATUS VNETC_NODE_BroadCast(IN MBUF_S *pstMbuf, IN USHORT usProto);
BS_STATUS VNETC_NODE_PktInput(IN MBUF_S *pstMbuf);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_NODE_H_*/


