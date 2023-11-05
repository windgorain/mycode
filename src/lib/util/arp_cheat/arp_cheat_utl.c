/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/arp_utl.h"
#include "utl/arp_cheat_utl.h"
#include "comp/comp_pcap.h"

typedef struct
{
    UINT uiPcapIndex;
    UINT uiCheatIP;         
    MAC_ADDR_S stCheatMac;  
}_ARP_CHEAT_CTRL_S;

ARP_CHEAT_HANDLE ARPCheat_Create()
{
    _ARP_CHEAT_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_ARP_CHEAT_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->uiPcapIndex = PCAP_INVALID_INDEX;

    return pstCtrl;
}

VOID ARPCheat_SetPcapIndex(IN ARP_CHEAT_HANDLE hArpCheat, IN UINT uiPcapIndex)
{
    _ARP_CHEAT_CTRL_S *pstCtrl = hArpCheat;

    pstCtrl->uiPcapIndex = uiPcapIndex;
}

UINT ARPCheat_GetPcapIndex(IN ARP_CHEAT_HANDLE hArpCheat)
{
    _ARP_CHEAT_CTRL_S *pstCtrl = hArpCheat;

    return pstCtrl->uiPcapIndex;
}

VOID ARPCheat_SetCheatIP(IN ARP_CHEAT_HANDLE hArpCheat, IN UINT uiCheatIP)
{
    _ARP_CHEAT_CTRL_S *pstCtrl = hArpCheat;

    pstCtrl->uiCheatIP = uiCheatIP;
}

UINT ARPCheat_GetCheatIP(IN ARP_CHEAT_HANDLE hArpCheat)
{
    _ARP_CHEAT_CTRL_S *pstCtrl = hArpCheat;

    return pstCtrl->uiCheatIP;
}

VOID ARPCheat_SetCheatMAC(IN ARP_CHEAT_HANDLE hArpCheat, IN MAC_ADDR_S *pstMacAddr)
{
    _ARP_CHEAT_CTRL_S *pstCtrl = hArpCheat;

    pstCtrl->stCheatMac = *pstMacAddr;
}

MAC_ADDR_S * ARPCheat_GetCheatMAC(IN ARP_CHEAT_HANDLE hArpCheat)
{
    _ARP_CHEAT_CTRL_S *pstCtrl = hArpCheat;

    if (MAC_ADDR_IS_ZERO(pstCtrl->stCheatMac.aucMac))
    {
        return NULL;
    }

    return &pstCtrl->stCheatMac;
}

BS_STATUS ARPCheat_SendCheatPkt(IN ARP_CHEAT_HANDLE hArpCheat)
{
    _ARP_CHEAT_CTRL_S *pstCtrl = hArpCheat;
    MBUF_S *pstMbuf;

    pstMbuf = ARP_BuildPacketWithEthHeader(pstCtrl->uiCheatIP, 0xffffffff, pstCtrl->stCheatMac.aucMac,
                              NULL, ARP_REPLY);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    return PCAP_SendPkt(pstCtrl->uiPcapIndex, pstMbuf);
}

