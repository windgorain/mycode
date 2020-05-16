/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-3-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../inc/vnetc_rmt.h"

BS_STATUS VNETC_FuncTbl_Reg()
{
    FUNCTBL_AddFunc(VNETC_RMT_GetDomainName, FUNCTBL_RET_SEQUENCE, "");
    FUNCTBL_AddFunc(VNETC_RMT_GetUserName, FUNCTBL_RET_SEQUENCE, "");
    FUNCTBL_AddFunc(VNETC_RMT_GetUserPasswd, FUNCTBL_RET_SEQUENCE, "");
    FUNCTBL_AddFunc(VNETC_RMT_SetUserConfig, FUNCTBL_RET_UINT32, "sss");

    return BS_OK;
}

