/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _IP_MBUF_H
#define _IP_MBUF_H
#include "utl/mbuf_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

IP_HEAD_S * IP_GetIPHeaderByMbuf(IN MBUF_S *pstMbuf, IN NET_PKT_TYPE_E enPktType);
BS_STATUS IP_ValidPkt(IN MBUF_S *pstMbuf);
/* 给Mbuf加上IP头 */
BS_STATUS IP_BuildIPHeader
(
    IN MBUF_S *pstMbuf,
    IN UINT uiDstIp/* 网络序 */,
    IN UINT uiSrcIp/* 网络序 */,
    IN UCHAR ucProto,
    IN USHORT usIdentification /* 网络序 */
);

#ifdef __cplusplus
}
#endif
#endif //IP_MBUF_H_
