/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-17
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

typedef struct
{
    CHAR szFilePath[FILE_MAX_PATH_LEN + 1];
    FILE *fp;
}_WS_PLUG_STATIC_S;

static WS_EV_RET_E ws_plugstatic_HeadOK(IN WS_TRANS_S *pstTrans)
{
    WS_CONTEXT_HANDLE hContext;
    CHAR *pcRequestFile;
    CHAR szFilePath[FILE_MAX_PATH_LEN + 1];
    CHAR *pcStaticPath;
    _WS_PLUG_STATIC_S *pstCtrl;

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

    pcRequestFile ++; /* 去掉开始的'/' */

    pcStaticPath = WS_Context_File2RootPathFile(hContext, pcRequestFile, szFilePath, sizeof(szFilePath));
    if (pcStaticPath == NULL)
    {    
        WS_Trans_Reply(pstTrans, HTTP_STATUS_NOT_FOUND, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
        return WS_EV_RET_STOP;
    }

    _WS_Trans_SetFlag(pstTrans, WS_TRANS_FLAG_DROP_REQ_BODY);

    pstCtrl = MEM_ZMalloc(sizeof(_WS_PLUG_STATIC_S));
    if (NULL == pstCtrl)
    {
        return WS_EV_RET_ERR;
    }

    TXT_Strlcpy(pstCtrl->szFilePath, szFilePath, sizeof(pstCtrl->szFilePath));

    pstTrans->apPlugContext[WS_PLUG_STATIC] = pstCtrl;

    return WS_EV_RET_BREAK;
}

static WS_EV_RET_E ws_plugstatic_BuildBody(IN WS_TRANS_S *pstTrans)
{
    _WS_PLUG_STATIC_S *pstCtrl;
    MBUF_S *pstMbuf;
    UCHAR aucData[1024];
    INT iReadLen;
    
    pstCtrl = pstTrans->apPlugContext[WS_PLUG_STATIC];
    if (NULL == pstCtrl)
    {
        return WS_EV_RET_CONTINUE;
    }

    if (feof(pstCtrl->fp))
    {
        WS_Trans_ReplyBodyFinish(pstTrans);
        return WS_EV_RET_STOP;
    }

    iReadLen = fread(aucData, 1, sizeof(aucData), pstCtrl->fp);
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

static BOOL_T ws_plugstatic_IsShouldReply304(IN WS_TRANS_S *pstTrans)
{
    _WS_PLUG_STATIC_S *pstCtrl;
    time_t uiTime;
    CHAR szStringTime[HTTP_RFC1123_DATESTR_LEN + 1];
    HTTP_HEAD_PARSER hParser;
    CHAR *pcIfModSince;
    CHAR *pcETag;

    pstCtrl = pstTrans->apPlugContext[WS_PLUG_STATIC];

    hParser = WS_Trans_GetHttpRequestParser(pstTrans);
    pcIfModSince = HTTP_GetHeadField(hParser, HTTP_FIELD_IF_MODIFIED_SINCE);
    if (NULL == pcIfModSince)
    {
        return FALSE;
    }

    (VOID) FILE_GetUtcTime(pstCtrl->szFilePath, NULL, &uiTime, NULL);
    HTTP_DateToStr(uiTime, szStringTime);
    if (strcmp(pcIfModSince, szStringTime) != 0)
    {
        return FALSE;
    }

    pcETag = HTTP_GetHeadField(hParser, HTTP_FIELD_IF_NONE_MATCH);
    if ((NULL == pcETag) || (strcmp(pcETag, WS_Context_GetDomainName(pstTrans->hContext)) == 0))
    {
        return TRUE;
    }

    return FALSE;
}

static WS_EV_RET_E ws_plugstatic_RecvBodyOK(IN WS_TRANS_S *pstTrans)
{
    S64 filesize;
    FILE *fp;
    _WS_PLUG_STATIC_S *pstCtrl;
    CHAR szStringTime[HTTP_RFC1123_DATESTR_LEN + 1];
    time_t uiTime;

    pstCtrl = pstTrans->apPlugContext[WS_PLUG_STATIC];
    if (NULL == pstCtrl)
    {
        return WS_EV_RET_CONTINUE;
    }

    if (TRUE == ws_plugstatic_IsShouldReply304(pstTrans))
    {
        WS_Trans_Reply(pstTrans, HTTP_STATUS_NOT_MODI, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
        return WS_EV_RET_STOP;
    }

    filesize = FILE_GetSize(pstCtrl->szFilePath);
    if (filesize < 0) {
        WS_Trans_Reply(pstTrans, HTTP_STATUS_NOT_FOUND, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
        return WS_EV_RET_STOP;
    }

    fp = FILE_Open(pstCtrl->szFilePath, FALSE, "rb");
    if (NULL == fp)
    {
        WS_Trans_Reply(pstTrans, HTTP_STATUS_NOT_FOUND, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
        return WS_EV_RET_STOP;
    }

    pstCtrl->fp = fp;

    HTTP_SetStatusCode(pstTrans->hHttpHeadReply, HTTP_STATUS_OK);
    HTTP_SetContentLen(pstTrans->hHttpHeadReply, filesize);

    uiTime = TM_NowInSec();
    HTTP_DateToStr(uiTime, szStringTime);
    HTTP_SetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_DATE, szStringTime);

    (VOID) FILE_GetUtcTime(pstCtrl->szFilePath, NULL, &uiTime, NULL);
    HTTP_DateToStr(uiTime, szStringTime);
    HTTP_SetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_LAST_MODIFIED, szStringTime);
    HTTP_SetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_ETAG, WS_Context_GetDomainName(pstTrans->hContext));

    if (WS_VHost_GetContextCount(pstTrans->hVHost) > 1)
    {
        HTTP_SetRevalidate(pstTrans->hHttpHeadReply);
    }

    WS_Trans_SetHeadFieldFinish(pstTrans);

    return WS_EV_RET_BREAK;
}

static WS_EV_RET_E ws_plugstatic_Destory(IN WS_TRANS_S *pstTrans)
{
    _WS_PLUG_STATIC_S *pstCtrl;

    pstCtrl = pstTrans->apPlugContext[WS_PLUG_STATIC];
    if (NULL != pstCtrl)
    {
        if (pstCtrl->fp != NULL)
        {
            fclose(pstCtrl->fp);
        }
        MEM_Free(pstCtrl);
        pstTrans->apPlugContext[WS_PLUG_STATIC] = NULL;
    }

    return WS_EV_RET_CONTINUE;
}

WS_EV_RET_E _WS_PlugStatic_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    WS_EV_RET_E eRet = WS_EV_RET_CONTINUE;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_HEAD_OK:
        {
            eRet = ws_plugstatic_HeadOK(pstTrans);
            break;
        }

        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = ws_plugstatic_RecvBodyOK(pstTrans);
            break;
        }

        case WS_TRANS_EVENT_BUILD_BODY:
        {
            eRet = ws_plugstatic_BuildBody(pstTrans);
            break;
        }

        case WS_TRANS_EVENT_DESTORY:
        {
            eRet = ws_plugstatic_Destory(pstTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}

