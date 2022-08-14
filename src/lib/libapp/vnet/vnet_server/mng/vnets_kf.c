/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-15
* Description: kf,用于外部管理vnets
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mime_utl.h"
#include "comp/comp_kfapp.h"

#include "../inc/vnets_dc.h"

static BS_STATUS vnets_kf_UserAdd(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *param)
{
    CHAR *pcUserName;
    CHAR *pcPassword;

    pcUserName = MIME_GetKeyValue(hMime, "UserName");
    pcPassword = MIME_GetKeyValue(hMime, "Password");

    if ((NULL == pcUserName) || (NULL == pcPassword))
    {
        return BS_BAD_PARA;
    }

    return VNETS_DC_AddUser(pcUserName, pcPassword);
}

BS_STATUS VNETS_KF_Init()
{
    KFAPP_RegFunc("vnets.user.add", vnets_kf_UserAdd, NULL);

    return BS_OK;
}

