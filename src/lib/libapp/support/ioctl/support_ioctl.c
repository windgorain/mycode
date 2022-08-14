/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-5-11
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_support.h"

#include "../h/support_bridge.h"

PLUG_API BS_STATUS Support_Ioctl(IN UINT uiCmd, IN VOID *pData)
{
    BS_STATUS eRet = BS_OK;

    switch(uiCmd)
    {
        case SPT_IOCTL_GET_BRIDGE_MAC:
        {
            SupportBridge_IoctlGetMac(pData);
            break;
        }

        default:
        {
            BS_DBGASSERT(0);
            eRet = BS_NOT_SUPPORT;
        }
    }

    return eRet;
}

