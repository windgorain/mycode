
#include "bs.h"

#include "utl/mbuf_utl.h"

#include "utl/mac_table.h"
#include "utl/txt_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_mac_tbl.h"
#include "../inc/vnetc_mac_fw.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_fsm.h"
#include "../inc/vnetc_caller.h"
#include "../inc/vnetc_vnic_phy.h"
#include "../inc/vnetc_master.h"
#include "../inc/vnetc_p_nodeinfo.h"
#include "../inc/vnetc_p_addr_change.h"

static BS_STATUS vnetc_enterdomain_Reply(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    CHAR *pcResult;
    CHAR *pcIsPermitBroadcast;
    BOOL_T bIsPermitBroadcast = FALSE;

    pcResult = MIME_GetKeyValue(hMime, "Result");
    if (strcmp(pcResult, "OK") != 0)
    {
        return BS_ERR;
    }

    pcIsPermitBroadcast = MIME_GetKeyValue(hMime, "PermitBroadcast");
    if (pcIsPermitBroadcast != NULL)
    {
        if (strcmp(pcIsPermitBroadcast, "True") == 0)
        {
            bIsPermitBroadcast = TRUE;
        }
    }

    if ((pcResult != NULL) && (strcmp(pcResult, "OK") == 0))
    {
        if (bIsPermitBroadcast == TRUE)
        {
            VNETC_MAC_FW_SetPermitBoradcast(TRUE);
        }
        else
        {
            VNETC_MAC_FW_SetPermitBoradcast(FALSE);
        }

        VNET_VNIC_PHY_SetMediaStatus(1);

        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_ENTER_DOMAIN_OK);
    }
    else
    {
        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_ENTER_DOMAIN_FAILED);
    }

    return BS_OK;
}

BS_STATUS VNETC_EnterDomain_Start()
{
    CHAR *pcDomainName;
    CHAR szInfo[256];

    VNET_VNIC_PHY_SetMediaStatus(0);

    pcDomainName = VNETC_GetDomainName();
    if ((pcDomainName == NULL) || (pcDomainName[0] == '\0'))
    {
        snprintf(szInfo, sizeof(szInfo), "Protocol=EnterDomain,Domain=@%s", VNETC_GetUserName());
    }
    else
    {
        snprintf(szInfo, sizeof(szInfo), "Protocol=EnterDomain,Domain=%s", pcDomainName);
    }

    return VNETC_Protocol_SendData(VNETC_TP_GetC2STP(), szInfo, strlen(szInfo) + 1);
}

static VOID vnetc_enterdomain_Kicked()
{
    VNET_VNIC_PHY_SetMediaStatus(0);
    VNETC_FSM_EventHandle(VNETC_FSM_EVENT_KICK_OUT_DOMAIN);
}

static VOID vnetc_enterdomain_RebootDomain()
{
    VNET_VNIC_PHY_SetMediaStatus(0);
    VNETC_FSM_EventHandle(VNETC_FSM_EVENT_REBOOT_DOMAIN);
}

BS_STATUS VNETC_EnterDomain_Init()
{
    return BS_OK;
}

BS_STATUS VNETC_EnterDomain_Input(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    BS_STATUS eRet = BS_OK;
    CHAR *pcType;

    pcType = MIME_GetKeyValue(hMime, "Type");
    if (strcmp(pcType, "KickOut") == 0)
    {
        vnetc_enterdomain_Kicked();
        return BS_OK;
    }

    if (strcmp(pcType, "RebootDomain") == 0)
    {
        vnetc_enterdomain_RebootDomain();
        return BS_OK;
    }

    if (strcmp(pcType, "EnterDomainReply") == 0)
    {
        vnetc_enterdomain_Reply(hMime, pstPacketInfo);
        return BS_OK;
    }

    return BS_OK;
}

