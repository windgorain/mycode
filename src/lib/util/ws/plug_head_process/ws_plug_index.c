/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-7-24
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/ws_utl.h"

#include "../ws_def.h"
#include "../ws_conn.h"
#include "../ws_trans.h"
#include "../ws_event.h"
#include "../ws_context.h"


WS_EV_RET_E _WS_PlugIndex_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    HTTP_HEAD_PARSER hReqParser;
    CHAR *pcIndex;
    CHAR *pcRequestFile;
    WS_CONTEXT_HANDLE hContext;
    HTTP_METHOD_E eMethod;

    hContext = pstTrans->hContext;
    if (NULL == hContext)
    {
        return WS_EV_RET_ERR;
    }

    hReqParser = pstTrans->hHttpHeadRequest;

    pcRequestFile = pstTrans->pcRequestFile;
    if (NULL == pcRequestFile)
    {
        return WS_EV_RET_ERR;
    }

    /* 不是请求'/' */
    if (strcmp(pcRequestFile, "/") != 0)
    {
        return WS_EV_RET_CONTINUE;
    }

    /* 不是HTTP的标准方法, 不进行重定向 */
    eMethod = HTTP_GetMethod(hReqParser);
    if (eMethod == HTTP_METHOD_BUTT)
    {
        return WS_EV_RET_CONTINUE;
    }

    /* 请求是'/'才则进行Index重定向的处理 */
    pcIndex = WS_Context_GetIndex(hContext);
    if ((pcIndex == NULL) || (pcIndex[0] == '\0'))
    {
        return WS_EV_RET_CONTINUE;
    }

    WS_Trans_Redirect(pstTrans, pcIndex);
    return WS_EV_RET_STOP;
}


