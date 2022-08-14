/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-8-14
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/fib_utl.h"
#include "utl/socket_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "comp/comp_if.h"
#include "comp/comp_wan.h"

#include "../h/wan_ifnet.h"
#include "../h/wan_ip_addr.h"

/* ip address x.x.x.x mask */
PLUG_API BS_STATUS WAN_IPAddrCmd_IpAddress
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    UINT uiIP;
    UINT uiPrefix;
    UINT uiMask;
    IF_INDEX ifIndex;
    BS_STATUS eRet;
    WAN_IP_ADDR_INFO_S stAddrInfo = {0};

    if (! Socket_IsIPv4(argv[2]))
    {
        EXEC_OutString("Invalid ip address.\r\n");
        return BS_ERR;
    }

    uiIP = Socket_NameToIpNet(argv[2]);
    if (BS_OK != TXT_Atoui(argv[3], &uiPrefix))
    {
        EXEC_OutString("Can't get mask.\r\n");
        return BS_ERR;
    }

    ifIndex = WAN_IF_GetIfIndexByEnv(pEnv);
    if (0 == ifIndex)
    {
        EXEC_OutString("Can't get ifindex.\r\n");
        return BS_ERR;
    }

    uiMask = PREFIX_2_MASK(uiPrefix);
    uiMask = htonl(uiMask);

    WanIPAddr_SetMode(ifIndex, WAN_IP_ADDR_MODE_STATIC);

    stAddrInfo.uiIfIndex = ifIndex;
    stAddrInfo.uiIP = uiIP;
    stAddrInfo.uiMask = uiMask;
    eRet = WanIPAddr_AddIp(&stAddrInfo);
    if (BS_CONFLICT == eRet)
    {
        EXEC_OutString("Ip address conflict.\r\n");
        return BS_ERR;
    }

    if (BS_OK != eRet)
    {
        EXEC_OutString("Can't add ip address.\r\n");
        return BS_ERR;
    }

    return BS_OK;
}

/* no ip address x.x.x.x */
PLUG_API BS_STATUS WAN_IPAddrCmd_NoIpAddress
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    UINT uiIP;
    IF_INDEX ifIndex;

    if (! Socket_IsIPv4(argv[3]))
    {
        EXEC_OutString("Invalid ip address.\r\n");
        return BS_ERR;
    }

    uiIP = Socket_NameToIpNet(argv[3]);

    ifIndex = WAN_IF_GetIfIndexByEnv(pEnv);
    if (0 == ifIndex)
    {
        EXEC_OutString("Can't get ifindex.\r\n");
        return BS_ERR;
    }

    WanIPAddr_SetMode(ifIndex, WAN_IP_ADDR_MODE_STATIC);

    WAN_IPAddr_DelIp(ifIndex, uiIP);

    return BS_OK;
}

static VOID _wan_ipaddrcmd_Save(IN IF_INDEX ifIndex, IN HANDLE hFile)
{
    WAN_IP_ADDR_S stIpAddrs;
    UINT uiPrefix;
    UINT uiMask;
    UINT i;

    if (BS_OK != WAN_IPAddr_GetInterfaceAllIp(ifIndex, &stIpAddrs))
    {
        return;
    }

    for (i=0; i<WAN_IP_ADDR_MAX_IF_IP_NUM; i++)
    {
        if (stIpAddrs.astIP[i].uiIP != 0)
        {
            uiMask = stIpAddrs.astIP[i].uiMask;
            uiMask = ntohl(uiMask);
            uiPrefix = MASK_2_PREFIX(uiMask);
            CMD_EXP_OutputCmd(hFile, "ip address %pI4 %d", &stIpAddrs.astIP[i].uiIP, uiPrefix);
        }
    }
}

BS_STATUS WAN_IpAddrCmd_Init()
{
    IFNET_RegSave(_wan_ipaddrcmd_Save);

    return BS_OK;
}


