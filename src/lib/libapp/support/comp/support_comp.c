/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-5-11
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_support.h"

#include "../h/support_ioctl.h"

static COMP_SUPPORT_S g_stSupportComp;

VOID SupportComp_Init()
{
    g_stSupportComp.pfIoctl = Support_Ioctl;
    g_stSupportComp.comp.comp_name = COMP_SUPPORT_NAME;

    COMP_Reg(&g_stSupportComp.comp);
}


