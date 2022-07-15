/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-5
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"

extern INT VNETC_Init();

static int _vnetc_init(IN CHAR *pszPlugFileName)
{
    BS_STATUS eRet;

    if (0 != (eRet = VNETC_Init())) {
        EXEC_OutString(" Can't init vnet client!\r\n");
    }

    return eRet;
}

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            return _vnetc_init();
        default:
            break;
    }

    return 0;
}

PLUG_MAIN


