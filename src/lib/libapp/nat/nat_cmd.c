/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "nat_main.h"
#include "nat_phy.h"
#include "nat_arp.h"

/* pcap private %INT */
PLUG_API BS_STATUS NAT_CMD_Pcap(IN UINT ulArgc, IN CHAR **argv)
{
	if (ulArgc < 3)
    {
        return BS_ERR;
    }

    return NAT_Phy_PrivatePcap(argv[2]);
}

/* pcap pub %INT */
PLUG_API BS_STATUS NAT_CMD_PubPcap(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 3)
    {
        return BS_ERR;
    }

    return NAT_Phy_PubPcap(argv[2]);
}

/* gateway-ip %STRING */
PLUG_API BS_STATUS NAT_CMD_SetGateWay(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        return BS_ERR;
    }

    NAT_Main_SetGateWay(argv[1]);

	return BS_OK;
}

/* pub-ip %STRING */
PLUG_API BS_STATUS NAT_CMD_SetPubIp(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        return BS_ERR;
    }

    NAT_Main_SetPubIp(argv[1]);

	return BS_OK;
}

/* pub-mac %STRING */
PLUG_API BS_STATUS NAT_CMD_SetPubMac(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        return BS_ERR;
    }

    NAT_Main_SetPubMac(argv[1]);

	return BS_OK;
}


/* start */
PLUG_API BS_STATUS NAT_CMD_Enable(IN UINT uiArgc, IN CHAR **argv)
{
    return NAT_Main_Start();
}

/*
    执行命令: show nat
*/
PLUG_API BS_STATUS NAT_CMD_Show(IN UINT ulArgc, IN CHAR ** argv)
{
    if (ulArgc < 2)
    {
        return BS_BAD_PARA;
    }

    NAT_Main_Show();

    return BS_OK;
}

/* save */
PLUG_API BS_STATUS NAT_CMD_Save(IN HANDLE hFile)
{
    NAT_Phy_Save(hFile);
    NAT_ARP_Save(hFile);
    NAT_Main_Save(hFile);

    return BS_OK;
}

VOID NAT_CMD_Init()
{
}

