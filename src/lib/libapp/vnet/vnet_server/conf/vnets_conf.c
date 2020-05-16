/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-5
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/eth_utl.h"

static MAC_ADDR_S g_stVnetsConfHostMac;

VOID VNETS_SetHostMac(IN MAC_ADDR_S *pstMac)
{
    g_stVnetsConfHostMac = *pstMac;
}

MAC_ADDR_S * VNETS_GetHostMac()
{
    return &g_stVnetsConfHostMac;
}

BOOL_T VNETS_IsHostMac(IN MAC_ADDR_S *pstMac)
{
    if (MAC_ADDR_IS_EQUAL(pstMac->aucMac, g_stVnetsConfHostMac.aucMac))
    {
        return TRUE;
    }

    return FALSE;
}

