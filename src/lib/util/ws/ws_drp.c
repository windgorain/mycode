/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
        
#include "utl/txt_utl.h"
#include "utl/ws_utl.h"
    
#include "ws_def.h"
#include "ws_context.h"
#include "ws_vhost.h"
#include "ws_trans.h"


typedef struct
{
    CHAR *pcKey;
    PF_DRP_SOURCE_FUNC pfFunc;
}WS_DRP_NODE_S;

static BS_STATUS ws_drp_Jump2Url
(
    IN DRP_HANDLE hDrp,
    IN LSTR_S *pstKey,
    IN VOID *pDrpCtx,
    IN HANDLE hUserHandle,
    IN HANDLE hUserHandle2
);
static BS_STATUS ws_drp_ContextList
(
    IN DRP_HANDLE hDrp,
    IN LSTR_S *pstKey,
    IN VOID *pDrpCtx,
    IN HANDLE hUserHandle,
    IN HANDLE hUserHandle2
);

static WS_DRP_NODE_S g_astWsDrps[] =
{
    {"domain.list", ws_drp_ContextList},
    {"domain.jump2url", ws_drp_Jump2Url},
};

static DRP_HANDLE g_hWsDrp = NULL;
static BOOL_T g_bWsDrpInit = FALSE;

static BS_STATUS ws_drp_Jump2Url
(
    IN DRP_HANDLE hDrp,
    IN LSTR_S *pstKey,
    IN VOID *pDrpCtx,
    IN HANDLE hUserHandle,
    IN HANDLE hUserHandle2
)
{
    WS_TRANS_S *pstTrans;
    USER_HANDLE_S *pstUserHandle = hUserHandle;
    CHAR *pcJump2Url;

    pstTrans = pstUserHandle->ahUserHandle[0];

    pcJump2Url = MIME_GetKeyValue(pstTrans->hQuery, "url");
    if (NULL == pcJump2Url)
    {
        pcJump2Url = "";
    }

    DRP_CtxOutputString(pDrpCtx, "\"");
    DRP_CtxOutputString(pDrpCtx, pcJump2Url);
    DRP_CtxOutputString(pDrpCtx, "\"");

    return BS_OK;
}

static BS_STATUS ws_drp_ContextList
(
    IN DRP_HANDLE hDrp,
    IN LSTR_S *pstKey,
    IN VOID *pDrpCtx,
    IN HANDLE hUserHandle,
    IN HANDLE hUserHandle2
)
{
    WS_TRANS_S *pstTrans;
    CHAR *pcContext = NULL;
    BOOL_T bFirst = TRUE;
    USER_HANDLE_S *pstUserHandle = hUserHandle;

    pstTrans = pstUserHandle->ahUserHandle[0];

    DRP_CtxOutputString(pDrpCtx, "[");

    while (NULL != (pcContext = WS_Context_GetNext(pstTrans->hVHost, pcContext)))
    {
        if (bFirst == TRUE)
        {
            bFirst = FALSE;
        }
        else
        {
            DRP_CtxOutputString(pDrpCtx, ",");
        }

        DRP_CtxOutputString(pDrpCtx, "{\"Name\":\"");
        DRP_CtxOutputString(pDrpCtx, pcContext);
        DRP_CtxOutputString(pDrpCtx, "\"}");
    }    

    DRP_CtxOutputString(pDrpCtx, "]");

    return BS_OK;
}

BS_STATUS _WS_DRP_Init()
{
    UINT i;
    BOOL_T bNeedInit = FALSE;

    SPLX_P();
    if (g_bWsDrpInit == FALSE)
    {
        bNeedInit = TRUE;
        g_bWsDrpInit = TRUE;
    }
    SPLX_V();

    if (bNeedInit == FALSE)
    {
        return BS_OK;
    }

    g_hWsDrp = DRP_Create("<?cgi", "?>");
    if (g_hWsDrp == NULL)
    {
        return BS_ERR;
    }

    for (i=0; i<sizeof(g_astWsDrps)/sizeof(WS_DRP_NODE_S); i++)
    {
        DRP_Set(g_hWsDrp, g_astWsDrps[i].pcKey, g_astWsDrps[i].pfFunc, NULL);
    }

    return BS_OK;
}

DRP_HANDLE _WS_Drp_GetDrp()
{
    return g_hWsDrp;
}

