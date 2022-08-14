/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-5-11
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../h/support_func.h"
#include "../h/support_bridge.h"

BS_STATUS Support_Init()
{
    SupportBridge_Init();

    return BS_OK;
}

