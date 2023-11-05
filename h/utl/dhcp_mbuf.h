/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _DHCP_MBUF_H
#define _DHCP_MBUF_H

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

BOOL_T DHCP_IsDhcpRequestEthPacketByMbuf(IN MBUF_S *pstMbuf, IN NET_PKT_TYPE_E enPktType);

#ifdef __cplusplus
}
#endif
#endif 
