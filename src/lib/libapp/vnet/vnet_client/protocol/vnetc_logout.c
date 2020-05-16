
#include "bs.h"

#include "utl/mbuf_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_auth.h"
#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_user_status.h"
#include "../inc/vnetc_vnic_phy.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_ses_c2s.h"

BS_STATUS VNETC_Logout_SendLogoutMsg()
{
    UINT uiIfIndex;
    UINT uiSesId;
    CHAR *pcString="Protocol:Logout";

    uiIfIndex = VNETC_CONF_GetC2SIfIndex();
    uiSesId = VNETC_SesC2S_GetSesId();

    if ((uiIfIndex == 0) || (uiSesId == 0))
    {
        return BS_OK;
    }

    return VNETC_Protocol_SendData(VNETC_TP_GetC2STP(), pcString, strlen(pcString) + 1);
}

BS_STATUS VNETC_Logout_PktInput(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    VNETC_User_SetStatus(VNET_USER_STATUS_OFFLINE, VNET_USER_REASON_NONE);
    VNET_VNIC_PHY_SetMediaStatus(0);

    return BS_OK;
}

