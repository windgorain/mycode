/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-5-11
* Description: 桥mac
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/rand_utl.h"
#include "utl/eth_utl.h"

static MAC_ADDR_S g_stSptBridgeMac = {{0x0, 0xff, 0x00, 0x00, 0x00, 0x00}};

VOID SupportBridge_Init()
{
    UINT uiRand;

    uiRand = RAND_Get();

    g_stSptBridgeMac.aucMac[2] = ((uiRand & 0xff000000) >> 24);
    g_stSptBridgeMac.aucMac[3] = ((uiRand & 0x00ff0000) >> 16);
    g_stSptBridgeMac.aucMac[4] = ((uiRand & 0x0000ff00) >> 8);
    g_stSptBridgeMac.aucMac[5] = (uiRand & 0x000000ff);
}

MAC_ADDR_S * SupportBridge_GetMac()
{
    return &g_stSptBridgeMac;
}

VOID SupportBridge_IoctlGetMac(OUT MAC_ADDR_S *pstMac)
{
    MAC_ADDR_COPY(pstMac->aucMac, g_stSptBridgeMac.aucMac);
}


