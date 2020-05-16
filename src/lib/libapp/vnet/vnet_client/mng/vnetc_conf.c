/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-5
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_CLIENTCONF
        
#include "bs.h"
    
#include "utl/cff_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/passwd_utl.h"
#include "utl/local_info.h"
#include "utl/dns_utl.h"
#include "utl/mbuf_utl.h"
#include "utl/exturl_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_master.h"
#include "../inc/vnetc_proto_start.h"
#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_auth.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_fsm.h"
#include "../inc/vnetc_ses_c2s.h"
#include "../inc/vnetc_udp_phy.h"
#include "../inc/vnetc_vnic_phy.h"
#include "../inc/vnetc_user_status.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_p_nodeinfo.h"

#define _VNETC_VER "Vnet version 1.0.1"

typedef struct
{
    VNETC_CONN_TYPE_E enType;
    CHAR *pcTypeName;
}VNETC_CONF_TYPE_MAP_S;

static UINT g_ulVnetSelectServerDomainId = 1;
static CHAR g_szVnetClientServerDomainName[VNET_CONF_MAX_DOMAIN_NAME_LEN + 1];
static CHAR g_szVnetClientServerUserName[VNET_CONF_MAX_USER_NAME_LEN + 1];
static CHAR g_szVnetClientServerPasswd[VNET_CONF_MAX_USER_ENC_PASSWD_LEN + 1];
static CHAR g_szVnetcServerAddress[VNETC_CONF_SERVER_ADDRESS_MAX_LEN + 1];
static CHAR g_szVnetcDescription[VNET_CONF_MAX_DES_LEN + 1];
static VNETC_CONF_TYPE_MAP_S g_astVnetcConfConnTypeMap[VNETC_CONN_TYPE_MAX] = 
{
    {VNETC_CONN_TYPE_UDP, "udp"},
};

static VNETC_CONN_TYPE_E g_enVnetcC2sConnType = VNETC_CONN_TYPE_MAX;
static BOOL_T g_bVnetcConfSupportDirect = TRUE;
static UINT g_ulVnetcC2SIfIndex = 0;
static MAC_ADDR_S g_stVnetcConfHostMac;

static VNETC_CONN_TYPE_E vnetc_conf_GetConnTypeByName(IN CHAR *pcTypeName)
{
    UINT i;

    for (i=0; i<sizeof(g_astVnetcConfConnTypeMap)/sizeof(VNETC_CONF_TYPE_MAP_S); i++)
    {
        if (strcmp(g_astVnetcConfConnTypeMap[i].pcTypeName, pcTypeName) == 0)
        {
            return g_astVnetcConfConnTypeMap[i].enType;
        }
    }

    return VNETC_CONN_TYPE_MAX;
}

/* 将udp://xxx.com:443解析出来 */
BS_STATUS VNETC_CONF_ParseServerAddress
(
    IN CHAR *pcServerAddress,
    OUT VNETC_CONN_TYPE_E *penConnType,
    OUT CHAR szServerName[VNETC_CONF_SERVER_ADDRESS_MAX_LEN + 1],
    OUT USHORT *pusPort  /* 主机序 */
)
{
    VNETC_CONN_TYPE_E enType;
	EXTURL_S stExtUrl;

    *penConnType = VNETC_CONN_TYPE_MAX;
    *pusPort = 0;

	if (BS_OK != EXTURL_Parse(pcServerAddress, &stExtUrl))
	{
		EXEC_OutString(" Can't parse address.\r\n");
        RETURN(BS_ERR);
	}

    enType = vnetc_conf_GetConnTypeByName(stExtUrl.szProtocol);
    if (enType == VNETC_CONN_TYPE_MAX)
    {
        EXEC_OutString(" Can't get type.\r\n");
        RETURN(BS_ERR);
    }

    if (enType == VNETC_CONN_TYPE_UDP)
    {
        if (stExtUrl.usPort == 0)
        {
            stExtUrl.usPort = VNET_CONF_DFT_UDP_PORT;
        }
    }

    if (NULL != szServerName)
    {
        TXT_Strlcpy(szServerName, stExtUrl.szAddress, VNETC_CONF_SERVER_ADDRESS_MAX_LEN + 1);
    }

    *penConnType = enType;
    *pusPort = stExtUrl.usPort;

    return BS_OK;
}

