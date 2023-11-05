
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/dns_utl.h"

#include "../../inc/vnet_conf.h"

#include "../../vnet/inc/vnet_mac_acl.h"

#include "../inc/vnets_udp_phy.h"
#include "../inc/vnets_dns_phy.h"
#include "../inc/vnets_vpn_link.h"

static USHORT g_usVnetsCmdUdpPort = VNET_CONF_DFT_UDP_PORT;  
static BOOL_T g_bVnetsCmdUdpStart = FALSE;

static BS_STATUS _vnet_cmdudp_OpenService()
{
    if (BS_OK != VNETS_UDP_PHY_Init( 0, g_usVnetsCmdUdpPort))
    {
        EXEC_OutInfo(" Can't start vnet udp server(port:%d).\r\n", g_usVnetsCmdUdpPort);
        return BS_ERR;
    }
    
    return BS_OK;
}


PLUG_API BS_STATUS VNETS_CmdUdp_SetServicePort(int argc, char **argv)
{
    UINT uiPort;

    TXT_Atoui(argv[1], &uiPort);

    g_usVnetsCmdUdpPort = uiPort;

    return BS_OK;
}


PLUG_API BS_STATUS VNETS_CmdUdp_Start(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    if (BS_OK == _vnet_cmdudp_OpenService())
    {
        g_bVnetsCmdUdpStart = TRUE;
    }

    return BS_OK;
}


PLUG_API BS_STATUS VNETS_CmdUdp_ShowThis(IN UINT ulArgc, IN UCHAR **argv)
{
    if (g_usVnetsCmdUdpPort != VNET_CONF_DFT_UDP_PORT)
    {
        EXEC_OutInfo(" port %d\r\n", g_usVnetsCmdUdpPort);
    }

    if (g_bVnetsCmdUdpStart == TRUE)
    {
        EXEC_OutString(" start\r\n");
    }

    return BS_OK;
}

BS_STATUS VNETS_CmdUdp_Save(IN HANDLE hFile)
{
    if ((g_usVnetsCmdUdpPort == VNET_CONF_DFT_UDP_PORT) || (g_bVnetsCmdUdpStart == FALSE)) {
        return 0;
    }

    if (0 != CMD_EXP_OutputMode(hFile, "service udp")) {
        return 0;
    }

    if (g_usVnetsCmdUdpPort != VNET_CONF_DFT_UDP_PORT) {
        CMD_EXP_OutputCmd(hFile, "port %d", g_usVnetsCmdUdpPort);
    }

    if (g_bVnetsCmdUdpStart) {
        CMD_EXP_OutputCmd(hFile, "start");
    }

    CMD_EXP_OutputModeQuit(hFile);

    return BS_OK;
}
