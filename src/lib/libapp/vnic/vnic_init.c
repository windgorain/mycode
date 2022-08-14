/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-5-4
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "vnic_func.h"

BS_STATUS VNIC_Init()
{
#ifdef IN_WINDOWS
    VnicIns_Init();
#endif

    return 0;
}


