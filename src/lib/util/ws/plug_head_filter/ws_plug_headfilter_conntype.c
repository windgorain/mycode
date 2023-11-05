/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-20
* Description: 
* History:     
******************************************************************************/


#include "bs.h"
    
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/time_utl.h"
#include "utl/ws_utl.h"

#include "../ws_def.h"
#include "../ws_conn.h"
#include "../ws_trans.h"
#include "../ws_event.h"
#include "../ws_context.h"


static VOID ws_plugconntype_PreBuildHead(IN WS_TRANS_S *pstTrans)
{
    HTTP_VERSION_E eVer;

    if (HTTP_CONNECTION_CLOSE == HTTP_GetConnection(pstTrans->hHttpHeadRequest))
    {
        HTTP_SetConnection(pstTrans->hHttpHeadReply, HTTP_CONNECTION_CLOSE);
        return;
    }

    if (NULL != HTTP_GetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_CONTENT_LENGTH))
    {
        return;
    }

    eVer = HTTP_GetVersion(pstTrans->hHttpHeadRequest);
    if ((eVer == HTTP_VERSION_0_9)  || (eVer == HTTP_VERSION_1_0))
    {
        HTTP_SetConnection(pstTrans->hHttpHeadReply, HTTP_CONNECTION_CLOSE);
        return;
    }

    HTTP_SetConnection(pstTrans->hHttpHeadReply, HTTP_CONNECTION_CLOSE);

    return;
}

WS_EV_RET_E _WS_PlugConnType_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    switch (uiEvent)
    {
        case WS_TRANS_EVENT_PRE_BUILD_HEAD:
        {
            (VOID) ws_plugconntype_PreBuildHead(pstTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    return WS_EV_RET_CONTINUE;
}


