/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_wsapp.h"

#include "webproxyapp_func.h"

BS_STATUS WebProxyApp_Init()
{
    WebProxyApp_Deliver_Init();
    WebProxyApp_Main_Init();
    return BS_OK;
}

/* save */
PLUG_API BS_STATUS WebProxyApp_Save(IN HANDLE hFile)
{
    WebProxyApp_Cmd_Save(hFile);

    return BS_OK;
}

