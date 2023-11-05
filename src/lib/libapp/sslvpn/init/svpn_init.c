/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-7-29
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_wsapp.h"
#include "comp/comp_if.h"
#include "comp/comp_wan.h"
#include "comp/comp_kfapp.h"
#include "comp/comp_vnic.h"

#include "../context/svpn_context_cmd.h"

#include "../h/svpn_def.h"
#include "../h/svpn_cfglock.h"
#include "../h/svpn_context.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_web_proxy.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_mf.h"
#include "../h/svpn_ulm.h"
#include "../h/svpn_ippool.h"
#include "../h/svpn_iptunnel.h"
#include "../h/svpn_deliver.h"
#include "../h/svpn_ctxdata.h"

BS_STATUS SVPN_Init()
{
    SVPN_ContextKf_Init();
    SVPN_Deliver_Init();
    SVPN_CfgLock_Init();
    SVPN_WebProxy_Init();
    SVPN_Context_Init();
    SVPN_DWEB_Init();
    SVPN_CtxData_Init();
    SVPN_MF_Init();
    SVPN_ULM_Init();
    SVPN_IpTunNode_Init();
    SVPN_IPPOOL_Init();
    SVPN_IpTunnel_Init();

    return BS_OK;
}


PLUG_API BS_STATUS SVPN_Save(IN HANDLE hFile)
{
    SVPN_ContextCmd_Save(hFile);

    return BS_OK;
}

