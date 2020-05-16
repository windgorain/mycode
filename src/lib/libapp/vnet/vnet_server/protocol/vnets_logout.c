#include "bs.h"

#include "utl/mbuf_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnets_domain.h"
#include "../inc/vnets_phy.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_protocol.h"

BS_STATUS VNETS_Logout_Recv(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    UINT uiSrcNID;
    UINT uiSesID;

    uiSrcNID = VNETS_Context_GetSrcNodeID(pstPacketInfo->pstMBuf);

    if (uiSrcNID != 0)
    {
        uiSesID = VNETS_NODE_GetSesID(uiSrcNID);
        if (uiSesID != 0)
        {
            VNETS_SES_Close(uiSesID);
        }
    }

	return BS_OK;
}


