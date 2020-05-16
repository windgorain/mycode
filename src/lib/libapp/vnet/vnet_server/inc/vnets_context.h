#ifndef __VNETS_CONTEXT_H_
#define __VNETS_CONTEXT_H_

#include "../inc/vnets_phy.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define VNETS_CONTEXT_FLAG_ONLINE 0x1 /* 在线标志 */

typedef struct
{
    UINT           ulRecvSesId;    /* 收取报文的会话ID */
    UINT           ulSendSesId;    /* 发送报文的会话ID */
    UINT           uiDstNodeID;
    UINT           uiSrcNodeID;
    UINT           uiFlag;
    MAC_ADDR_S     stSrcMac;       /* 接收到的报文的源MAC */
}VNET_LINK_CONTEXT_S;

typedef struct
{
    VNETS_PHY_CONTEXT_S stPhyContext;
    VNET_LINK_CONTEXT_S stContext;
}VNETS_CONTEXT_S;

VNETS_CONTEXT_S * VNETS_Context_AddContextToMbuf(IN MBUF_S *pstMbuf);
BS_STATUS VNETS_Context_SetContextToMbuf(IN MBUF_S *pstMbuf, IN VNETS_CONTEXT_S *pstContext);
VNETS_CONTEXT_S * VNETS_Context_GetContext(IN MBUF_S *pstMbuf);


VOID VNETS_Context_SetRecvSesID(IN MBUF_S *pstMbuf, IN UINT uiSesID);
UINT VNETS_Context_GetRecvSesID(IN MBUF_S *pstMbuf);
VOID VNETS_Context_SetSendSesID(IN MBUF_S *pstMbuf, IN UINT uiSesID);
UINT VNETS_Context_GetSendSesID(IN MBUF_S *pstMbuf);
VOID VNETS_Context_SetDstNodeID(IN MBUF_S *pstMbuf, IN UINT uiNodeID);
UINT VNETS_Context_GetDstNodeID(IN MBUF_S *pstMbuf);
VOID VNETS_Context_SetSrcNodeID(IN MBUF_S *pstMbuf, IN UINT uiNodeID);
UINT VNETS_Context_GetSrcNodeID(IN MBUF_S *pstMbuf);
UINT VNETS_Context_GetFlag(IN MBUF_S *pstMbuf);
VOID VNETS_Context_SetFlag(IN MBUF_S *pstMbuf, IN UINT uiFlag);
BOOL_T VNETS_Context_CheckFlag(IN MBUF_S *pstMbuf, IN UINT uiFlagBit);
VOID VNETS_Context_SetFlagBit(IN MBUF_S *pstMbuf, IN UINT uiFlagBit);
VOID VNETS_Context_ClrFlagBit(IN MBUF_S *pstMbuf, IN UINT uiFlagBit);
VOID VNETS_Context_SetPhyType(IN MBUF_S *pstMbuf, IN VNETS_PHY_TYPE_E enType);
VNETS_PHY_TYPE_E VNETS_Context_GetPhyType(IN MBUF_S *pstMbuf);
VNETS_PHY_CONTEXT_S * VNETS_Context_GetPhyContext(IN MBUF_S *pstMbuf);

MBUF_S * VNETS_Context_CreateMbufByCopyBuf
(
    IN UINT ulReserveHeadSpace,
    IN UCHAR *pucBuf,
    IN UINT ulLen
);
MBUF_S * VNETS_Context_CreateMbufByCluster
(
    IN MBUF_CLUSTER_S *pstCluster,
    IN UINT ulReserveHeadSpace,
    IN UINT ulLen
);
#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_CONTEXT_H_*/

