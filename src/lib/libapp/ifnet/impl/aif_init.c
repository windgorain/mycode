/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "aif_func.h"

int AIF_Loaded()
{
    AIF_PCAP_Load();
#ifdef IN_WINDOWS
    AIF_VNIC_Load();
#endif

    return 0;
}

int AIF_CmdReged0()
{
    AIF_PCAP_AfterRegCmd0();
    TUN_Init();
    TAP_Init();
    return 0;
}

int AIF_CfgLoaded()
{
    AIF_PCAP_AutoCreateInterface();
    return 0;
}

