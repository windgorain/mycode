/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-5-5
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_vnic.h"

#include "vnic_func.h"

static COMP_VNIC_S g_stVnicComp;

VOID VNIC_COMP_Init()
{
    /* vrf */
    g_stVnicComp.pfStart = VnicIns_Start;
    g_stVnicComp.pfStop = VnicIns_Stop;
    g_stVnicComp.pfRegRecver = VnicIns_RegRecver;
    g_stVnicComp.pfGetVnicHandle = VnicIns_GetVnicHandle;
    g_stVnicComp.pfVnicOutput = VnicIns_Output;
    g_stVnicComp.pfGetVnicMac = VnicIns_GetVnicMac;
    g_stVnicComp.comp.comp_name = COMP_VNIC_NAME;

    COMP_Reg(&g_stVnicComp.comp);
}
