/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0 
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_muc.h"

#include "../h/muc_cmd.h"
#include "../h/muc_core.h"

BS_STATUS MUC_Init(void *env)
{
    MucCmd_Init();
    MucCore_Init(env);
    return BS_OK;
}

BS_STATUS MUC_Init2()
{
    MucCore_Create(0);
    return BS_OK;
}

