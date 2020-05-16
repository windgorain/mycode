/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-16
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/local_info.h"
#include "comp/comp_wsapp.h"

#include "vnets_web_inner.h"

static CHAR g_szVnetsBindWsService[WSAPP_SERVICE_NAME_LEN + 1];

/* bind ws-service %STRING */
PLUG_API BS_STATUS VNETS_CmdWeb_BindWsService(IN UINT ulArgc, IN UCHAR **argv)
{
    CHAR *pcWsService = argv[2];
    CHAR szFullPath[FILE_MAX_PATH_LEN + 1];

    if (BS_OK != COMP_WSAPP_BindService(pcWsService))
    {
        EXEC_OutString("bind ws service failed.\r\n");
        return BS_ERR;
    }

    strlcpy(g_szVnetsBindWsService, pcWsService, sizeof(g_szVnetsBindWsService));

    VNETS_Web_BindService(pcWsService);

    LOCAL_INFO_ExpandToConfPath("web/", szFullPath);
    COMP_WSAPP_SetDocRoot(pcWsService, szFullPath);
    COMP_WSAPP_SetIndex(pcWsService, "/index.htm");

    return BS_OK;
}

BS_STATUS VNETS_CmdWeb_Save(IN HANDLE hFile)
{
    if (g_szVnetsBindWsService[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFile, "bind ws-service %s", g_szVnetsBindWsService);
    }

    return BS_OK;
}


