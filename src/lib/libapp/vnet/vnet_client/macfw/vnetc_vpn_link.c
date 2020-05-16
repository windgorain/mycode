#include "bs.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_vpn_link.h"

#include "../inc/vnetc_mac_fw.h"
#include "../inc/vnetc_vpn_link.h"
#include "../inc/vnetc_context.h"
#include "../inc/vnetc_master.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_ses.h"

static BS_STATUS vnetc_vpn_link_SendMbuf(IN UINT uiSesID, IN MBUF_S *pstMbuf)
{
    if (uiSesID == 0)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }
    
    if (BS_OK != VNET_VpnLink_Encrypt(pstMbuf, TRUE))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return VNETC_SES_SendPkt(uiSesID, pstMbuf);
}

BS_STATUS VNETC_VPN_LINK_Output (IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType)
{
    VNET_VPN_LINK_HEAD_S *pstLinkHead;
    UINT uiSesID;

    if (BS_OK != MBUF_Prepend (pstMbuf, sizeof(VNET_VPN_LINK_HEAD_S)))
    {
        MBUF_Free (pstMbuf);
        return(BS_ERR);
    }

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof(VNET_VPN_LINK_HEAD_S)))
    {
        MBUF_Free (pstMbuf);
        return(BS_ERR);
    }

    pstLinkHead = MBUF_MTOD (pstMbuf);

    memset(pstLinkHead, 0, sizeof(VNET_VPN_LINK_HEAD_S));

    pstLinkHead->usVer = htons(_VNET_VPN_LINK_VER);
    pstLinkHead->usProto = usProtoType;

    uiSesID = VNETC_Context_GetSendSesID(pstMbuf);

    return vnetc_vpn_link_SendMbuf (uiSesID, pstMbuf);
}


BS_STATUS VNETC_VPN_LINK_Input(IN MBUF_S *pstMbuf)
{
    if (BS_OK != VNET_VpnLink_Encrypt(pstMbuf, FALSE))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    MBUF_CutHead (pstMbuf, sizeof (VNET_VPN_LINK_HEAD_S));

    return VNETC_NODE_PktInput(pstMbuf);
}


