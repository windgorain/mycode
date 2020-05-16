/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/mac_table.h"
#include "utl/ippool_utl.h"
#include "utl/udp_utl.h"
#include "utl/udp_mbuf.h"
#include "utl/dhcp_utl.h"
#include "utl/dhcp_mbuf.h"

BOOL_T DHCP_IsDhcpRequestEthPacketByMbuf(IN MBUF_S *pstMbuf, IN NET_PKT_TYPE_E enPktType)
{
    UDP_HEAD_S *pstUdpHeader;

    pstUdpHeader = UDP_GetUDPHeaderByMbuf(pstMbuf, enPktType);
    if (NULL == pstUdpHeader)
    {
        return FALSE;
    }

    if (pstUdpHeader->usDstPort != htons(DHCP_DFT_SERVER_PORT))
    {
        return FALSE;
    }

    return TRUE;
}

