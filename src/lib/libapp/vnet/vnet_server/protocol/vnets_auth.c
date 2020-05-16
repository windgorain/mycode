
#include "bs.h"

#include "utl/mac_table.h"
#include "utl/mbuf_utl.h"
#include "utl/rand_utl.h"
#include "utl/kf_utl.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_ifnet.h"
#include "../../inc/vnet_user_status.h"

#include "../inc/vnets_conf.h"
#include "../inc/vnets_protocol.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_dc.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_logout.h"
#include "../inc/vnets_enter_domain.h"
#include "../inc/vnets_dns_quest_list.h"

#define _VNET_SERVER_AUTH_DBG_FLAG_PACKET    0x1

static UINT g_ulVnetServerAuthDbgFlag = 0;

static BS_STATUS vnets_auth_SendReply
(
    IN UINT uiTpID,
    IN VNET_USER_REASON_E eReason,
    IN UINT uiNodeID
)
{
    MAC_ADDR_S *pstMac;
    CHAR szInfo[512];
    UINT uiCookie;
    CHAR szCookieString[VNETS_NODE_COOKIE_STRING_LEN + 1];
    
    if (eReason == VNET_USER_REASON_NONE)
    {
        pstMac = VNETS_GetHostMac();
        uiCookie = VNETS_NODE_GetCookie(uiNodeID);
        uiCookie = ntohl(uiCookie);
        VNETS_NODE_GetCookieString(uiNodeID, szCookieString);

        snprintf(szInfo, sizeof(szInfo),
            "Protocol=Auth,Result=OK,NodeID=%d,Cookie=%08x,CookieString=%s,ServerMac=%pM",
            uiNodeID, uiCookie, szCookieString, pstMac);
    }
    else
    {
        snprintf(szInfo, sizeof(szInfo), "Protocol=Auth,Result=Failed,ErrorCode=%d", eReason);
    }

    return VNETS_Protocol_SendData(uiTpID, szInfo, strlen(szInfo) + 1);
}

static BS_STATUS vnets_auth_RecvAuthRequest(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    CHAR *pszUserName;
    CHAR *pszPasswd;
    BOOL_T bAuthSuccess = TRUE;
    BOOL_T bSuccess;
    VNET_USER_REASON_E eReason = VNET_USER_REASON_NONE;
    UINT uiNodeID = VNETS_NODE_INVALID_ID;
    CHAR szPassword[VNET_CONF_MAX_USER_PASSWD_LEN + 1];

    pszUserName   = MIME_GetKeyValue(hMime, "UserName");
    pszPasswd     = MIME_GetKeyValue(hMime, "Password");

    PW_Base64Decrypt(pszPasswd, szPassword, sizeof(szPassword));

    /* 认证用户 */
    bSuccess = VNETS_DC_CheckUserPassword(pszUserName, szPassword);

    BS_DBG_OUTPUT(g_ulVnetServerAuthDbgFlag, _VNET_SERVER_AUTH_DBG_FLAG_PACKET, 
        ("VNETS-AUTH:User %s login %s.\r\n",
        pszUserName, bSuccess == FALSE ? "failed" : "success"));

    if (bSuccess == FALSE) /* 认证失败 */
    {
        eReason = VNET_USER_REASON_AUTH_FAILED;
    }
    else
    {
        uiNodeID = VNETS_NODE_AddNode(VNETS_Context_GetRecvSesID(pstPacketInfo->pstMBuf));
        if (VNETS_NODE_INVALID_ID == uiNodeID)
        {
            eReason = VNET_USER_REASON_NO_RESOURCE;
        }
        VNETS_NODE_SetUserName(uiNodeID, pszUserName);
        VNETS_NODE_SetTPID(uiNodeID, pstPacketInfo->uiTpID);
    }

    vnets_auth_SendReply(pstPacketInfo->uiTpID, eReason, uiNodeID);

    return BS_OK;
}

BS_STATUS VNETS_Auth_Input(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    return vnets_auth_RecvAuthRequest(hMime, pstPacketInfo);
}


/* debug auth packet */
PLUG_API VOID VNETS_AUTH_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetServerAuthDbgFlag |= _VNET_SERVER_AUTH_DBG_FLAG_PACKET;
}

/* no debug auth packet */
PLUG_API VOID VNETS_AUTH_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetServerAuthDbgFlag &= ~_VNET_SERVER_AUTH_DBG_FLAG_PACKET;
}


