/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-1
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_SERVERINIT

#include "bs.h"

#include "utl/local_info.h"
#include "utl/exec_utl.h"
#include "comp/comp_dc.h"
#include "comp/comp_wsapp.h"

#include "../../vnet/inc/vnet_mac_acl.h"

#include "../../inc/vnet_ifnet.h"
#include "../../inc/vnet_conf.h"

#include "../inc/vnets_conf.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_functbl.h"
#include "../inc/vnets_cmd.h"
#include "../inc/vnets_db.h"
#include "../inc/vnets_dc.h"
#include "../inc/vnets_phy.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_tp.h"
#include "../inc/vnets_protocol.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_dc.h"
#include "../inc/vnets_enter_domain.h"
#include "../inc/vnets_rcu.h"
#include "../inc/vnets_kf.h"
#include "../inc/vnets_master.h"
#include "../inc/vnets_worker.h"
#include "../inc/vnets_wan_plug.h"
#include "../inc/vnets_web.h"
#include "../inc/vnets_mac_layer.h"

#define VNETS_ETH_ADDR "\x00\xff\x58\x5c\xa1\x54"

#define VNET_DOMAIN_MAX_DOMAIN_NUM 1000

typedef BS_STATUS (*PF_VNETS_INIT_FUNC)();


static PF_VNETS_INIT_FUNC g_apfVnetsInit1[] =
{
    VNETS_NODE_Init,
    VNETS_RCU_Init,
    VNETS_Web_Init,
    VNETS_WebKf_Init,
    VNETS_WebVldCode_Init,
    VNETS_WebUlm_Init,
    VNETS_Protocol_Init,
    VNETS_TP_Init,
    VNETS_Worker_Init,
    VNETS_Master_Init,
    VNETS_MAC_ACL_Init,
    VNETS_DC_Init,
    VNETS_Domain_Init,
    VNETS_WAN_PLUG_Init,
    VNETS_KF_Init,
};


static PF_VNETS_INIT_FUNC g_apfVnetsInit2[] =
{
    VNETS_SES_Init,
    VNETS_TP_Init2,
    VNETS_NODE_Init2,
};

static CMD_EXP_NO_DBG_NODE_S g_stVnetsNoDbgNode;

static VOID vnets_NoDbgAll()
{
    VNETS_MacLayer_NoDebugAll();
    VNETS_SES_NoDebugAll();
    VNETS_Protocol_NoDebugAll();
}

BS_STATUS VNETS_Init()
{
    MAC_ADDR_S stMac;
    BS_STATUS eRet = BS_OK;
    UINT i;

    g_stVnetsNoDbgNode.pfNoDbgFunc = vnets_NoDbgAll;
    CMD_EXP_RegNoDbgFunc(&g_stVnetsNoDbgNode);

    MEM_Copy(stMac.aucMac, VNETS_ETH_ADDR, 6);
    VNETS_SetHostMac(&stMac);

    for (i=0; i<sizeof(g_apfVnetsInit1)/sizeof(PF_VNETS_INIT_FUNC); i++)
    {
        eRet = g_apfVnetsInit1[i]();
        if (BS_OK != eRet) {
            EXEC_OutInfo("[1.%d]", i);
            return eRet;
        }
    }

    for (i=0; i<sizeof(g_apfVnetsInit2)/sizeof(PF_VNETS_INIT_FUNC); i++)
    {
        eRet = g_apfVnetsInit2[i]();
        if (BS_OK != eRet) {
            EXEC_OutInfo("[2.%d]", i);
            return eRet;
        }
    }

    VNETS_FuncTbl_Reg();

    return eRet;
}

PLUG_API BS_STATUS VNETS_SaveCmd (IN HANDLE hFile)
{
    VNETS_CmdSave(hFile);

    return BS_OK;
}


