/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-10-21
* Description: 在线代理
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/bit_opt.h"
#include "utl/lstr_utl.h"
#include "utl/socket_utl.h"
#include "utl/base64_utl.h"
#include "utl/fsm_utl.h"
#include "utl/dns_utl.h"
#include "utl/rand_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/nap_utl.h"
#include "utl/ws_utl.h"
#include "utl/ssl_utl.h"
#include "utl/html_up.h"
#include "utl/url_lib.h"
#include "utl/http_auth.h"
#include "utl/js_rw.h"
#include "utl/css_up.h"
#include "utl/web_proxy.h"

#define _WEB_PROXY_NAME "WebProxy"

#define _WEB_PROXY_CACHE_SIZE 1024  
#define _WEB_PROXY_WWW_AUTH_BASE64_LEN 511

#define _WEB_PROXY_PREFIX_BEFORE_PATH_MAX_LEN 511 

#define _WEB_RPOXY_MAX_TAG_NAME_LEN 15
#define _WEB_RPOXY_MAX_JS_RW_FILEPATH_LEN 63

#define _WEB_PROXY_AUTH_DIGEST_MAX_NAP_NUM 4096

typedef struct
{
    UINT uiAuthID;
    UINT uiRandom;  

    HTTP_AUTH_HANDLE hHttpAuth;
}_WEB_PROXY_AUTH_NODE_S;

typedef struct
{
    CHAR szTag[_WEB_RPOXY_MAX_TAG_NAME_LEN + 1];
    CHAR szJsRwFilePath[_WEB_RPOXY_MAX_JS_RW_FILEPATH_LEN + 1];
    USHORT usTagLen;
    USHORT usJsRwFilePathLen;
    UINT uiFlag;
    UINT uiDbgFlag;
    NAP_HANDLE hAuthNAP;
}_WEB_PROXY_CTRL_S;


#define _WEB_PROXY_MAX_PROTO_LEN 15
#define _WEB_PROXY_MAX_HOST_LEN  63

#define _WEB_PROXY_MAX_RES_HEAD_LEN 8192

typedef enum
{
    _WEB_PROXY_PROTO_HTTP = 0,
    _WEB_PROXY_PROTO_HTTPS,

    _WEB_PROXY_PROTO_MAX
}_WEB_PROXY_PROTO_E;

#define _WEB_PROXY_FLAG_EMBED_RW_JS         0x1 
#define _WEB_PROXY_FLAG_ERRTYPE_COMPATIBLE  0x2 
#define _WEB_PROXY_FLAG_DOWN_READ_OK_FAST   0x4 

typedef BS_STATUS (*PF_WEBPROXY_RwFunc)(IN VOID *pstNode, IN UCHAR *pucData, IN UINT uiDataLen);

typedef struct
{
    FSM_S stFsm;
    _WEB_PROXY_CTRL_S *pstCtrl;  
    MYPOLL_HANDLE hPoller;
    void *sslctx;
    VOID *pstSsl;
    _WEB_PROXY_PROTO_E enProto;
    CHAR szHost[_WEB_PROXY_MAX_HOST_LEN + 1];
    LSTR_S stPath;
    LSTR_S stPort;
    USHORT usPort;
    INT iUpSocketID;
    WS_TRANS_HANDLE hWsTrans;
    VBUF_S stVBuf;
    MBUF_S *pstSend2UpBody; 

    PF_WEBPROXY_RwFunc pfRwFunc;
    HTML_UP_HANDLE hHtmlUp;
    CSS_UP_HANDLE hCssUpHandle;
    JS_RW_HANDLE hJsRwHandle;

    UINT uiFlag;
}_WEB_PROXY_NODE_S;


typedef enum
{
    _WEB_PROXY_STATE_INIT = 0,
    _WEB_PROXY_STATE_CONNECTING,
    _WEB_PROXY_STATE_SSL_CONNECTING,
    _WEB_PROXY_STATE_REQ_HEAD,
    _WEB_PROXY_STATE_REQ_BODY,
    _WEB_PROXY_STATE_RES_HEAD,
    _WEB_PROXY_STATE_RES_BODY,
    _WEB_PROXY_STATE_RES_END
}_WEB_PROXY_STATE_E;

typedef enum
{
    _WEB_PROXY_EVENT_DOWN_READ = 0,
    _WEB_PROXY_EVENT_DOWN_READ_OK,
    _WEB_PROXY_EVENT_DOWN_WRITE,
    _WEB_PROXY_EVENT_DOWN_ERR,
    _WEB_PROXY_EVENT_UP_READ,
    _WEB_PROXY_EVENT_UP_WRITE,
    _WEB_PROXY_EVENT_UP_ERR,
}_WEB_PROXY_EVENT_E;

static FSM_STATE_MAP_S g_astWebProxyStateMap[] = 
{
    {"S.Init", _WEB_PROXY_STATE_INIT},
    {"S.Connecting", _WEB_PROXY_STATE_CONNECTING},
    {"S.SslConnecting", _WEB_PROXY_STATE_SSL_CONNECTING},
    {"S.ReqHead", _WEB_PROXY_STATE_REQ_HEAD},
    {"S.ReqBody", _WEB_PROXY_STATE_REQ_BODY},
    {"S.ResHead", _WEB_PROXY_STATE_RES_HEAD},
    {"S.ResBody", _WEB_PROXY_STATE_RES_BODY},
    {"S.End", _WEB_PROXY_STATE_RES_END},
};


static FSM_EVENT_MAP_S g_astWebProxyEventMap[] = 
{
    {"E.DownRead", _WEB_PROXY_EVENT_DOWN_READ},
    {"E.DownReadOK", _WEB_PROXY_EVENT_DOWN_READ_OK},
    {"E.DownWrite", _WEB_PROXY_EVENT_DOWN_WRITE},
    {"E.DownErr", _WEB_PROXY_EVENT_DOWN_ERR},
    {"E.UpRead", _WEB_PROXY_EVENT_UP_READ},
    {"E.UpWrite", _WEB_PROXY_EVENT_UP_WRITE},
    {"E.UpErr", _WEB_PROXY_EVENT_UP_ERR},
};


static VOID _webproxy_RecvBodyEnd(IN _WEB_PROXY_NODE_S *pstNode);
static BS_STATUS _webproxy_ConnectServer(IN _WEB_PROXY_NODE_S *pstNode);


static _WEB_PROXY_PROTO_E _webproxy_GetProto(IN LSTR_S *pstProto)
{
    if (0 == LSTR_StrCmp(pstProto, "http"))
    {
        return _WEB_PROXY_PROTO_HTTP;
    }

    if (0 == LSTR_StrCmp(pstProto, "https"))
    {
        return _WEB_PROXY_PROTO_HTTPS;
    }

    return _WEB_PROXY_PROTO_MAX;
}

static CHAR * _webproxy_GetProtoString(IN _WEB_PROXY_PROTO_E eProto)
{
    switch (eProto)
    {
        case _WEB_PROXY_PROTO_HTTP:
        {
            return "http";
        }

        case _WEB_PROXY_PROTO_HTTPS:
        {
            return "https";
        }

        default:
        {
            return "unknown";
        }
    }

    return "unknown";
}

static USHORT _webproxy_GetDftPort(IN _WEB_PROXY_PROTO_E enProto)
{
    USHORT usPort;

    switch (enProto)
    {
        case _WEB_PROXY_PROTO_HTTP:
            usPort = 80;
            break;
        case _WEB_PROXY_PROTO_HTTPS:
            usPort = 443;
            break;
        default:
            usPort = 0;
            break;
    }

    return usPort;
}

static CHAR * _webproxy_GetDftPortString(IN _WEB_PROXY_PROTO_E enProto)
{
    CHAR *pcPort = NULL;

    switch (enProto)
    {
        case _WEB_PROXY_PROTO_HTTP:
            pcPort = "80";
            break;
        case _WEB_PROXY_PROTO_HTTPS:
            pcPort = "443";
            break;
        default:
            pcPort = NULL;
            break;
    }

    return pcPort;
}

static CHAR * _webproxy_GetDftPortStringByProtoString(IN LSTR_S *pstProto)
{
    _WEB_PROXY_PROTO_E eProto;

    eProto = _webproxy_GetProto(pstProto);

    return _webproxy_GetDftPortString(eProto);
}


typedef VOID (*PF_WEBPROXY_RW_OUTPUT)(IN CHAR *pcData, IN ULONG ulDataLen, IN VOID *pUserContext);

static VOID _webproxy_RwRelativePageUrl
(
    IN _WEB_PROXY_NODE_S *pstNode,
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    IN PF_WEBPROXY_RW_OUTPUT pfOutput,
    IN VOID *pUserContext
)
{
    UINT uiPathDeep;
    UINT uiBackNum;

    if ((*pcUrl == '.') && (pcUrl[1] == '.'))
    {
        *pcUrl = '.';
    }

    uiPathDeep = FILE_GetPathDeep(pstNode->stPath.pcData, pstNode->stPath.uiLen);
    uiBackNum = URL_LIB_GetUrlBackNum(pcUrl, uiUrlLen);

    while (uiBackNum > uiPathDeep)
    {
        pcUrl += 3;
        uiUrlLen -= 3;
        uiBackNum --;
    }

    pfOutput(pcUrl, uiUrlLen, pUserContext);
}

static VOID _webproxy_RwRelativeRootUrl
(
    IN _WEB_PROXY_NODE_S *pstNode,
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    IN PF_WEBPROXY_RW_OUTPUT pfOutput,
    IN VOID *pUserContext
)
{
    CHAR *pcProto;
    CHAR szTmp[16];

    pcProto = _webproxy_GetProtoString(pstNode->enProto);
    sprintf(szTmp, "/%d/", pstNode->usPort);

    pfOutput("/", 1, pUserContext);
    pfOutput(pstNode->pstCtrl->szTag, pstNode->pstCtrl->usTagLen, pUserContext);
    pfOutput("/", 1, pUserContext);
    pfOutput(pcProto, strlen(pcProto), pUserContext);
    pfOutput(szTmp, strlen(szTmp), pUserContext);
    pfOutput(pstNode->szHost, strlen(pstNode->szHost), pUserContext);
    pfOutput(pcUrl, uiUrlLen, pUserContext);
}

