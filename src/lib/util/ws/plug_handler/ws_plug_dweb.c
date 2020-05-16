/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-10
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
        
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/time_utl.h"
#include "utl/drp_utl.h"
#include "utl/ws_utl.h"
    
#include "../ws_def.h"
#include "../ws_conn.h"
#include "../ws_trans.h"
#include "../ws_event.h"
#include "../ws_context.h"
#include "../ws_drp.h"

typedef struct
{
    CHAR szFilePath[FILE_MAX_PATH_LEN + 1];
    DRP_FILE *pstDFile;
}_WS_PLUG_DWEB_S;

static CHAR * g_apcWsPlugDwebFileExts[] = 
{
    "cgi"
};

static BOOL_T ws_plugdweb_IsDwebRequest(IN CHAR *pcRequestFile)
{
    CHAR *pcExternName;
    UINT i;

    pcExternName = FILE_GetExternNameFromPath(pcRequestFile, strlen(pcRequestFile));
    if (NULL == pcExternName)
    {
        return FALSE;
    }

    for (i=0; i<sizeof(g_apcWsPlugDwebFileExts)/sizeof(CHAR*); i++)
    {
        if (stricmp(pcExternName, g_apcWsPlugDwebFileExts[i]) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static WS_EV_RET_E ws_plugdweb_HeadOK(IN WS_TRANS_S *pstTrans)
{
    WS_CONTEXT_HANDLE hContext;
    CHAR *pcRequestFile;
    CHAR szFilePath[FILE_MAX_PATH_LEN + 1];
    CHAR *pcStaticPath;
    _WS_PLUG_DWEB_S *pstCtrl;

    hContext = pstTrans->hContext;
    if (NULL == hContext)
    {
        return WS_EV_RET_ERR;
    }

    pcRequestFile = pstTrans->pcRequestFile;
    if (NULL == pcRequestFile)
    {
        return WS_EV_RET_ERR;
    }

    if (FALSE == ws_plugdweb_IsDwebRequest(pcRequestFile))
    {
        return WS_EV_RET_CONTINUE;
    }

    pcRequestFile ++; /* 去掉开始的'/' */

    pcStaticPath = WS_Context_File2RootPathFile(hContext, pcRequestFile, szFilePath, sizeof(szFilePath));
    if (pcStaticPath == NULL)
    {    
        WS_Trans_Reply(pstTrans, HTTP_STATUS_NOT_FOUND, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
        return WS_EV_RET_BREAK;
    }

    _WS_Trans_SetFlag(pstTrans, WS_TRANS_FLAG_DROP_REQ_BODY);

    pstCtrl = MEM_ZMalloc(sizeof(_WS_PLUG_DWEB_S));
    if (NULL == pstCtrl)
    {
        return WS_EV_RET_ERR;
    }

    TXT_Strlcpy(pstCtrl->szFilePath, szFilePath, sizeof(pstCtrl->szFilePath));

    pstTrans->apPlugContext[WS_PLUG_DWEB] = pstCtrl;

    return WS_EV_RET_BREAK;
}

static WS_EV_RET_E ws_plugdweb_RecvBodyOK(IN WS_TRANS_S *pstTrans)
{
    DRP_FILE *pstDFile;
    _WS_PLUG_DWEB_S *pstCtrl;
    CHAR szStringTime[HTTP_RFC1123_DATESTR_LEN + 1];
    time_t uiTime;
    USER_HANDLE_S stUserHandle;

    pstCtrl = pstTrans->apPlugContext[WS_PLUG_DWEB];
    if (NULL == pstCtrl)
    {
        return WS_EV_RET_CONTINUE;
    }

    stUserHandle.ahUserHandle[0] = pstTrans;
    pstDFile = DRP_FileOpen(_WS_Drp_GetDrp(), pstCtrl->szFilePath, &stUserHandle);
    if (NULL == pstDFile)
    {
        WS_Trans_Reply(pstTrans, HTTP_STATUS_NOT_FOUND, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
        return WS_EV_RET_BREAK;
    }

    pstCtrl->pstDFile = pstDFile;

    HTTP_SetStatusCode(pstTrans->hHttpHeadReply, HTTP_STATUS_OK);

    HTTP_SetContentLen(pstTrans->hHttpHeadReply, (ULONG)DRP_FileLength(pstDFile));

    uiTime = TM_NowInSec();
    HTTP_DateToStr(uiTime, szStringTime);
    HTTP_SetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_DATE, szStringTime);

    HTTP_SetNoCache(pstTrans->hHttpHeadReply);

    WS_Trans_SetHeadFieldFinish(pstTrans);

    return WS_EV_RET_BREAK;
}

static WS_EV_RET_E ws_plugdweb_BuildBody(IN WS_TRANS_S *pstTrans)
{
    _WS_PLUG_DWEB_S *pstCtrl;
    MBUF_S *pstMbuf;
    UCHAR aucData[1024];
    INT iReadLen;
    
    pstCtrl = pstTrans->apPlugContext[WS_PLUG_DWEB];
    if (NULL == pstCtrl)
    {
        return WS_EV_RET_CONTINUE;
    }

    if (DRP_FileEOF(pstCtrl->pstDFile))
    {
        WS_Trans_ReplyBodyFinish(pstTrans);
        return WS_EV_RET_BREAK;
    }

    iReadLen = DRP_FileRead(pstCtrl->pstDFile, aucData, sizeof(aucData));
    if (iReadLen < 0)
    {
        return WS_EV_RET_ERR;
    }

    pstMbuf = MBUF_CreateByCopyBuf(MBUF_DFT_RESERVED_HEAD_SPACE, aucData, iReadLen, MBUF_DATA_DATA);
    if (NULL == pstMbuf)
    {
        return WS_EV_RET_ERR;
    }

    WS_Trans_AddReplyBody(pstTrans, pstMbuf);

    return WS_EV_RET_BREAK;
}

static WS_EV_RET_E ws_plugdweb_Destory(IN WS_TRANS_S *pstTrans)
{
    _WS_PLUG_DWEB_S *pstCtrl;

    pstCtrl = pstTrans->apPlugContext[WS_PLUG_DWEB];
    if (NULL != pstCtrl)
    {
        if (pstCtrl->pstDFile != NULL)
        {
            DRP_FileClose(pstCtrl->pstDFile);
        }
        MEM_Free(pstCtrl);
        pstTrans->apPlugContext[WS_PLUG_DWEB] = NULL;
    }

    return WS_EV_RET_CONTINUE;
}

WS_EV_RET_E _WS_PlugDWeb_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    WS_EV_RET_E eRet = WS_EV_RET_CONTINUE;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_HEAD_OK:
        {
            eRet = ws_plugdweb_HeadOK(pstTrans);
            break;
        }

        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = ws_plugdweb_RecvBodyOK(pstTrans);
            break;
        }

        case WS_TRANS_EVENT_BUILD_BODY:
        {
            eRet = ws_plugdweb_BuildBody(pstTrans);
            break;
        }

        case WS_TRANS_EVENT_DESTORY:
        {
            eRet = ws_plugdweb_Destory(pstTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}



