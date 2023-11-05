/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-5
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../../inc/vnet_ifnet.h"
#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_mac_tbl.h"
#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_kf.h"
#include "../inc/vnetc_mac_fw.h"
#include "../inc/vnetc_cmd.h"
#include "../inc/vnetc_auth.h"
#include "../inc/vnetc_ipmac.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_peer_info.h"
#include "../inc/vnetc_enter_domain.h"
#include "../inc/vnetc_vnic_phy.h"
#include "../inc/vnetc_user_status.h"
#include "../inc/vnetc_ip_fwd_service.h"
#include "../inc/vnetc_master.h"
#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_caller.h"
#include "../inc/vnetc_proto_start.h"
#include "../inc/vnetc_p_addr_change.h"
#include "../inc/vnetc_p_nodeinfo.h"
#include "../inc/vnetc_addr_monitor.h"
#include "../inc/vnetc_api.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_ses.h"
#include "../inc/vnetc_fsm.h"
#include "../inc/vnetc_ses_c2s.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_vnic_node.h"
#include "../inc/vnetc_init.h"

typedef BS_STATUS (*PF_VNETC_INIT_FUNC)();


static PF_VNETC_INIT_FUNC g_apfVnetcInit1[] =
{
    VNETC_Ipmac_Init,
    VNETC_Master_Init,
    VNETC_Protocol_Init,
    VNETC_FSM_Init,
    VNETC_TP_Init,
    VNETC_FuncTbl_Reg,
    VNETC_KF_Init,
    VNET_VNIC_PHY_Init,
    VNETC_CMD_Init,
    VNETC_API_Init,
    VNETC_P_AddrChange_Init
};


static PF_VNETC_INIT_FUNC g_apfVnetcInit2[] =
{
    VNETC_SesC2S_Init,
    VNETC_SES_Init,
    VNETC_NODE_Init,
    VNETC_VnicNode_Init,
    VNETC_Proto_Init,
    VNETC_AddrMonitor_Init,
    VNETC_TP_Init2,
    VNETC_AUTH_Init,
    VNETC_EnterDomain_Init,
    VNETC_P_NodeInfo_Init,
    VNETC_MACTBL_Init,
};

static VOID vnetc_Exit(IN INT lExitNum, IN USER_HANDLE_S *pstUserHandle)
{
    VNETC_Stop();
}

static CMD_EXP_NO_DBG_NODE_S g_stVnetcNoDbgNode;

static VOID vnetc_NoDbgAll()
{
    VNETC_MAC_FW_NoDebugAll();
}

INT VNETC_Init()
{
    UINT i;

    g_stVnetcNoDbgNode.pfNoDbgFunc = vnetc_NoDbgAll;
    CMD_EXP_RegNoDbgFunc(&g_stVnetcNoDbgNode);
    
    for (i=0; i<sizeof(g_apfVnetcInit1)/sizeof(PF_VNETC_INIT_FUNC); i++)
    {
        if (BS_OK != g_apfVnetcInit1[i]())
        {
            return -1;
        }
    }

    for (i=0; i<sizeof(g_apfVnetcInit2)/sizeof(PF_VNETC_INIT_FUNC); i++)
    {
        if (BS_OK != g_apfVnetcInit2[i]())
        {
            return -1;
        }
    }

    SYSRUN_RegExitNotifyFunc(vnetc_Exit, NULL);

    return 0;
}