BS_STATUS VNETC_Start ()
{
    VNETC_CONN_TYPE_E enType;
    UINT uiIp = 0;
    USHORT usPort = 0;
    CHAR szServerName[VNETC_CONF_SERVER_ADDRESS_MAX_LEN + 1];
    VNETC_PHY_CONTEXT_S stPhyContext;

    if (BS_OK != VNETC_CONF_ParseServerAddress(g_szVnetcServerAddress,
        &enType, szServerName , &usPort))
    {
        RETURN(BS_NOT_INIT);
    }

    VNETC_SetC2SConnType(enType);

    switch (enType)
    {
        case VNETC_CONN_TYPE_UDP:
        {
            uiIp = Socket_NameToIpHost(szServerName);
            if (uiIp == 0)
            {
                EXEC_OutInfo(" Can not parse %s.\r\n", szServerName);
            }

            if (BS_OK != VNETC_UDP_PHY_Start())
            {
                RETURN(BS_ERR);
            }

            /* 创建C2S Ses */
            stPhyContext.uiIfIndex = VNETC_CONF_GetC2SIfIndex();
            stPhyContext.unPhyContext.stUdpPhyContext.uiPeerIp = htonl(uiIp);
            stPhyContext.unPhyContext.stUdpPhyContext.usPeerPort = htons(usPort);

            VNETC_SesC2S_SetPhyContext(&stPhyContext);

            break;
        }

        default:
            EXEC_OutInfo(" Can't support connection type %d.\r\n", enType);
            RETURN(BS_NOT_SUPPORT);
            break;
    }

    VNET_VNIC_CreateVnic();

    VNETC_FSM_EventHandle(VNETC_FSM_EVENT_START);

	return BS_OK;
}

BS_STATUS VNETC_Stop ()
{
    UINT uiTimes = 0;
    
    VNETC_Proto_Stop();

    /* 等待它停止 */
    do {
        Sleep(20);
        uiTimes ++;
    } while ((VNETC_User_GetStatus() != VNET_USER_STATUS_INIT) && (uiTimes < 500));
    
    return BS_OK;
}

BS_STATUS VNETC_SetServer(IN CHAR *pcServerAddress)
{
    VNETC_CONN_TYPE_E enType;
    USHORT usPort = 0;

    if (BS_OK != VNETC_CONF_ParseServerAddress(pcServerAddress, &enType, NULL, &usPort))
    {
        EXEC_OutString(" Server address error.\r\n");
        RETURN(BS_BAD_PARA);
    }

    if (strlen(pcServerAddress) >= sizeof(g_szVnetcServerAddress))
    {
        EXEC_OutString(" Server address is too long.\r\n");
        RETURN(BS_TOO_LONG);
    }
    
    TXT_Strlcpy(g_szVnetcServerAddress, pcServerAddress, sizeof(g_szVnetcServerAddress));

    return BS_OK;
}

CHAR * VNETC_GetServerAddress()
{
    return g_szVnetcServerAddress;
}

VOID VNETC_SetServerDomain(IN CHAR *pszDomainName)
{
    if (strlen(pszDomainName) > VNET_CONF_MAX_DOMAIN_NAME_LEN)
    {
        return;
    }

    TXT_StrCpy(g_szVnetClientServerDomainName, pszDomainName);
}

CHAR * VNETC_GetUserName()
{
    return g_szVnetClientServerUserName;
}

