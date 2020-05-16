/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-4
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/file_utl.h"
#include "utl/local_info.h"

#include "comp/comp_wsapp.h"

#include "webproxyapp_func.h"

static CHAR g_szWebProxyAppBindWsService[WSAPP_SERVICE_NAME_LEN + 1];

/* bind ws-service %STRING */
PLUG_API BS_STATUS WebProxyApp_Cmd_BindWsService(IN UINT ulArgc, IN CHAR **argv)
{
    CHAR *pcWsService = argv[2];
    CHAR szFullPath[FILE_MAX_PATH_LEN + 1];

    if (BS_OK != COMP_WSAPP_BindService(pcWsService))
    {
        EXEC_OutString("bind ws service failed.\r\n");
        return BS_ERR;
    }

    strlcpy(g_szWebProxyAppBindWsService, pcWsService, sizeof(g_szWebProxyAppBindWsService));

    WebProxyApp_Deliver_BindService(pcWsService);

    LOCAL_INFO_ExpandToConfPath("web/", szFullPath);
    COMP_WSAPP_SetDocRoot(pcWsService, szFullPath);
    COMP_WSAPP_SetIndex(pcWsService, "/index.htm");

    return BS_OK;
}

BS_STATUS WebProxyApp_Cmd_Save(IN HANDLE hFile)
{
    if (g_szWebProxyAppBindWsService[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFile, "bind ws-service %s", g_szWebProxyAppBindWsService);
    }

    return BS_OK;
}