static VOID _webproxy_RwAbsUrl
(
    IN _WEB_PROXY_CTRL_S *pstCtrl,
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    IN PF_WEBPROXY_RW_OUTPUT pfOutput,
    IN VOID *pUserContext
)
{
    LSTR_S stProtocol;
    LSTR_S stHost;
    LSTR_S stPath;
    LSTR_S stPort;
    if (BS_OK != ULR_LIB_ParseAbsUrl(pcUrl, uiUrlLen, &stProtocol, &stHost, &stPort, &stPath))
    {
        return;
    }

    if (stPort.pcData == NULL)
    {
        stPort.pcData = _webproxy_GetDftPortStringByProtoString(&stProtocol);
        if (NULL == stPort.pcData)
        {
            stPort.pcData = "0";
        }
        stPort.uiLen = strlen(stPort.pcData);
    }

    pfOutput("/", 1, pUserContext);
    pfOutput(pstCtrl->szTag, pstCtrl->usTagLen, pUserContext);
    pfOutput("/", 1, pUserContext);
    pfOutput(stProtocol.pcData, stProtocol.uiLen, pUserContext);
    pfOutput("/", 1, pUserContext);
    pfOutput(stPort.pcData, stPort.uiLen, pUserContext);
    pfOutput("/", 1, pUserContext);
    pfOutput(stHost.pcData, stHost.uiLen, pUserContext);
    pfOutput(stPath.pcData, stPath.uiLen, pUserContext);
}

static VOID _webproxy_RwSimpleAbsUrl
(
    IN _WEB_PROXY_CTRL_S *pstCtrl,
    IN _WEB_PROXY_NODE_S *pstNode,
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    IN PF_WEBPROXY_RW_OUTPUT pfOutput,
    IN VOID *pUserContext
)
{
    LSTR_S stHost;
    LSTR_S stPath;
    LSTR_S stPort;
    CHAR *pcProto;

    if (BS_OK != ULR_LIB_ParseSimpleAbsUrl(pcUrl, uiUrlLen, &stHost, &stPort, &stPath))
    {
        return;
    }

    if (stPort.pcData == NULL)
    {
        stPort.pcData = _webproxy_GetDftPortString(pstNode->enProto);
        if (NULL == stPort.pcData)
        {
            stPort.pcData = "0";
        }
        stPort.uiLen = strlen(stPort.pcData);
    }

    pcProto = _webproxy_GetProtoString(pstNode->enProto);

    pfOutput("/", 1, pUserContext);
    pfOutput(pstCtrl->szTag, pstCtrl->usTagLen, pUserContext);
    pfOutput("/", 1, pUserContext);
    pfOutput(pcProto, strlen(pcProto), pUserContext);
    pfOutput("/", 1, pUserContext);
    pfOutput(stPort.pcData, stPort.uiLen, pUserContext);
    pfOutput("/", 1, pUserContext);
    pfOutput(stHost.pcData, stHost.uiLen, pUserContext);
    pfOutput(stPath.pcData, stPath.uiLen, pUserContext);
}



static VOID _webproxy_UrlRw
(
    IN _WEB_PROXY_NODE_S *pstNode,
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    IN PF_WEBPROXY_RW_OUTPUT pfOutput,
    IN VOID *pUserContext
)
{
    URL_LIB_URL_TYPE_E eType;
    BOOL_T bQuted = FALSE; 
    CHAR cFirst;

    cFirst = *pcUrl;

    if ((cFirst == '\'') || (cFirst == '\"'))
    {
        pfOutput(pcUrl, 1, pUserContext);
        pcUrl ++;
        uiUrlLen -= 2;
        bQuted = TRUE;
    }

    eType = URL_LIB_GetUrlType(pcUrl, uiUrlLen);
    switch (eType)
    {
        case URL_TYPE_ABSOLUTE:
        {
            _webproxy_RwAbsUrl(pstNode->pstCtrl, pcUrl, uiUrlLen, pfOutput, pUserContext);
            break;
        }

        case URL_TYPE_ABSOLUTE_SIMPLE:
        {
            _webproxy_RwSimpleAbsUrl(pstNode->pstCtrl, pstNode, pcUrl, uiUrlLen, pfOutput, pUserContext);
            break;
        }

        case URL_TYPE_RELATIVE_ROOT:
        {
            _webproxy_RwRelativeRootUrl(pstNode, pcUrl, uiUrlLen, pfOutput, pUserContext);
            break;
        }

        case URL_TYPE_RELATIVE_PAGE:
        {
            _webproxy_RwRelativePageUrl(pstNode, pcUrl, uiUrlLen, pfOutput, pUserContext);
            break;
        }

        default:
        {
            pfOutput(pcUrl, uiUrlLen, pUserContext);
            break;
        }
    }

    if (bQuted)
    {
        pfOutput(&cFirst, 1, pUserContext);
    }

    return;
}

static INT _webproxy_UpWrite(IN _WEB_PROXY_NODE_S *pstNode, IN UCHAR *pucData, IN UINT uiLen)
{
    if (NULL == pstNode->pstSsl)
    {
        return Socket_Write(pstNode->iUpSocketID, pucData, uiLen, 0);
    }

    return SSL_UTL_Write(pstNode->pstSsl, pucData, uiLen);
}

static INT _webproxy_UpRead(IN _WEB_PROXY_NODE_S *pstNode, OUT UCHAR *pucBuf, IN UINT uiBufLen)
{
    if (NULL == pstNode->pstSsl)
    {
        return Socket_Read(pstNode->iUpSocketID, pucBuf, uiBufLen, 0);
    }

    return SSL_UTL_Read(pstNode->pstSsl, pucBuf, uiBufLen);
}

static BS_STATUS _webproxy_SendReqHead(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    UCHAR *pucData;
    UINT uiLen;
    INT iSendLen;
    BS_STATUS eRet;

    pucData = VBUF_GetData(&pstNode->stVBuf);
    uiLen = VBUF_GetDataLength(&pstNode->stVBuf);

    iSendLen = _webproxy_UpWrite(pstNode, pucData, uiLen);
    if (iSendLen == SOCKET_E_AGAIN)
    {
        return BS_OK;
    }

    if (iSendLen < 0)
    {
        return BS_ERR;
    }

    VBUF_EarseHead(&pstNode->stVBuf, iSendLen);
    if (VBUF_GetDataLength(&pstNode->stVBuf) > 0)
    {
        return BS_OK;
    }

    FSM_SetState(pstFsm, _WEB_PROXY_STATE_REQ_BODY);

    WS_Trans_Continue(pstNode->hWsTrans);

    if (pstNode->uiFlag & _WEB_PROXY_FLAG_DOWN_READ_OK_FAST)
    {
        eRet = FSM_EventHandle(&pstNode->stFsm, _WEB_PROXY_EVENT_DOWN_READ_OK);
        if (BS_OK != eRet)
        {
            return BS_ERR;
        }
    }

    return BS_OK;
}

