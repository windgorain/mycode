#include "bs.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_vpn_link.h"

#include "../inc/vnets_node_input.h"
#include "../inc/vnets_vpn_link.h"
#include "../inc/vnets_phy.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_context.h"
#include "../inc/vnets_dns_quest_list.h"
#include "../inc/vnets_tp.h"
#include "../inc/vnets_worker.h"
#include "../inc/vnets_master.h"

BS_STATUS VNETS_VPN_LINK_Output (IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProto)
{
    UINT uiSesID;
    VNET_VPN_LINK_HEAD_S *pstLinkHead;

    uiSesID = VNETS_Context_GetSendSesID(pstMbuf);

    BS_DBGASSERT(uiSesID != 0);

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
    pstLinkHead->usProto = usProto;

    if (BS_OK != VNET_VpnLink_Encrypt(pstMbuf, TRUE))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return VNETS_SES_SendPkt(uiSesID, pstMbuf);
}


BS_STATUS VNETS_VPN_LINK_Input(IN MBUF_S *pstMbuf)
{
    if (BS_OK != VNET_VpnLink_Encrypt(pstMbuf, FALSE))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    MBUF_CutHead(pstMbuf, sizeof(VNET_VPN_LINK_HEAD_S));
    
    return VNETS_NodeInput(pstMbuf);
}


