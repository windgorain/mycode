/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-3-24
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/mac_table.h"
#include "utl/ulm_utl.h"
#include "utl/txt_utl.h"
#include "utl/rand_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_master.h"
#include "../inc/vnetc_mac_tbl.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_auth.h"
#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_caller.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_context.h"
#include "../inc/vnetc_alias.h"
#include "../inc/vnetc_enter_domain.h"
#include "../inc/vnetc_vnic_phy.h"
#include "../inc/vnetc_logout.h"
#include "../inc/vnetc_user_status.h"
#include "../inc/vnetc_fsm.h"

static BS_STATUS vnetc_auth_SendRequest()
{
    CHAR szInfo[256];

    snprintf(szInfo, sizeof(szInfo), "Protocol=Auth,UserName=%s,Password=%s",
        VNETC_GetUserName(), VNETC_GetUserPasswd());

    return VNETC_Protocol_SendData(VNETC_TP_GetC2STP(),szInfo, strlen(szInfo) + 1);
}

BS_STATUS VNETC_AUTH_StartAuth()
{
    BS_STATUS eRet;

    VNETC_User_SetStatus(VNET_USER_STATUS_AUTH_ING, VNET_USER_REASON_NONE);

    eRet = vnetc_auth_SendRequest();

    if (eRet != BS_OK)
    {
        VNETC_User_SetStatus(VNET_USER_STATUS_OFFLINE, VNET_USER_REASON_NO_RESOURCE);
    }

    return eRet;
}

BS_STATUS VNETC_AUTH_Init()
{
    return BS_OK;
}

BS_STATUS VNETC_Auth_Input(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    MAC_NODE_S stMacNode;
    VNETC_MAC_USER_DATA_S stUserData;
    CHAR *pcResult;
    CHAR *pcServerMAC;
    CHAR *pcNodeID;
    CHAR *pcCookie;
    CHAR *pcCookieString;
    CHAR *pcReason;
    UINT uiCookie;
    UINT uiNodeID;
    UINT uiReason = 0;

    pcResult = MIME_GetKeyValue(hMime, "Result");
    if (NULL == pcResult)
    {
        return BS_ERR;
    }

    pcServerMAC = MIME_GetKeyValue(hMime, "ServerMac");
    pcNodeID = MIME_GetKeyValue(hMime, "NodeID");
    pcCookie = MIME_GetKeyValue(hMime, "Cookie");
    pcCookieString = MIME_GetKeyValue(hMime, "CookieString");

    if ((strcmp(pcResult, "OK") == 0)
        && (NULL != pcServerMAC)
        && (NULL != pcNodeID)
        && (NULL != pcCookie))
    {
        VNETC_User_SetStatus(VNET_USER_STATUS_ONLINE, VNET_USER_REASON_NONE);
        TXT_Atoui(pcNodeID, &uiNodeID);
        STRING_2_MAC_ADDR(pcServerMAC, stMacNode.stMac.aucMac);
        stMacNode.uiFlag = MAC_NODE_FLAG_STATIC;
        stUserData.uiNodeID = VNETC_Context_GetSrcNID(pstPacketInfo->pstMBuf);
        VNETC_MACTBL_Add(&stMacNode, &stUserData, MAC_MODE_SET);
        VNETC_NODE_SetSelf(uiNodeID);
        TXT_XAtoui(pcCookie, &uiCookie);
        uiCookie = htonl(uiCookie);
        VNETC_NODE_SetSelfCookie(uiCookie);
        VNETC_NODE_SetSelfCookieString(pcCookieString);
        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_AUTH_OK);
    }
    else
    {
        pcReason = MIME_GetKeyValue(hMime, "ErrorCode");
        if (NULL != pcReason)
        {
            TXT_Atoui(pcReason, &uiReason);
        }
        VNETC_User_SetStatus(VNET_USER_STATUS_OFFLINE, uiReason);
        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_AUTH_FAILED);
    }

	return BS_OK;
}

VOID VNETC_AUTH_Logout()
{
    VNET_VNIC_PHY_SetMediaStatus(0);
    VNETC_Logout_SendLogoutMsg();
    VNETC_User_SetStatus(VNET_USER_STATUS_OFFLINE, VNET_USER_REASON_NONE);
}