static BOOL_T _webproxy_IsIgnoreReqHeader(IN CHAR *pcHeadField)
{
    UINT i;
    static CHAR * apcIgnoreHeader[] =
    {
        HTTP_FIELD_HOST,
        HTTP_FIELD_REFERER,
        HTTP_FIELD_ACCEPT_ENCODING,
        HTTP_FIELD_CONNECTION
    };

    for (i=0; i<sizeof(apcIgnoreHeader)/sizeof(CHAR*); i++)
    {
        if (stricmp(apcIgnoreHeader[i], pcHeadField) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static BS_STATUS _webproxy_RecvUpBodyErr(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    _webproxy_RecvBodyEnd(pstNode);

    return BS_STOP;
}

static inline VOID _webproxy_SendString2Down(IN _WEB_PROXY_NODE_S *pstNode, IN CHAR *pcString)
{
    WS_Trans_AddReplyBodyByBuf(pstNode->hWsTrans, (void*)pcString, strlen(pcString));
}

static VOID _webproxy_UpErr(IN _WEB_PROXY_NODE_S *pstNode, IN CHAR *pcErrInfo)
{
    HTTP_HEAD_PARSER hEncap;

    MyPoll_Del(pstNode->hPoller, pstNode->iUpSocketID);
    if (NULL != pstNode->pstSsl)
    {
        SSL_UTL_Free(pstNode->pstSsl);
        pstNode->pstSsl = NULL;
    }
    Socket_Close(pstNode->iUpSocketID);
    pstNode->iUpSocketID = -1;

    hEncap = WS_Trans_GetHttpEncap(pstNode->hWsTrans);

    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(pstNode->hWsTrans);
    if (NULL != pcErrInfo)
    {
        WS_Trans_AddReplyBodyByBuf(pstNode->hWsTrans, (void*)pcErrInfo, strlen(pcErrInfo));
    }
    WS_Trans_ReplyBodyFinish(pstNode->hWsTrans);
}

static BS_STATUS _webproxy_SendReqUpErr(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    _webproxy_UpErr(pstNode, "Can't send request to server.");

    return BS_STOP;
}

static BS_STATUS _webproxy_RecvUpHeadErr(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    _webproxy_UpErr(pstNode, "Can't receive data from server.");

    return BS_STOP;
}

static BS_STATUS _webproxy_ConnectErr(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    _webproxy_SendString2Down(pstNode, "Can't connect ");
    _webproxy_SendString2Down(pstNode, pstNode->szHost);
    _webproxy_UpErr(pstNode, NULL);
    WS_Trans_Continue(pstNode->hWsTrans);

    return BS_STOP;
}

static BS_STATUS _webproxy_SslConnectErr(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    _webproxy_SendString2Down(pstNode, "SSL: ");

    return _webproxy_ConnectErr(pstFsm, uiEvent);
}

#define _WEB_PROXY_MAX_HEAD_FIELD_LEN 1023

static BS_STATUS _webproxy_SetReqHeadField
(
    IN _WEB_PROXY_NODE_S *pstNode,
    IN HTTP_HEAD_PARSER hEncap,
    IN CHAR *pcField,
    IN CHAR *pcValue
)
{
    return HTTP_SetHeadField(hEncap, pcField, pcValue);
}

static inline _WEB_PROXY_AUTH_NODE_S * _webproxy_GetAuthNodeByID(IN NAP_HANDLE hAuthNAP, IN UINT uiAuthID)
{
    return NAP_GetNodeByID(hAuthNAP, uiAuthID);
}

static _WEB_PROXY_AUTH_NODE_S * _webproxy_GetAuthNodeByCookie
(
    IN _WEB_PROXY_CTRL_S *pstCtrl,
    IN CHAR *pcCookie
)
{
    CHAR szRandom[12];
    CHAR szAuthID[12];
    UINT uiAuthID;
    UINT uiRandom;
    _WEB_PROXY_AUTH_NODE_S * pstAuthNode;

    if (strlen(pcCookie) < 16)
    {
        return NULL;
    }

    MEM_Copy(szRandom, pcCookie, 8);
    szRandom[8] = '\0';
    MEM_Copy(szAuthID, pcCookie + 8, 8);
    szAuthID[8] = '\0';

    TXT_XAtoui(szAuthID, &uiAuthID);
    TXT_XAtoui(szRandom, &uiRandom);

    pstAuthNode = _webproxy_GetAuthNodeByID(pstCtrl->hAuthNAP, uiAuthID);
    if (NULL == pstAuthNode)
    {
        return NULL;
    }

    if (uiRandom != pstAuthNode->uiRandom)
    {
        return NULL;
    }

    return pstAuthNode;
}

static _WEB_PROXY_AUTH_NODE_S * _webproxy_FindAuthNode(IN _WEB_PROXY_NODE_S *pstNode)
{
    HTTP_HEAD_PARSER hReqParser;
    CHAR *pcCookie;
    MIME_HANDLE hMime;
    CHAR *pcAuthCookie;
    _WEB_PROXY_AUTH_NODE_S *pstAuthNode;

    hReqParser = WS_Trans_GetHttpRequestParser(pstNode->hWsTrans);

    pcCookie = HTTP_GetHeadField(hReqParser, HTTP_FIELD_COOKIE);
    if (NULL == pcCookie)
    {
        return NULL;
    }

    hMime = MIME_Create();
    if (NULL == hMime)
    {
        return NULL;
    }

    MIME_ParseCookie(hMime, pcCookie);

    pcAuthCookie = MIME_GetKeyValue(hMime, "_proxy_auth_cookie");
    if (NULL == pcAuthCookie)
    {
        MIME_Destroy(hMime);
        return NULL;
    }

    pstAuthNode = _webproxy_GetAuthNodeByCookie(pstNode->pstCtrl, pcAuthCookie);

    MIME_Destroy(hMime);

    return pstAuthNode;
}

static _WEB_PROXY_AUTH_NODE_S * _webproxy_FindOrCreateAuthNode(IN _WEB_PROXY_NODE_S *pstNode)
{
    _WEB_PROXY_AUTH_NODE_S *pstAuthNode;
    NAP_HANDLE hAuthNAP;

    pstAuthNode = _webproxy_FindAuthNode(pstNode);
    if (NULL != pstAuthNode)
    {
        return pstAuthNode;
    }
    
    hAuthNAP = pstNode->pstCtrl->hAuthNAP;

    pstAuthNode = NAP_ZAlloc(hAuthNAP);
    if (NULL == pstAuthNode)
    {
        return NULL;
    }

    pstAuthNode->hHttpAuth = HTTP_Auth_ClientCreate();
    if (NULL == pstAuthNode->hHttpAuth)
    {
        NAP_Free(hAuthNAP, pstAuthNode);
        return NULL;
    }

    return pstAuthNode;
}

static VOID _webproxy_FreeAuthNode(IN _WEB_PROXY_NODE_S *pstNode, IN _WEB_PROXY_AUTH_NODE_S *pstAuthNode)
{
    HTTP_Auth_ClientDestroy(pstAuthNode->hHttpAuth);
    NAP_Free(pstNode->pstCtrl->hAuthNAP, pstAuthNode);
}

static _WEB_PROXY_AUTH_NODE_S * _webproxy_CreateAuthNode
(
    IN _WEB_PROXY_NODE_S *pstNode,
    IN CHAR *pcWwwAuthenticate
)
{
    _WEB_PROXY_AUTH_NODE_S *pstAuthNode;

    pstAuthNode = _webproxy_FindOrCreateAuthNode(pstNode);
    if (NULL == pstAuthNode)
    {
        return NULL;
    }

    if (BS_OK != HTTP_Auth_ClientSetAuthContext(pstAuthNode->hHttpAuth, pcWwwAuthenticate))
    {
        _webproxy_FreeAuthNode(pstNode, pstAuthNode);
        return NULL;
    }

    pstAuthNode->uiAuthID = (UINT)NAP_GetIDByNode(pstNode->pstCtrl->hAuthNAP, pstAuthNode);
    pstAuthNode->uiRandom = RAND_Get();

    return pstAuthNode;
}

static VOID _webproxy_TryBuildAuthInfo(IN _WEB_PROXY_NODE_S *pstNode, IN HTTP_HEAD_PARSER hEncap)
{
    HTTP_HEAD_PARSER hReqParser;
    _WEB_PROXY_AUTH_NODE_S *pstAuthNode;
    CHAR szDigest[HTTP_AUTH_DIGEST_LEN + 1];
    
    hReqParser = WS_Trans_GetHttpRequestParser(pstNode->hWsTrans);

    pstAuthNode = _webproxy_FindAuthNode(pstNode);
    if (NULL == pstAuthNode)
    {
        return;
    }

    if (BS_OK != HTTP_AUTH_ClientDigestBuild(pstAuthNode->hHttpAuth, 
        HTTP_GetMethodData(hReqParser), pstNode->stPath.pcData, szDigest))
    {
        return;
    }

    HTTP_SetHeadField(hEncap, HTTP_FIELD_AUTHORIZATION, szDigest);

    return;
}

static BS_STATUS _webproxy_BuildReqHead(IN _WEB_PROXY_NODE_S *pstNode)
{
    HTTP_HEAD_PARSER hReqParser;
    HTTP_HEAD_PARSER hEncap;
    HTTP_HEAD_FIELD_S *pstHeadField = NULL;
    ULONG ulHeadLen;
    CHAR *pcHead;
    CHAR *pcStr;

    hReqParser = WS_Trans_GetHttpRequestParser(pstNode->hWsTrans);

    hEncap = HTTP_CreateHeadParser();
    if (NULL == hEncap)
    {
        return BS_ERR;
    }

    HTTP_SetMethod(hEncap, HTTP_GetMethod(hReqParser));
    HTTP_SetVersion(hEncap, HTTP_VERSION_1_0);
    HTTP_SetUriPath(hEncap, pstNode->stPath.pcData, pstNode->stPath.uiLen);
    pcStr = HTTP_GetUriQuery(hReqParser);
    if (NULL != pcStr)
    {
        HTTP_SetUriQuery(hEncap, pcStr);
    }

    _webproxy_TryBuildAuthInfo(pstNode, hEncap);
    
    while (NULL != (pstHeadField = HTTP_GetNextHeadField(hReqParser, pstHeadField)))
    {
        if (FALSE == _webproxy_IsIgnoreReqHeader(pstHeadField->pcFieldName))
        {
            _webproxy_SetReqHeadField(pstNode, hEncap, pstHeadField->pcFieldName, pstHeadField->pcFieldValue);
        }
    }
    HTTP_SetHeadField(hEncap, HTTP_FIELD_HOST, pstNode->szHost);
    HTTP_SetHeadField(hEncap, HTTP_FIELD_CONNECTION, "close");

    pcHead = HTTP_BuildHttpHead(hEncap, HTTP_REQUEST, &ulHeadLen);

    if (NULL == pcHead)
    {
        HTTP_DestoryHeadParser(hEncap);
        return BS_ERR;
    }

    BS_DBG_OUTPUT(pstNode->pstCtrl->uiDbgFlag, WEB_PROXY_DBG_FLAG_PACKET,
        ("--------------\r\nBuild request header:\r\n%s\r\n--------------\r\n", pcHead));

    VBUF_CpyBuf(&pstNode->stVBuf, pcHead, ulHeadLen);
    HTTP_DestoryHeadParser(hEncap);

    return BS_OK;
}

static BS_STATUS _webproxy_BuildReqHeadAndSend(IN _WEB_PROXY_NODE_S *pstNode)
{
    MyPoll_ClearEvent(pstNode->hPoller, pstNode->iUpSocketID);

    if (BS_OK != _webproxy_BuildReqHead(pstNode))
    {
        return BS_ERR;
    }

    FSM_SetState(&pstNode->stFsm, _WEB_PROXY_STATE_REQ_HEAD);

    return _webproxy_SendReqHead(&pstNode->stFsm, MYPOLL_EVENT_OUT);
}

static BS_STATUS _webproxy_CreateSsl(IN _WEB_PROXY_NODE_S *pstNode)
{
    VOID *pstSsl;

    pstSsl = SSL_UTL_New(pstNode->sslctx);
    if (NULL == pstSsl)
    {
        return BS_ERR;
    }

    if (NULL != pstNode->pstSsl)
    {
        SSL_UTL_Free(pstNode->pstSsl);
    }

    pstNode->pstSsl = pstSsl;
    SSL_UTL_SetFd(pstSsl, pstNode->iUpSocketID);
    SSL_UTL_SetHostName(pstSsl, pstNode->szHost);

    return BS_OK;
}

static BS_STATUS _webproxy_Connected(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    if (pstNode->enProto == _WEB_PROXY_PROTO_HTTPS)
    {
        if (BS_OK != _webproxy_CreateSsl(pstNode))
        {
            return BS_ERR;
        }

        FSM_SetState(pstFsm, _WEB_PROXY_STATE_SSL_CONNECTING);

        return BS_OK;
    }

    return _webproxy_BuildReqHeadAndSend(pstNode);
}

static BS_STATUS _webproxy_DownReadOKFast(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    pstNode->uiFlag |= _WEB_PROXY_FLAG_DOWN_READ_OK_FAST;

    return BS_OK;
}

static BS_STATUS _webproxy_SslConnecting(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    INT iRet;
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    iRet = SSL_UTL_Connect(pstNode->pstSsl);

    if (iRet < 0)
    {
        if (iRet == SSL_UTL_E_WANT_READ)
        {
            MyPoll_ModifyEvent(pstNode->hPoller, pstNode->iUpSocketID, MYPOLL_EVENT_IN);
            return BS_OK;
        }

        if (iRet == SSL_UTL_E_WANT_WRITE)
        {
            MyPoll_ModifyEvent(pstNode->hPoller, pstNode->iUpSocketID, MYPOLL_EVENT_OUT);
            return BS_OK;
        }

        return FSM_EventHandle(&pstNode->stFsm, _WEB_PROXY_EVENT_UP_ERR);
    }

    return _webproxy_BuildReqHeadAndSend(pstNode);
}

static BS_STATUS _webproxy_SendBody2Up(IN _WEB_PROXY_NODE_S *pstNode, IN MBUF_S *pstMbuf)
{
    INT iSendLen;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf)))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    iSendLen = _webproxy_UpWrite(pstNode, MBUF_MTOD(pstMbuf), MBUF_TOTAL_DATA_LEN(pstMbuf));
    if (iSendLen == SOCKET_E_AGAIN)
    {
        pstNode->pstSend2UpBody = pstMbuf;
        WS_Trans_Pause(pstNode->hWsTrans);
        MyPoll_AddEvent(pstNode->hPoller, pstNode->iUpSocketID, MYPOLL_EVENT_OUT);
        return BS_OK;
    }

    if (iSendLen < 0)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    if ((UINT)iSendLen < MBUF_TOTAL_DATA_LEN(pstMbuf))
    {
        MBUF_CutHead(pstMbuf, iSendLen);
        pstNode->pstSend2UpBody = pstMbuf;
        WS_Trans_Pause(pstNode->hWsTrans);
        MyPoll_AddEvent(pstNode->hPoller, pstNode->iUpSocketID, MYPOLL_EVENT_OUT);
        return BS_OK;
    }

    MBUF_Free(pstMbuf);
    MyPoll_DelEvent(pstNode->hPoller, pstNode->iUpSocketID, MYPOLL_EVENT_OUT);
    WS_Trans_Continue(pstNode->hWsTrans);

    return BS_OK;
}

