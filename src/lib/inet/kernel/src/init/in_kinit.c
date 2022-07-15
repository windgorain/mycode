/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-4
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "protosw.h"
#include "udp_func.h"

BS_STATUS IN_KInit()
{
    IN_Proto_Init();
    udp_Init();

    return BS_OK;
}