CHAR * VNETC_GetUserPasswd()
{
    return g_szVnetClientServerPasswd;
}

CHAR * VNETC_GetDomainName()
{
    return g_szVnetClientServerDomainName;
}

BS_STATUS VNETC_SetUserName(IN CHAR *pszUserName)
{
    if (NULL == pszUserName)
    {
        return BS_NULL_PARA;
    }

    if (strlen(pszUserName) > VNET_CONF_MAX_USER_NAME_LEN)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    TXT_Strlcpy(g_szVnetClientServerUserName, pszUserName, sizeof(g_szVnetClientServerUserName));

	return BS_OK;
}

BS_STATUS VNETC_SetUserPasswdSimple(IN CHAR *pszPasswd)
{
    if (NULL == pszPasswd)
    {
        return BS_NULL_PARA;
    }

    if (strlen(pszPasswd) > VNET_CONF_MAX_USER_PASSWD_LEN)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    PW_Base64Encrypt(pszPasswd, g_szVnetClientServerPasswd, sizeof(g_szVnetClientServerPasswd));

	return BS_OK;
}

BS_STATUS VNETC_SetUserPasswdCipher(IN CHAR *pszPasswd)
{
    if (NULL == pszPasswd)
    {
        return BS_NULL_PARA;
    }

    if (strlen(pszPasswd) > VNET_CONF_MAX_USER_ENC_PASSWD_LEN)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    TXT_Strlcpy(g_szVnetClientServerPasswd, pszPasswd, sizeof(g_szVnetClientServerPasswd));

	return BS_OK;
}

BS_STATUS VNETC_SetDomainName(IN CHAR *pszDomainName)
{
    if (strlen(pszDomainName) > VNET_CONF_MAX_DOMAIN_NAME_LEN)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    TXT_Strlcpy(g_szVnetClientServerDomainName, pszDomainName, sizeof(g_szVnetClientServerDomainName));

	return BS_OK;
}

BS_STATUS VNETC_SetDescription(IN CHAR *pcDescription)
{
    UINT uiNeedSize;

    if (NULL == pcDescription)
    {
        return BS_NULL_PARA;
    }

    uiNeedSize = strlen(pcDescription);
    if (uiNeedSize >= sizeof(g_szVnetcDescription))
    {
        return BS_OUT_OF_RANGE;
    }

    TXT_Strlcpy(g_szVnetcDescription, pcDescription, sizeof(g_szVnetcDescription));

    if (VNETC_User_GetStatus() == VNET_USER_STATUS_ONLINE)
    {
        VNETC_P_NodeInfo_SendInfo();
    }

    return BS_OK;
}

CHAR * VNETC_GetDescription()
{
    return g_szVnetcDescription;
}

CHAR * VNETC_GetVersion()
{
    return _VNETC_VER;
}

VOID VNETC_SetC2SConnType(IN VNETC_CONN_TYPE_E enType)
{
    g_enVnetcC2sConnType = enType;
}

VNETC_CONN_TYPE_E VNETC_GetC2SConnType()
{
    return g_enVnetcC2sConnType;
}

VOID VNETC_CONF_SetSupportDirect(IN BOOL_T bIsSupport)
{
    g_bVnetcConfSupportDirect = bIsSupport;
}

BOOL_T VNETC_CONF_IsSupportDirect()
{
    return g_bVnetcConfSupportDirect;
}

VOID VNETC_CONF_SetC2SIfIndex(IN UINT ulIfIndex)
{
    g_ulVnetcC2SIfIndex = ulIfIndex;
}

UINT VNETC_CONF_GetC2SIfIndex()
{
    return g_ulVnetcC2SIfIndex;
}

VOID VNETC_SetHostMac(IN MAC_ADDR_S *pstMac)
{
    g_stVnetcConfHostMac = *pstMac;
}

MAC_ADDR_S * VNETC_GetHostMac()
{
    return &g_stVnetcConfHostMac;
}


