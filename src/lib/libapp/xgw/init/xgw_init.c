/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#include "bs.h"
#include "../h/xgw_vht.h"
#include "../h/xgw_cfg_lock.h"

int XGW_Init(void)
{
    XGW_CFGLOCK_Init();
    XGW_VHT_Init();
    return 0;
}

