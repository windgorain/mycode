/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-7-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/cjson.h"
#include "utl/txt_utl.h"
#include "utl/conn_utl.h"
#include "utl/ssl_utl.h"
#include "utl/mime_utl.h"
#include "utl/http_lib.h"

#include "../h/svpnc_conf.h"
#include "../h/svpnc_utl.h"

#include "svpnc_tcprelay_inner.h"


static MYPOLL_HANDLE g_hSvpncTcpRelayMyPoll = NULL;

static BS_STATUS _svpnc_tr_Main(IN USER_HANDLE_S *pstUserHandle)
{
    MyPoll_Run(g_hSvpncTcpRelayMyPoll);

	return BS_OK;
}

BS_STATUS SVPNC_TR_MainInit()
{
    g_hSvpncTcpRelayMyPoll = MyPoll_Create();
    if (g_hSvpncTcpRelayMyPoll == NULL)
    {
        return BS_ERR;
    }

    if (THREAD_ID_INVALID ==
            THREAD_Create("svpnc_tr", NULL, _svpnc_tr_Main, NULL))
    {
        return BS_ERR;
    }

    return BS_OK;
}

MYPOLL_HANDLE SVPNC_TR_GetMyPoller()
{
    return g_hSvpncTcpRelayMyPoll;
}

