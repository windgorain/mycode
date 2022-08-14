/* retcode所需要的宏 */
#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_SERVERINIT
    
#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/ippool_utl.h"
#include "utl/txt_utl.h"
#include "utl/dns_utl.h"

#include "../../vnet/inc/vnet_mac_acl.h"

#include "../inc/vnets_udp_phy.h"
#include "../inc/vnets_dns_phy.h"
#include "../inc/vnets_vpn_link.h"
#include "../inc/vnets_cmd.h"
#include "../inc/vnets_web.h"

/* broadcast { permit | deny } */
PLUG_API BS_STATUS VNETS_SetPermitBroadcast(int ulArgc, char **argv)
{
    BOOL_T bIsPermit = FALSE;

    if (strcmp(argv[1], "permit") == 0)
    {
        bIsPermit = TRUE;
    }

    VNETS_MAC_ACL_PermitBroadcast(bIsPermit);

    return BS_OK;
}

BS_STATUS VNETS_CmdSave(IN HANDLE hFile)
{
    CMD_EXP_OutputCmd(hFile, "broadcast %s", VNETS_MAC_ACL_IsPermitBroadcast() == TRUE ? "permit" : "deny");

    VNETS_CmdUdp_Save(hFile);

    VNETS_CmdWeb_Save(hFile);

    return BS_OK;
}