static BS_STATUS _webproxy_SendReqBody(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    MBUF_S *pstMbuf;

    if (NULL != pstNode->pstSend2UpBody)
    {
        pstMbuf = pstNode->pstSend2UpBody;
        pstNode->pstSend2UpBody = NULL;
    }
    else
    {
        pstMbuf = WS_Trans_GetBodyData(pstNode->hWsTrans);
    }

    if (NULL == pstMbuf)
    {
        return BS_OK;
    }

    return _webproxy_SendBody2Up(pstNode, pstMbuf);
}

static BS_STATUS _webproxy_DownReadOK(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    MyPoll_ModifyEvent(pstNode->hPoller, pstNode->iUpSocketID, MYPOLL_EVENT_IN);

    return BS_OK;
}

static BOOL_T _webproxy_IsIgnoreReplyHeader(IN CHAR *pcHeadField)
{
    UINT i;
    static CHAR * apcIgnoreHeader[] =
    {
        HTTP_FIELD_CONNECTION,
        HTTP_FIELD_CONTENT_LENGTH
    };

    for (i=0; i<sizeof(apcIgnoreHeader)/sizeof(CHAR*); i++)
    {
        if (stricmp(apcIgnoreHeader[i], pcHeadField) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}


static BOOL_T _webproxy_NeedRwReplyHeader(IN CHAR *pcHeadField)
{
    UINT i;
    static CHAR * apcIgnoreHeader[] =
    {
        HTTP_FIELD_LOCATION,
        HTTP_FIELD_CONTENT_LOCATION
    };

    for (i=0; i<sizeof(apcIgnoreHeader)/sizeof(CHAR*); i++)
    {
        if (stricmp(apcIgnoreHeader[i], pcHeadField) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

#define _WEBPROXY_MAX_RW_HEAD_FILED_LEN 1023

typedef struct
{
    CHAR acValue[_WEBPROXY_MAX_RW_HEAD_FILED_LEN + 1];
    UINT uiLen;
}_WEBPROXY_RW_HEAD_FIELD_S;

static VOID _webproxy_RwReplyHeadFiledCallBack(IN CHAR *pcData, IN ULONG ulDataLen, IN VOID *pUserContext)
{
    _WEBPROXY_RW_HEAD_FIELD_S *pstValue = pUserContext;
    UINT uiRwLen;

    uiRwLen = MIN(ulDataLen, (_WEBPROXY_MAX_RW_HEAD_FILED_LEN - pstValue->uiLen));
    
    if (uiRwLen == 0)
    {
        return;
    }

    MEM_Copy(pstValue->acValue + pstValue->uiLen, pcData, uiRwLen);
    pstValue->uiLen += uiRwLen;
    pstValue->acValue[pstValue->uiLen] = '\0';

}

static VOID _webproxy_SetReplyHeadField
(
    IN _WEB_PROXY_NODE_S *pstNode,
    IN HTTP_HEAD_PARSER hEncap,
    IN CHAR *pcFieldName,
    IN CHAR *pcFieldValue
)
{
    _WEBPROXY_RW_HEAD_FIELD_S stValue;
    CHAR *pcTmp = pcFieldValue;

    stValue.uiLen = 0;

    if (TRUE == _webproxy_NeedRwReplyHeader(pcFieldName))
    {
        _webproxy_UrlRw(pstNode, pcFieldValue, strlen(pcFieldValue),
            _webproxy_RwReplyHeadFiledCallBack, &stValue);
        pcTmp = stValue.acValue;
    }

    HTTP_SetHeadField(hEncap, pcFieldName, pcTmp);
}

static BS_STATUS _webproxy_SendBody2Down(IN _WEB_PROXY_NODE_S *pstNode, IN void *body, IN UINT uiDataLen)
{
    BS_STATUS eRet;

    if (uiDataLen == 0)
    {
        return BS_OK;
    }

    eRet = WS_Trans_AddReplyBodyByBuf(pstNode->hWsTrans, body, uiDataLen);
    if (eRet != BS_OK)
    {
        return eRet;
    }

    MyPoll_DelEvent(pstNode->hPoller, pstNode->iUpSocketID, MYPOLL_EVENT_IN);

    return BS_OK;
}

static VOID _webproxy_RwOutput(IN CHAR *pcData, IN ULONG ulDataLen, IN VOID *pUserContext)
{
    _webproxy_SendBody2Down(pUserContext, pcData, ulDataLen);
}

#define _WEB_PROXY_EMBED_RW_JS_HEADLER "<script type=\"text/javascript\" src=\""
#define _WEB_PROXY_EMBED_RW_JS_HEADLER_LEN STR_LEN(_WEB_PROXY_EMBED_RW_JS_HEADLER)
#define _WEB_PROXY_EMBED_RW_JS_TAILER  "\"></script>\r\n"
#define _WEB_PROXY_EMBED_RW_JS_TAILER_LEN STR_LEN(_WEB_PROXY_EMBED_RW_JS_TAILER)

static VOID _webproxy_EmbedRwJs(IN _WEB_PROXY_NODE_S *pstNode)
{
    if (pstNode->uiFlag & _WEB_PROXY_FLAG_EMBED_RW_JS)
    {
        return;
    }

    pstNode->uiFlag |= _WEB_PROXY_FLAG_EMBED_RW_JS;

    _webproxy_SendBody2Down(pstNode, _WEB_PROXY_EMBED_RW_JS_HEADLER, _WEB_PROXY_EMBED_RW_JS_HEADLER_LEN);
    _webproxy_SendBody2Down(pstNode, pstNode->pstCtrl->szJsRwFilePath, pstNode->pstCtrl->usJsRwFilePathLen);
    _webproxy_SendBody2Down(pstNode, _WEB_PROXY_EMBED_RW_JS_TAILER, _WEB_PROXY_EMBED_RW_JS_TAILER_LEN);
}

static VOID _webproxy_CssUpCallBack
(
    IN CSS_UP_TYPE_E eType,
    IN CHAR *pcData,
    IN ULONG ulDataLen,
    IN VOID *pUserContext
)
{
    _WEB_PROXY_NODE_S *pstNode = pUserContext;

    if (eType == CSS_UP_TYPE_URL)
    {
        _webproxy_UrlRw(pstNode, pcData, ulDataLen, _webproxy_RwOutput, pstNode);
    }
    else
    {
        _webproxy_SendBody2Down(pstNode, pcData, ulDataLen);
    }
}

static VOID _webproxy_HtmlUpCallBack(IN HTML_UP_PARAM_S *pstParam, IN VOID *pUserContext)
{
    _WEB_PROXY_NODE_S *pstNode = pUserContext;

    if (pstParam->enType == HTML_UP_URL)
    {
        _webproxy_UrlRw(pstNode, pstParam->pcData, pstParam->uiDataLen, _webproxy_RwOutput, pstNode);
    }
    else if (pstParam->enType == HTML_UP_TAG_START)
    {
        _webproxy_EmbedRwJs(pstNode);
    }
    else if (pstParam->enType == HTML_UP_JS_START)
    {
        BS_DBGASSERT(pstNode->hJsRwHandle == NULL);
        pstNode->hJsRwHandle = JS_RW_Create(_webproxy_RwOutput, pstNode);
    }
    else if (pstParam->enType == HTML_UP_JS)
    {
        if (NULL == pstNode->hJsRwHandle)
        {
            _webproxy_SendBody2Down(pstNode, pstParam->pcData, pstParam->uiDataLen);
        }
        else
        {
            JS_RW_Run(pstNode->hJsRwHandle, pstParam->pcData, pstParam->uiDataLen);
        }
    }
    else if (pstParam->enType == HTML_UP_JS_END)
    {
        if (NULL != pstNode->hJsRwHandle)
        {
            JS_RW_End(pstNode->hJsRwHandle);
            JS_RW_Destroy(pstNode->hJsRwHandle);
            pstNode->hJsRwHandle = NULL;
        }
    }
    else if (pstParam->enType == HTML_UP_CSS_START)
    {
        BS_DBGASSERT(pstNode->hCssUpHandle == NULL);
        pstNode->hCssUpHandle = CSS_UP_Create(_webproxy_CssUpCallBack, pstNode);
    }
    else if (pstParam->enType == HTML_UP_CSS)
    {
        if (NULL == pstNode->hCssUpHandle)
        {
            _webproxy_SendBody2Down(pstNode, pstParam->pcData, pstParam->uiDataLen);
        }
        else
        {
            CSS_UP_Run(pstNode->hCssUpHandle, pstParam->pcData, pstParam->uiDataLen);
        }
    }
    else if (pstParam->enType == HTML_UP_CSS_END)
    {
        if (NULL != pstNode->hCssUpHandle)
        {
            CSS_UP_End(pstNode->hCssUpHandle);
            CSS_UP_Destroy(pstNode->hCssUpHandle);
            pstNode->hCssUpHandle = NULL;
        }
    }
    else
    {
        _webproxy_SendBody2Down(pstNode, pstParam->pcData, pstParam->uiDataLen);
    }
}

static BS_STATUS _webproxy_RwHtml(IN VOID *pNode, IN UCHAR *pucData, IN UINT uiDataLen)
{
    _WEB_PROXY_NODE_S *pstNode = pNode;
    if (NULL == pstNode->hHtmlUp)
    {
        pstNode->hHtmlUp = HTML_UP_Create(_webproxy_HtmlUpCallBack, pstNode);
        if (NULL == pstNode->hHtmlUp)
        {
            return BS_NO_MEMORY;
        }
    }

    HTML_UP_InputHtml(pstNode->hHtmlUp, (CHAR*)pucData, uiDataLen);

    return BS_OK;
}

static BS_STATUS _webproxy_RwJs(IN VOID *pNode, IN UCHAR *pucData, IN UINT uiDataLen)
{
    _WEB_PROXY_NODE_S *pstNode = pNode;

    if (NULL == pstNode->hJsRwHandle)
    {
        pstNode->hJsRwHandle = JS_RW_Create(_webproxy_RwOutput, pstNode);
        if (NULL == pstNode->hJsRwHandle)
        {
            return BS_NO_MEMORY;
        }
    }

    JS_RW_Run(pstNode->hJsRwHandle, (CHAR*)pucData, uiDataLen);

    return BS_OK;
}

static BS_STATUS _webproxy_RwCss(IN VOID *pNode, IN UCHAR *pucData, IN UINT uiDataLen)
{
    _WEB_PROXY_NODE_S *pstNode = pNode;
    
    if (NULL == pstNode->hCssUpHandle)
    {
        pstNode->hCssUpHandle = CSS_UP_Create(_webproxy_CssUpCallBack, pstNode);
        if (NULL == pstNode->hCssUpHandle)
        {
            return BS_NO_MEMORY;
        }
    }

    CSS_UP_Run(pstNode->hCssUpHandle, (CHAR*)pucData, uiDataLen);

    return BS_OK;
}

typedef struct
{
    CHAR *pcType;
    PF_WEBPROXY_RwFunc pfRwFunc;
}_WEBPROXY_RW_TYPE_S;

STATIC _WEBPROXY_RW_TYPE_S g_astWebproxyRwContentType[] =
{
    {"text/html", _webproxy_RwHtml},
    {"text/javascript", _webproxy_RwJs},
    {"application/x-javascript", _webproxy_RwJs},
    {"application/javascript", _webproxy_RwJs},
    {"text/css", _webproxy_RwCss},
};

STATIC _WEBPROXY_RW_TYPE_S g_astWebproxyRwFileExt[] =
{
    {"js", _webproxy_RwJs},
    {"css", _webproxy_RwCss},
};

static VOID _webproxy_InitRw(IN _WEB_PROXY_NODE_S *pstNode, IN CHAR *pcContentType)
{
    UINT i;
    LSTR_S stExt;

    if (BS_OK == LSTR_GetExt(&pstNode->stPath, &stExt))
    {
        for (i=0; i<sizeof(g_astWebproxyRwFileExt)/sizeof(_WEBPROXY_RW_TYPE_S); i++)
        {
             if ((stExt.uiLen == strlen(g_astWebproxyRwFileExt[i].pcType))
                && (strnicmp(stExt.pcData, g_astWebproxyRwFileExt[i].pcType, stExt.uiLen) == 0))
             {
                pstNode->pfRwFunc = g_astWebproxyRwFileExt[i].pfRwFunc;
                break;
             }
        }
    }

    if (pstNode->pfRwFunc == NULL)
    {
        for (i=0; i<sizeof(g_astWebproxyRwContentType)/sizeof(_WEBPROXY_RW_TYPE_S); i++)
        {
             if (strstr(pcContentType, g_astWebproxyRwContentType[i].pcType) != NULL)
             {
                pstNode->pfRwFunc = g_astWebproxyRwContentType[i].pfRwFunc;
                break;
             }
        }
    }
}

static BS_STATUS _webproxy_ProcUnauthorized
(
    IN _WEB_PROXY_NODE_S *pstNode,
    IN HTTP_HEAD_PARSER hReplyParser,
    IN HTTP_HEAD_PARSER hEncap
)
{
    static CHAR *pcNeedAuth =
        "<html>\r\n"
        "<body>\r\n"
        "<div align=\"center\">\r\n"
        "<form method=POST action=\"/_proxy/auth_digest.cgi\">\r\n"
        "<input type=\"text\" style=\"display:none;\" name=\"_proxy_auth_cookie\" value=\"%s\">\r\n"
        "<input type=\"text\" style=\"display:none;\" name=\"redirect_uri\" value=\"%s\">\r\n"
        "UserName: <input type=\"text\" name=\"user_name\"><br>\r\n"
        "PassWord: <input type=\"password\" name=\"user_password\"><br>\r\n"
        "<input type=\"submit\" value=\"Login\" />\r\n"
        "</from>\r\n"
        "</div>\r\n"
        "</body>\r\n"
        "</html>";
    CHAR szTmp[1024];
    CHAR *pcRequestFile;
    CHAR *pcWWWAuth;
    CHAR szAuthCookie[20];
    _WEB_PROXY_AUTH_NODE_S *pstAuthNode;

    pcRequestFile = WS_Trans_GetRequestFile(pstNode->hWsTrans);
    pcWWWAuth = HTTP_GetHeadField(hReplyParser, HTTP_FIELD_WWW_AUTHENTICATE);
    if ((NULL == pcRequestFile) || (NULL == pcWWWAuth))
    {
        return BS_ERR;
    }

    pstAuthNode = _webproxy_CreateAuthNode(pstNode, pcWWWAuth);
    if (NULL == pstAuthNode)
    {
        return BS_ERR;
    }

    sprintf(szAuthCookie, "%08x%08x", pstAuthNode->uiRandom, pstAuthNode->uiAuthID);

    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(pstNode->hWsTrans);

    snprintf(szTmp, sizeof(szTmp), pcNeedAuth, szAuthCookie, pcRequestFile);

    WS_Trans_AddReplyBodyByBuf(pstNode->hWsTrans, szTmp, strlen(szTmp));

    return BS_OK;
}

static BOOL_T _webproxy_IsDigestUnauthorized(IN HTTP_HEAD_PARSER hReplyParser)
{
    CHAR * pcAuthInfo;

    if (HTTP_STATUS_UAUTHO != HTTP_GetStatusCode(hReplyParser))
    {
        return FALSE;
    }

    pcAuthInfo = HTTP_GetHeadField(hReplyParser, HTTP_FIELD_WWW_AUTHENTICATE);
    if (NULL == pcAuthInfo)
    {
        return FALSE;
    }

    return HTTP_AUTH_IsDigestUnauthorized(pcAuthInfo);
}

static BS_STATUS _webproxy_ProcUpHead
(
    IN _WEB_PROXY_NODE_S *pstNode,
    IN UCHAR *pucHead,
    IN UINT uiHeadLen
)
{
    HTTP_HEAD_PARSER hReplyParser;
    HTTP_HEAD_PARSER hEncap;
    HTTP_HEAD_FIELD_S *pstHeadField = NULL;
    CHAR *pcContentType;
    HTTP_STATUS_CODE_E eCode;

    hEncap = WS_Trans_GetHttpEncap(pstNode->hWsTrans);

    hReplyParser = HTTP_CreateHeadParser();
    if (NULL == hReplyParser)
    {
        return BS_ERR;
    }

    if (BS_OK != HTTP_ParseHead(hReplyParser, (CHAR*)pucHead, uiHeadLen, HTTP_RESPONSE))
    {
        HTTP_DestoryHeadParser(hReplyParser);
        return BS_ERR;
    }

    if (TRUE == _webproxy_IsDigestUnauthorized(hReplyParser))
    {
        _webproxy_ProcUnauthorized(pstNode, hReplyParser, hEncap);
        HTTP_DestoryHeadParser(hReplyParser);
        return BS_STOP;
    }
    else
    {
        eCode = HTTP_GetStatusCode(hReplyParser);
        HTTP_SetStatusCode(hEncap, eCode);

        while (NULL != (pstHeadField = HTTP_GetNextHeadField(hReplyParser, pstHeadField)))
        {
            if (FALSE == _webproxy_IsIgnoreReplyHeader(pstHeadField->pcFieldName))
            {
                _webproxy_SetReplyHeadField(pstNode, hEncap, pstHeadField->pcFieldName, pstHeadField->pcFieldValue);
            }
        }

        pcContentType = HTTP_GetHeadField(hReplyParser, HTTP_FIELD_CONTENT_TYPE);
        if (NULL != pcContentType)
        {
            _webproxy_InitRw(pstNode, pcContentType);
            if ((NULL != pstNode->pfRwFunc)
                && (pstNode->pstCtrl->uiFlag & WEB_PROXY_FLAG_ERRTYPE_COMPATIBLE))
            {
                pstNode->uiFlag |= _WEB_PROXY_FLAG_ERRTYPE_COMPATIBLE;
            }
        }

        WS_Trans_SetHeadFieldFinish(pstNode->hWsTrans);
    }

    HTTP_DestoryHeadParser(hReplyParser);

    return BS_OK;
}

#define _WEB_PROXY_ERRTYPE_COMPATIBLE_LEN 2

static BOOL_T _webproxy_IsErrTypeCompatible(IN UCHAR *pucData, IN UINT uiDataLen)
{
    UINT i = 0;
    
    if (uiDataLen < _WEB_PROXY_ERRTYPE_COMPATIBLE_LEN)
    {
        return TRUE;
    }

    if (uiDataLen >= 3)
    {
        
        if (((pucData[0] == 0xFF) && (pucData[1] == 0xFE))
            || ((pucData[0] == 0xFE) && (pucData[1] == 0xFF)))
        {
            i = 2;
        }
        else if ((pucData[0] == 0xEF) && (pucData[1] == 0xBB) && (pucData[2] == 0xBF))
        {
            i = 3;
        }
    }

    for (; i<_WEB_PROXY_ERRTYPE_COMPATIBLE_LEN; i++)
    {
        if (pucData[i] > 127)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static BS_STATUS _webproxy_ProcessUpBody(IN _WEB_PROXY_NODE_S *pstNode, IN UCHAR *pucData, IN UINT uiDataLen)
{
    if (pstNode->pfRwFunc == NULL)
    {
        return _webproxy_SendBody2Down(pstNode, pucData, uiDataLen);
    }

    if (pstNode->uiFlag & _WEB_PROXY_FLAG_ERRTYPE_COMPATIBLE)
    {
        BIT_CLR(pstNode->uiFlag, _WEB_PROXY_FLAG_ERRTYPE_COMPATIBLE);
        if (TRUE == _webproxy_IsErrTypeCompatible(pucData, uiDataLen))
        {
            pstNode->pfRwFunc = NULL;
            return _webproxy_SendBody2Down(pstNode, pucData, uiDataLen);
        }
    }
    
    return pstNode->pfRwFunc(pstNode, pucData, uiDataLen);
}

static BS_STATUS _webproxy_RecvUpHead(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    UCHAR aucData[1024*32];
    INT iReadLen;
    UINT uiDataLen;
    UINT uiHeadLen;
    UCHAR *pucData;
    BOOL_T bCopyed = FALSE;
    BS_STATUS eRet;
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    iReadLen = _webproxy_UpRead(pstNode, aucData, sizeof(aucData));
    if (iReadLen == SOCKET_E_AGAIN)
    {
        return BS_OK;
    }

    if (iReadLen <= 0)
    {
        return BS_ERR;
    }

    pucData = aucData;
    uiDataLen = iReadLen;
    if (VBUF_GetDataLength(&pstNode->stVBuf) != 0)
    {
        if (BS_OK != VBUF_CatBuf(&pstNode->stVBuf, aucData, iReadLen))
        {
            return BS_ERR;
        }

        pucData = VBUF_GetData(&pstNode->stVBuf);
        uiDataLen = VBUF_GetDataLength(&pstNode->stVBuf);
        bCopyed = TRUE;
    }

    uiHeadLen = HTTP_GetHeadLen((CHAR*)pucData, uiDataLen);
    if (0 == uiHeadLen) {
        if (uiDataLen > _WEB_PROXY_MAX_RES_HEAD_LEN) {
            return BS_ERR;
        }

        if (bCopyed == FALSE) {
            if (BS_OK != VBUF_CatBuf(&pstNode->stVBuf, pucData, uiDataLen)) {
                return BS_ERR;
            }
        }

        return BS_OK;
    }

    
    {
        pucData[uiHeadLen-2] = '\0';
        BS_DBG_OUTPUT(pstNode->pstCtrl->uiDbgFlag, WEB_PROXY_DBG_FLAG_PACKET,
            ("--------------\r\nReceive response header:\r\n%s\r\n--------------\r\n", pucData));
        pucData[uiHeadLen-2] = '\r';
    }

    eRet = _webproxy_ProcUpHead(pstNode, pucData, uiHeadLen);
    if (BS_STOP == eRet)
    {
        VBUF_CutAll(&pstNode->stVBuf);
        _webproxy_RecvBodyEnd(pstNode);
        return BS_OK;
    }
    
    if (BS_OK != eRet)
    {
        return BS_ERR;
    }

    FSM_SetState(pstFsm, _WEB_PROXY_STATE_RES_BODY);

    pucData += uiHeadLen;
    uiDataLen -= uiHeadLen;
    if (VBUF_GetDataLength(&pstNode->stVBuf) != 0)
    {
        VBUF_CutHead(&pstNode->stVBuf, uiHeadLen);
    }

    if (uiDataLen == 0)
    {
        return BS_OK;
    }

    if (uiDataLen >= _WEB_PROXY_CACHE_SIZE)
    {
        eRet = _webproxy_ProcessUpBody(pstNode, pucData, uiDataLen);
        VBUF_CutAll(&pstNode->stVBuf);
        return eRet;
    }

    if (VBUF_GetDataLength(&pstNode->stVBuf) == 0)
    {
        if (BS_OK != VBUF_CpyBuf(&pstNode->stVBuf, pucData, uiDataLen))
        {
            return BS_ERR;
        }
    }

    return BS_OK;
}

static VOID _webproxy_RecvBodyEnd(IN _WEB_PROXY_NODE_S *pstNode)
{
    if (VBUF_GetDataLength(&pstNode->stVBuf) > 0)
    {
        _webproxy_ProcessUpBody(pstNode, VBUF_GetData(&pstNode->stVBuf), VBUF_GetDataLength(&pstNode->stVBuf));
        VBUF_CutAll(&pstNode->stVBuf);
    }
    
    if (pstNode->hCssUpHandle != NULL)
    {
        CSS_UP_End(pstNode->hCssUpHandle);
    }

    if (pstNode->hJsRwHandle != NULL)
    {
        JS_RW_End(pstNode->hJsRwHandle);
    }

    if (pstNode->hHtmlUp)
    {
        HTML_UP_InputHtmlEnd(pstNode->hHtmlUp);
    }

    MyPoll_Del(pstNode->hPoller, pstNode->iUpSocketID);
    Socket_Close(pstNode->iUpSocketID);
    if (NULL != pstNode->pstSsl)
    {
        SSL_UTL_Free(pstNode->pstSsl);
        pstNode->pstSsl = NULL;
    }
    pstNode->iUpSocketID = -1;

    WS_Trans_ReplyBodyFinish(pstNode->hWsTrans);
}

static BS_STATUS _webproxy_RecvUpBody(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    UCHAR aucData[1024*32];
    INT iReadLen;
    UINT uiDataLen;
    UCHAR *pucData;
    BS_STATUS eRet;
    _WEB_PROXY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    iReadLen = _webproxy_UpRead(pstNode, aucData, sizeof(aucData));
    if (iReadLen == SOCKET_E_AGAIN)
    {
        return BS_OK;
    }

    if (iReadLen <= 0)
    {
        _webproxy_RecvBodyEnd(pstNode);
        return BS_OK;
    }

    pucData = aucData;
    uiDataLen = iReadLen;

    if (VBUF_GetDataLength(&pstNode->stVBuf) + uiDataLen < _WEB_PROXY_CACHE_SIZE)
    {
        return VBUF_CatBuf(&pstNode->stVBuf, pucData, uiDataLen);
    }

    if (VBUF_GetDataLength(&pstNode->stVBuf) > 0)
    {
        eRet = _webproxy_ProcessUpBody(pstNode, VBUF_GetData(&pstNode->stVBuf), VBUF_GetDataLength(&pstNode->stVBuf));
        VBUF_CutAll(&pstNode->stVBuf);
        if (BS_OK != eRet)
        {
            return eRet;
        }
    }

    return _webproxy_ProcessUpBody(pstNode, pucData, uiDataLen);
}

static FSM_SWITCH_MAP_S g_astWebProxySwitchMap[] =
{
    {"S.Connecting", "E.UpWrite", "@", _webproxy_Connected},
    {"S.Connecting", "E.DownReadOK", "@", _webproxy_DownReadOKFast},
    {"S.SslConnecting", "E.UpWrite,E.UpRead", "@", _webproxy_SslConnecting},
    {"S.ReqHead",    "E.UpWrite", "@", _webproxy_SendReqHead},
    {"S.ReqBody",    "E.DownRead,E.UpWrite", "@", _webproxy_SendReqBody},
    {"S.ReqBody",    "E.DownReadOK", "S.ResHead", _webproxy_DownReadOK},
    {"S.ResHead",    "E.UpRead", "@", _webproxy_RecvUpHead},
    {"S.ResBody",    "E.UpRead", "@", _webproxy_RecvUpBody},

    {"S.Connecting", "E.UpErr",   "@", _webproxy_ConnectErr},
    {"S.SslConnecting", "E.UpErr", "@", _webproxy_SslConnectErr},
    {"S.ReqHead",    "E.UpErr",   "@", _webproxy_SendReqUpErr},
    {"S.ReqBody",    "E.UpErr",   "@", _webproxy_SendReqUpErr},
    {"S.ResHead",    "E.UpErr",   "@", _webproxy_RecvUpHeadErr},
    {"S.ResBody",    "E.UpErr",   "@", _webproxy_RecvUpBodyErr}
};

static FSM_SWITCH_TBL g_hWebProxySwitchTbl = NULL;

static int _webproxy_UpEvent(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    _WEB_PROXY_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];
    BS_STATUS eRet = BS_OK;

    if (uiEvent & MYPOLL_EVENT_ERR)
    {
        eRet = FSM_EventHandle(&pstNode->stFsm, _WEB_PROXY_EVENT_UP_ERR);
        return 0;
    }

    if (uiEvent & MYPOLL_EVENT_IN)
    {
        eRet = FSM_EventHandle(&pstNode->stFsm, _WEB_PROXY_EVENT_UP_READ);
    }

    if ((eRet == BS_OK) && (uiEvent & MYPOLL_EVENT_OUT))
    {
        eRet = FSM_EventHandle(&pstNode->stFsm, _WEB_PROXY_EVENT_UP_WRITE);
    }

    if (eRet != BS_OK)
    {
        WS_Trans_Err(pstNode->hWsTrans);
    }

    return 0;
}

static BS_STATUS _webproxy_ConnectServer(IN _WEB_PROXY_NODE_S *pstNode)
{
    INT iSocket;
    UINT uiIp;
    USER_HANDLE_S stUserHandle;

    uiIp = Socket_NameToIpHost(pstNode->szHost);
    if (0 == uiIp)
    {
        return BS_ERR;
    }

    iSocket = Socket_Create(AF_INET, SOCK_STREAM);
    if (iSocket < 0)
    {
        return BS_ERR;
    }

    Socket_SetNoBlock(iSocket, TRUE);

    if (SOCKET_E_AGAIN != Socket_Connect(iSocket, uiIp, pstNode->usPort))
    {
        return BS_ERR;
    }

    pstNode->iUpSocketID = iSocket;
    FSM_SetState(&pstNode->stFsm, _WEB_PROXY_STATE_CONNECTING);

    stUserHandle.ahUserHandle[0] = pstNode;
    MyPoll_SetEvent(pstNode->hPoller, pstNode->iUpSocketID, MYPOLL_EVENT_OUT, _webproxy_UpEvent, &stUserHandle);

    return BS_OK;
}


BS_STATUS WEB_Proxy_ParseServerUrl
(
    IN CHAR *pcUrl,
    OUT LSTR_S *pstTag,   
    OUT LSTR_S *pstProto, 
    OUT LSTR_S *pstHost,
    OUT LSTR_S *pstPort,
    OUT LSTR_S *pstPath
)
{
    CHAR *pcEnd;
    CHAR *pcStart;

    
    pcStart = pcUrl + 1;
    pcEnd = strchr(pcStart, '/');
    if (NULL == pcEnd)
    {
        return BS_ERR;
    }
    if (NULL != pstTag)
    {
        pstTag->pcData = pcStart;
        pstTag->uiLen = pcEnd - pcStart;
    }

    
    pcStart = pcEnd + 1;
    pcEnd = strchr(pcStart, '/');
    if (NULL == pcEnd)
    {
        return BS_ERR;
    }
    if (NULL != pstProto)
    {
        pstProto->pcData = pcStart;
        pstProto->uiLen = pcEnd - pcStart;
    }

    
    pcStart = pcEnd + 1;
    pcEnd = strchr(pcStart, '/');
    if (NULL == pcEnd)
    {
        return BS_ERR;
    }
    if (NULL != pstPort)
    {
        pstPort->pcData = pcStart;
        pstPort->uiLen = pcEnd - pcStart;
    }

    
    pcStart = pcEnd + 1;
    pcEnd = strchr(pcStart, '/');

    if (NULL != pstHost)
    {
        pstHost->pcData = pcStart;
        if (pcEnd != NULL)
        {
            pstHost->uiLen = pcEnd - pcStart;
        }
        else
        {
            pstHost->uiLen = strlen(pcStart);
        }
    }

    
    if (NULL != pstPath)
    {
        pstPath->pcData = pcEnd;
        pstPath->uiLen = 0;
        if (NULL != pcEnd)
        {
            pstPath->uiLen = strlen(pcEnd);
        }
    }

    return BS_OK;
}

static BS_STATUS _webproxy_ParseServerUrlAndCheck
(
    IN _WEB_PROXY_CTRL_S *pstCtrl,
    IN CHAR *pcUrl,
    OUT LSTR_S *pstTag,   
    OUT LSTR_S *pstProto, 
    OUT LSTR_S *pstHost,
    OUT LSTR_S *pstPort,
    OUT LSTR_S *pstPath
)
{
    _WEB_PROXY_PROTO_E enProto;
    UINT uiPort;

    if (BS_OK != WEB_Proxy_ParseServerUrl(pcUrl, pstTag, pstProto, pstHost, pstPort, pstPath))
    {
        return BS_ERR;
    }

    if ((pstHost->uiLen > _WEB_PROXY_MAX_HOST_LEN)
        || (pstPath->uiLen == 0))
    {
        return BS_ERR;
    }

    if (0 != LSTR_StrCmp(pstTag, pstCtrl->szTag))
    {
        return BS_ERR;
    }

    enProto = _webproxy_GetProto(pstProto);
    if (enProto == _WEB_PROXY_PROTO_MAX)
    {
        return BS_ERR;
    }

    if (BS_OK != LSTR_Atoui(pstPort, &uiPort))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS _webproxy_GetPrefixBeforePathByUri(IN CHAR *pcUri, OUT LSTR_S *pstPrefix)
{
    CHAR *pcPath;
    
    pcPath = TXT_StrchrX(pcUri, '/', 5);
    if (pcPath == NULL)
    {
        return BS_ERR;
    }

    pstPrefix->pcData = pcUri;
    pstPrefix->uiLen = (UINT)(pcPath - pcUri) + 1;

    return BS_OK;
}

static BS_STATUS _webproxy_ParseServer(IN _WEB_PROXY_NODE_S *pstNode, IN CHAR *pcUrl)
{
    LSTR_S stTag;
    LSTR_S stProto;
    LSTR_S stHost;
    LSTR_S stPort;
    LSTR_S stPath;
    UINT uiPort;

    if (BS_OK != (WEB_Proxy_ParseServerUrl(pcUrl, &stTag, &stProto, &stHost, &stPort, &stPath)))
    {
        return BS_ERR;
    }

    if (stHost.uiLen > _WEB_PROXY_MAX_HOST_LEN)
    {
        return BS_ERR;
    }

    if (0 != LSTR_StrCmp(&stTag, pstNode->pstCtrl->szTag))
    {
        return BS_ERR;
    }
    
    pstNode->enProto = _webproxy_GetProto(&stProto);
    if (pstNode->enProto == _WEB_PROXY_PROTO_MAX)
    {
        return BS_ERR;
    }

    if (BS_OK != LSTR_Atoui(&stPort, &uiPort))
    {
        return BS_ERR;
    }
    
    pstNode->usPort = uiPort;

    TXT_Strlcpy(pstNode->szHost, stHost.pcData ,stHost.uiLen + 1);

    pstNode->stPath = stPath;
    pstNode->stPort = stPort;

    return BS_OK;
}

static BOOL_T _webproxy_TryRedirect(IN _WEB_PROXY_NODE_S *pstNode)
{
    CHAR *pcQuery;
    CHAR szTmp[1024];
    HTTP_HEAD_PARSER hHeadParser;
    CHAR *pcPath;
    CHAR *pcProto;
    USHORT usPort;

    if ((pstNode->stPath.uiLen != 0) && (pstNode->usPort != 0))
    {
        return FALSE;
    }

    hHeadParser = WS_Trans_GetHttpRequestParser(pstNode->hWsTrans);

    pcPath = pstNode->stPath.pcData;
    if (pstNode->stPath.uiLen == 0)
    {
        pcPath = "/";
    }

    pcProto = _webproxy_GetProtoString(pstNode->enProto);
    if (NULL == pcProto)
    {
        WS_Trans_Reply(pstNode->hWsTrans, HTTP_STATUS_NOT_ACC, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
        return TRUE;
    }

    usPort = pstNode->usPort;
    if (usPort == 0)
    {
        usPort = _webproxy_GetDftPort(pstNode->enProto);
        if (usPort == 0)
        {
            WS_Trans_Reply(pstNode->hWsTrans, HTTP_STATUS_NOT_ACC, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
            return TRUE;
        }
    }

    pcQuery = HTTP_GetUriQuery(hHeadParser);
    if (NULL != pcQuery)
    {
        snprintf(szTmp, sizeof(szTmp), "/%s/%s/%d/%s%s?%s",
            pstNode->pstCtrl->szTag, pcProto, usPort, pstNode->szHost, pcPath, pcQuery);
    }
    else
    {
        snprintf(szTmp, sizeof(szTmp), "/%s/%s/%d/%s%s",
            pstNode->pstCtrl->szTag, pcProto, usPort, pstNode->szHost, pcPath);
    }

    WS_Trans_Redirect(pstNode->hWsTrans, szTmp);

    return TRUE;
}

static BS_STATUS  _webproxy_RecvHeadOK
(
    IN _WEB_PROXY_CTRL_S *pstCtrl,
    IN MYPOLL_HANDLE hPoller,
    IN void *sslctx,
    IN WS_TRANS_HANDLE hWsTrans
)
{
    _WEB_PROXY_NODE_S *pstNode;
    HTTP_HEAD_PARSER hHeadParser;
    CHAR *pcUrl;

    hHeadParser = WS_Trans_GetHttpRequestParser(hWsTrans);
    pcUrl = HTTP_GetUriAbsPath(hHeadParser);

    pstNode = MEM_ZMalloc(sizeof(_WEB_PROXY_NODE_S));
    if (NULL == pstNode)
    {
        return BS_ERR;
    }
    pstNode->pstCtrl = pstCtrl;
    pstNode->sslctx = sslctx;
    pstNode->hPoller = hPoller;
    VBUF_Init(&pstNode->stVBuf);

    pstNode->hWsTrans = hWsTrans;
    WS_Trans_SetUserHandle(hWsTrans, _WEB_PROXY_NAME, pstNode);

    FSM_Init(&pstNode->stFsm, g_hWebProxySwitchTbl);
    if (pstCtrl->uiDbgFlag & WEB_PROXY_DBG_FLAG_FSM)
    {
        FSM_SetDbgFlag(&pstNode->stFsm, FSM_DBG_FLAG_ALL);
    }
    FSM_InitState(&pstNode->stFsm, _WEB_PROXY_STATE_INIT);
    FSM_SetPrivateData(&pstNode->stFsm, pstNode);

    if (BS_OK != _webproxy_ParseServer(pstNode, pcUrl))
    {
        WS_Trans_Reply(pstNode->hWsTrans, HTTP_STATUS_NOT_ACC, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
        return BS_OK;
    }

    if (TRUE == _webproxy_TryRedirect(pstNode))
    {
        return BS_OK;
    }

    if (pstNode->enProto == _WEB_PROXY_PROTO_HTTPS)
    {
        if (pstNode->sslctx== NULL) {
            WS_Trans_Reply(pstNode->hWsTrans, HTTP_STATUS_SERVICE_UAVAIL, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);
            return BS_OK;
        }
    }

    if (BS_OK != _webproxy_ConnectServer(pstNode))
    {
        _webproxy_SendString2Down(pstNode, "Can't connect ");
        _webproxy_SendString2Down(pstNode, pstNode->szHost);
        WS_Trans_ReplyBodyFinish(pstNode->hWsTrans);
        WS_Trans_Reply(pstNode->hWsTrans, HTTP_STATUS_OK, WS_TRANS_REPLY_FLAG_IMMEDIATELY);
        return BS_OK;
    }

    WS_Trans_Pause(hWsTrans);

    return BS_OK;
}

static BS_STATUS _webproxy_RecvBody(_WEB_PROXY_CTRL_S *pstCtrl, IN WS_TRANS_HANDLE hWsTrans)
{
    _WEB_PROXY_NODE_S *pstNode;
    
    pstNode = WS_Trans_GetUserHandle(hWsTrans, _WEB_PROXY_NAME);
    if (NULL == pstNode)
    {
        return BS_ERR;
    }

    if (BS_OK != FSM_EventHandle(&pstNode->stFsm, _WEB_PROXY_EVENT_DOWN_READ))
    {
        return BS_ERR;
    }
    
    return BS_OK;
}

static BS_STATUS _webproxy_RecvBodyOK(_WEB_PROXY_CTRL_S *pstCtrl, IN WS_TRANS_HANDLE hWsTrans)
{
    _WEB_PROXY_NODE_S *pstNode;

    pstNode = WS_Trans_GetUserHandle(hWsTrans, _WEB_PROXY_NAME);
    if (NULL == pstNode)
    {
        return BS_ERR;
    }

    if (BS_OK != FSM_EventHandle(&pstNode->stFsm, _WEB_PROXY_EVENT_DOWN_READ_OK))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS _webproxy_BuildBodyEvent(_WEB_PROXY_CTRL_S *pstCtrl, IN WS_TRANS_HANDLE hWsTrans)
{
    _WEB_PROXY_NODE_S *pstNode;

    pstNode = WS_Trans_GetUserHandle(hWsTrans, _WEB_PROXY_NAME);
    if (NULL == pstNode)
    {
        return BS_ERR;
    }

    MyPoll_AddEvent(pstNode->hPoller, pstNode->iUpSocketID, MYPOLL_EVENT_IN);

    return BS_OK;
}

static BS_STATUS _webproxy_Destroy(_WEB_PROXY_CTRL_S *pstCtrl, IN WS_TRANS_HANDLE hWsTrans)
{
    _WEB_PROXY_NODE_S *pstNode;

    pstNode = WS_Trans_GetUserHandle(hWsTrans, _WEB_PROXY_NAME);
    if (NULL == pstNode)
    {
        return BS_OK;
    }

    FSM_SetState(&pstNode->stFsm, _WEB_PROXY_STATE_RES_END);

    if (NULL != pstNode->hHtmlUp)
    {
        HTML_UP_Destroy(pstNode->hHtmlUp);
    }

    if (NULL != pstNode->hCssUpHandle)
    {
        CSS_UP_Destroy(pstNode->hCssUpHandle);
    }

    if (NULL != pstNode->hJsRwHandle)
    {
        JS_RW_Destroy(pstNode->hJsRwHandle);
    }

    VBUF_Finit(&pstNode->stVBuf);

    if (NULL != pstNode->pstSend2UpBody)
    {
        MBUF_Free(pstNode->pstSend2UpBody);
    }

    if (pstNode->pstSsl != NULL)
    {
        SSL_UTL_Free(pstNode->pstSsl);
    }

    if (pstNode->iUpSocketID >= 0)
    {
        MyPoll_Del(pstNode->hPoller, pstNode->iUpSocketID);
        Socket_Close(pstNode->iUpSocketID);
        pstNode->iUpSocketID = -1;
    }

    WS_Trans_SetUserHandle(hWsTrans, _WEB_PROXY_NAME, NULL);

    MEM_Free(pstNode);

    return BS_OK;
}

static VOID _webproxy_DftInitCtrl(IN _WEB_PROXY_CTRL_S *pstCtrl)
{
    TXT_Strlcpy(pstCtrl->szTag, "_proxy_", _WEB_RPOXY_MAX_TAG_NAME_LEN + 1);
    pstCtrl->usTagLen = strlen(pstCtrl->szTag);
    TXT_Strlcpy(pstCtrl->szJsRwFilePath, "/_base/_proxy/rewrite.js", _WEB_RPOXY_MAX_JS_RW_FILEPATH_LEN + 1);
    pstCtrl->usJsRwFilePathLen = strlen(pstCtrl->szJsRwFilePath);
}

static BS_STATUS _webproxy_InitFsmTbl()
{
    SPLX_P();
    if (NULL == g_hWebProxySwitchTbl)
    {
        g_hWebProxySwitchTbl = FSM_CREATE_SWITCH_TBL(g_astWebProxyStateMap,
                    g_astWebProxyEventMap, g_astWebProxySwitchMap);
    }
    SPLX_V();

    if (NULL == g_hWebProxySwitchTbl)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

WEB_PROXY_HANDLE WEB_Proxy_Create(IN UINT uiFlag)
{
    _WEB_PROXY_CTRL_S *pstCtrl;

    if (BS_OK != _webproxy_InitFsmTbl())
    {
        return NULL;
    }

    pstCtrl = MEM_ZMalloc(sizeof(_WEB_PROXY_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    NAP_PARAM_S param = {0};
    param.enType = NAP_TYPE_HASH;
    param.uiMaxNum = _WEB_PROXY_AUTH_DIGEST_MAX_NAP_NUM;
    param.uiNodeSize = sizeof(_WEB_PROXY_AUTH_NODE_S);

    pstCtrl->hAuthNAP = NAP_Create(&param);
    if (NULL == pstCtrl->hAuthNAP)
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    NAP_EnableSeq(pstCtrl->hAuthNAP, 0, _WEB_PROXY_AUTH_DIGEST_MAX_NAP_NUM);

    pstCtrl->uiFlag = uiFlag;

    _webproxy_DftInitCtrl(pstCtrl);

    return pstCtrl;
}

VOID WEB_Proxy_Destroy(IN WEB_PROXY_HANDLE hWebProxy)
{
    _WEB_PROXY_CTRL_S *pstCtrl = hWebProxy;

    if (NULL != pstCtrl->hAuthNAP)
    {
        NAP_FreeAll(pstCtrl->hAuthNAP);
        NAP_Destory(pstCtrl->hAuthNAP);
    }

    MEM_Free(pstCtrl);
}

VOID WEB_Proxy_SetDgbFlag(IN WEB_PROXY_HANDLE hWebProxy, IN UINT uiDbgFlag)
{
    _WEB_PROXY_CTRL_S *pstCtrl = hWebProxy;

    pstCtrl->uiDbgFlag |= uiDbgFlag;
}

VOID WEB_Proxy_ClrDgbFlag(IN WEB_PROXY_HANDLE hWebProxy, IN UINT uiDbgFlag)
{
    _WEB_PROXY_CTRL_S *pstCtrl = hWebProxy;

    pstCtrl->uiDbgFlag &= (~uiDbgFlag);
}


BOOL_T WEB_Proxy_IsRwedUrl(IN WEB_PROXY_HANDLE hWebProxy, IN CHAR *pcUrl)
{
    _WEB_PROXY_CTRL_S *pstCtrl = hWebProxy;
    LSTR_S stTag;
    LSTR_S stProto;
    LSTR_S stHost;
    LSTR_S stPort;
    LSTR_S stPath;

    if (BS_OK != _webproxy_ParseServerUrlAndCheck(pstCtrl, pcUrl, &stTag, &stProto, &stHost, &stPort, &stPath))
    {
        return FALSE;
    }

    return TRUE;
}

static BS_STATUS _webproxy_Refer2Prefix
(
    IN WEB_PROXY_HANDLE hWebProxy,
    IN CHAR *pcRefer,
    OUT CHAR *pcPrefix,
    IN UINT uiPrefixSize
)
{
    _WEB_PROXY_CTRL_S *pstCtrl = hWebProxy;
    LSTR_S stTag;
    LSTR_S stProto;
    LSTR_S stHost;
    LSTR_S stPath;
    LSTR_S stPort;
    CHAR szTmpProto[16];
    CHAR szTmpPort[12];
    CHAR szTmpHost[64];

    stPath.pcData = pcRefer;
    stPath.uiLen = strlen(pcRefer);

    if (BS_OK != _webproxy_ParseServerUrlAndCheck(pstCtrl, stPath.pcData,
            &stTag, &stProto, &stHost, &stPort, &stPath))
    {
        return BS_ERR;
    }

    
    snprintf(pcPrefix, uiPrefixSize, "/%s/%s/%s/%s",
                pstCtrl->szTag,
                LSTR_Strlcpy(&stProto, sizeof(szTmpProto), szTmpProto),
                LSTR_Strlcpy(&stPort, sizeof(szTmpPort), szTmpPort),
                LSTR_Strlcpy(&stHost, sizeof(szTmpHost), szTmpHost));

    return BS_OK;
}

BS_STATUS WEB_Proxy_ReferInput
(
    IN WEB_PROXY_HANDLE hWebProxy,
    IN WS_TRANS_HANDLE hTrans,
    IN UINT uiEvent
)
{
    CHAR *pcUrl;
    CHAR *pcQuery;
    CHAR *pcRefer;
    HTTP_HEAD_PARSER hParser;
    CHAR szPrefix[128];
    CHAR szTmp[1024];

    hParser = WS_Trans_GetHttpRequestParser(hTrans);
    pcUrl = HTTP_GetUriAbsPath(hParser);

    pcRefer = HTTP_GetHeadField(hParser, HTTP_FIELD_REFERER);

    if (BS_OK != _webproxy_Refer2Prefix(hWebProxy, pcRefer, szPrefix, sizeof(szPrefix)))
    {
        return BS_ERR;
    }

    pcQuery = HTTP_GetUriQuery(hParser);

    if (NULL == pcQuery)
    {
        snprintf(szTmp, sizeof(szTmp), "%s%s", szPrefix, pcUrl);
    }
    else
    {
        snprintf(szTmp, sizeof(szTmp), "%s%s?%s", szPrefix, pcUrl, pcQuery);
    }

    WS_Trans_Redirect(hTrans, szTmp);

    return BS_OK;
}

BS_STATUS WEB_Proxy_AuthAgentInput
(
    IN WEB_PROXY_HANDLE hWebProxy,
    IN WS_TRANS_HANDLE hTrans,
    IN UINT uiEvent
)
{
    _WEB_PROXY_CTRL_S *pstCtrl = hWebProxy;
    HTTP_HEAD_PARSER hParser;
    HTTP_HEAD_PARSER hEncap;
    MIME_HANDLE hBodyMime;
    CHAR *pcAuthCookie;
    CHAR *pcRedirectUri;
    CHAR *pcUserName;
    CHAR *pcPassWord;
    _WEB_PROXY_AUTH_NODE_S *pstAuthNode;
    CHAR szCookie[256];
    LSTR_S stPrefix;
    CHAR szPrefix[_WEB_PROXY_PREFIX_BEFORE_PATH_MAX_LEN + 1];

    hParser = WS_Trans_GetHttpRequestParser(hTrans);
    hEncap = WS_Trans_GetHttpEncap(hTrans);
    hBodyMime = WS_Trans_GetBodyMime(hTrans);

    if ((NULL == hParser) || (NULL == hEncap) || (NULL == hBodyMime))
    {
        return BS_ERR;
    }

    pcAuthCookie = MIME_GetKeyValue(hBodyMime, "_proxy_auth_cookie");
    pcRedirectUri = MIME_GetKeyValue(hBodyMime, "redirect_uri");
    if ((NULL == pcAuthCookie) || (NULL == pcRedirectUri))
    {
        return BS_ERR;
    }

    if (BS_OK != _webproxy_GetPrefixBeforePathByUri(pcRedirectUri, &stPrefix))
    {
        return BS_ERR;
    }

    LSTR_Lstr2Str(&stPrefix, szPrefix, sizeof(szPrefix));

    pstAuthNode = _webproxy_GetAuthNodeByCookie(pstCtrl, pcAuthCookie);
    if (NULL == pstAuthNode)
    {
        return BS_ERR;
    }

    pcUserName = MIME_GetKeyValue(hBodyMime, "user_name");
    pcPassWord = MIME_GetKeyValue(hBodyMime, "user_password");

    if (BS_OK != HTTP_Auth_ClientSetUser(pstAuthNode->hHttpAuth, pcUserName, pcPassWord))
    {
        return BS_ERR;
    }

    if (snprintf(szCookie, sizeof(szCookie), "_proxy_auth_cookie=%s; path=%s", pcAuthCookie, szPrefix) < 0) {
        return BS_ERR;
    }

    HTTP_SetHeadField(hEncap, HTTP_FIELD_SET_COOKIE, szCookie);

    return WS_Trans_Redirect(hTrans, pcRedirectUri);
}

BS_STATUS WEB_Proxy_RequestIn
(
    IN WEB_PROXY_HANDLE hWebProxy,
    IN MYPOLL_HANDLE hPoller,
    IN void *sslctx,
    IN WS_TRANS_HANDLE hTrans,
    IN UINT uiEvent
)
{
    BS_STATUS eRet = BS_ERR;
    _WEB_PROXY_CTRL_S *pstCtrl = hWebProxy;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_HEAD_OK:
        {
            eRet = _webproxy_RecvHeadOK(pstCtrl, hPoller, sslctx, hTrans);
            break;
        }

        case WS_TRANS_EVENT_RECV_BODY:
        {
            eRet = _webproxy_RecvBody(pstCtrl, hTrans);
            break;
        }

        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = _webproxy_RecvBodyOK(pstCtrl, hTrans);
            break;
        }

        case WS_TRANS_EVENT_BUILD_BODY:
        {
            eRet = _webproxy_BuildBodyEvent(pstCtrl, hTrans);
            break;
        }

        case WS_TRANS_EVENT_DESTORY:
        {
            eRet = _webproxy_Destroy(pstCtrl, hTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}



