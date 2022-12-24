/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _UDP_MBUF_H
#define _UDP_MBUF_H

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

UDP_HEAD_S * UDP_GetUDPHeaderByMbuf(IN MBUF_S *pstMbuf, IN NET_PKT_TYPE_E enPktType);

/* 给MBUF加上UDP头 */
BS_STATUS UDP_BuilderHeader
(
    IN MBUF_S *pstMbuf,
    IN USHORT usSrcPort/* 网络序 */,
    IN USHORT usDstPort/* 网络序 */,
    IN UINT uiSrcIp/* 网络序 */,
    IN UINT uiDstIp/* 网络序 */
);

#ifdef __cplusplus
}
#endif
#endif //UDP_MBUF_H_
