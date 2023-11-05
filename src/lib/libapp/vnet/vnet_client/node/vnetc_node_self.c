/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-5-19
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/txt_utl.h"

#include "../../inc/vnet_node.h"

#include "../inc/vnetc_node.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_ses.h"


static UINT g_uiVnetcSelfNid; 
static UINT g_uiVnetcSelfCookie;
static CHAR g_szVnetcSelfCookieString[512] = "";

VOID VNETC_NODE_SetSelf(IN UINT uiSelfNodeID)
{
    g_uiVnetcSelfNid = uiSelfNodeID;
}

VOID VNETC_NODE_SetSelfCookie(IN UINT uiCookie)
{
    g_uiVnetcSelfCookie = uiCookie;
}


UINT VNETC_NODE_SelfCookie()
{
    return g_uiVnetcSelfCookie;
}

VOID VNETC_NODE_SetSelfCookieString(IN CHAR *pcCookieString)
{
    TXT_Strlcpy(g_szVnetcSelfCookieString, pcCookieString, sizeof(g_szVnetcSelfCookieString));
}

CHAR * VNETC_NODE_GetSelfCookieString()
{
    return g_szVnetcSelfCookieString;
}

UINT VNETC_NODE_Self()
{
    return g_uiVnetcSelfNid;
}


PLUG_API BS_STATUS VNETC_NODE_ShowSelf(IN UINT uiArgc, IN CHAR **argv)
{
    EXEC_OutInfo(" Self node ID is %s%d.\r\n",
            VNET_NODE_GetTypeStringByNID(g_uiVnetcSelfNid),
            VNET_NID_INDEX(g_uiVnetcSelfNid));

    return BS_OK;
}

