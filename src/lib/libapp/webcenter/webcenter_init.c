/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-12
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/file_utl.h"

#include "comp/comp_wsapp.h"
#include "comp/comp_kfapp.h"
#include "comp/comp_localuser.h"

#include "webcenter_inner.h"

BS_STATUS WebCenter_Init()
{
    WebCenter_Deliver_Init();
    WebCenter_ULM_Init();
    WebCenter_KF_Init();

	return BS_OK;
}

/* save */
PLUG_API BS_STATUS WebCenter_Save(IN HANDLE hFile)
{
    WebCenter_Cmd_Save(hFile);

    return BS_OK;
}


