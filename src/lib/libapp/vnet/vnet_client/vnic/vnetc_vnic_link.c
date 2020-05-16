/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_if.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_mac_fw.h"
#include "../inc/vnetc_arp_monitor.h"
#include "../inc/vnetc_context.h"
#include "../inc/vnetc_master.h"
#include "../inc/vnetc_vnic_link.h"
#include "../inc/vnetc_vnic_node.h"

static VOID vnetc_vnic_link_MsgInput(IN USER_HANDLE_S *pstUserHandle)
{
    MBUF_S *pstMbuf = pstUserHandle->ahUserHandle[0];

    VNETC_Context_SetSrcNID(pstMbuf, VNETC_VnicNode_GetNID());

    VNETC_MAC_FW_Input (pstMbuf);
}

BS_STATUS VNET_VNIC_LinkInput (IN UINT ulIfIndex, IN MBUF_S *pstMbuf)
{
    BS_STATUS eRet;
    USER_HANDLE_S stUserHandle;
    
    VNETC_ArpMonitor_ProcArpRequest(ulIfIndex, pstMbuf);

    stUserHandle.ahUserHandle[0] = pstMbuf;

    eRet = VNETC_Master_MsgInput(vnetc_vnic_link_MsgInput, &stUserHandle);
    if (BS_OK != eRet)
    {
        MBUF_Free(pstMbuf);
        return eRet;
    }

    return BS_OK;
}

BS_STATUS VNET_VNIC_LinkOutput (IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoTyp)
{
    VNETC_ArpMonitor_PacketMonitor(pstMbuf);
    
    return CompIf_PhyOutput (ulIfIndex, pstMbuf);
}

