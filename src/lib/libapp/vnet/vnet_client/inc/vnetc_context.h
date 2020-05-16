#ifndef __VNETC_CONTEXT_H_
#define __VNETC_CONTEXT_H_

#include "utl/mbuf_utl.h"
#include "../inc/vnetc_phy.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    UINT   uiRecvSesId;    /* 收取报文的会话ID */
    UINT   uiSendSesId;
}VNETC_LINK_CONTEXT_S;

typedef struct
{
    UINT uiDstNID;
    UINT uiSrcNID;
}VNETC_NID_CONTEXT_S;

typedef struct
{
    VNETC_PHY_CONTEXT_S stPhyContext;
    VNETC_LINK_CONTEXT_S stContext;
    VNETC_NID_CONTEXT_S stNidContext;
}VNETC_CONTEXT_S;

VNETC_CONTEXT_S * VNETC_Context_AddContextToMbuf(IN MBUF_S *pstMbuf);


VOID VNETC_Context_SetRecvSesID(IN MBUF_S *pstMbuf, IN UINT uiSesID);
UINT VNETC_Context_GetRecvSesID(IN MBUF_S *pstMbuf);
VOID VNETC_Context_SetSendSesID(IN MBUF_S *pstMbuf, IN UINT uiSesID);
UINT VNETC_Context_GetSendSesID(IN MBUF_S *pstMbuf);
VOID VNETC_Context_SetSrcNID(IN MBUF_S *pstMbuf, IN UINT uiNodeID);
UINT VNETC_Context_GetSrcNID(IN MBUF_S *pstMbuf);
VOID VNETC_Context_SetDstNID(IN MBUF_S *pstMbuf, IN UINT uiNodeID);
UINT VNETC_Context_GetDstNID(IN MBUF_S *pstMbuf);


VNETC_PHY_CONTEXT_S * VNETC_Context_GetPhyContext(IN MBUF_S *pstMbuf);


MBUF_S * VNETC_Context_CreateMbufByCopyBuf
(
    IN UINT ulReserveHeadSpace,
    IN UCHAR *pucBuf,
    IN UINT ulLen
);
MBUF_S * VNETC_Context_CreateMbufByCluster
(
    IN MBUF_CLUSTER_S *pstCluster,
    IN UINT ulReserveHeadSpace,
    IN UINT ulLen
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_CONTEXT_H_*/

