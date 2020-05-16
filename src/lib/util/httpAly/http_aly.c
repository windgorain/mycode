/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-5-19
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_HTTPALY


#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/ssltcp_utl.h"
#include "utl/num_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/http_aly.h"
#include "utl/data2hex_utl.h"


/* ---define--- */
#define HTTP_ALY_MAX_UINT_STRING_LEN 10 /* UINT的十进制表示字符串的长度 */

#define HTTP_ALY_MAX_COOKIE_LEN 255

#define HTTP_ALY_IS_HEAD_RECVED(pstHttpAlyCtrl) ((pstHttpAlyCtrl)->stReadData.bIsHeadFin)
#define HTTP_ALY_IS_BODY_RECVED(pstHttpAlyCtrl) ((pstHttpAlyCtrl)->stReadData.bIsBodyFin)

/* ---struct--- */
typedef struct
{
    DLL_NODE_S stDllNode;   /* 必须是第一个域 */
    UCHAR *pucKey;
    UCHAR *pucKeyValue;
    UINT ulValueOffsetInHead;
    UINT ulKeyOffsetInHead;
    BOOL_T bIsBuildHttpHeadDisable;  /* 如果是True, 则不参与HTTP_ALY_BuildHttpHead 的组装 */
}_HTTP_ALY_KEY_S;

typedef struct
{
    DLL_NODE_S stDllNode;   /* 必须是第一个域 */
    UCHAR *pucKey;
    UCHAR *pucKeyValue;
    UCHAR *pszPath;
}_HTTP_ALY_COOKIE_S;

typedef struct
{
    UCHAR  *pucMethod;
    UCHAR  *pucUrlPath;       /* 不包含?号后面的Query */
    UCHAR  *pucQueryString;
    UCHAR  *pucHost;
    DLL_HEAD_S stCookie;      /* 收到的Cookie */
    UCHAR  *pucReferer;
    BOOL_T bIsProxyConnection;
    BOOL_T bKeepAlive;
    HTTP_ALY_BODY_LEN_TYPE_E eBodyLenType;
    USHORT usPort;
    UINT  ulBodyLen;
    UINT  ulHeadLen;   /* 包含"\r\n\r\n" */
    HTTP_PROTOCOL_VER_E  eHttpVer;
    HTTP_PROTOCOL_TYPE_E eHttpType;
    HTTP_ALY_METHOD_E eMethod;

    DLL_HEAD_S       stDllHeadField;    /* http头中的Field */
    
    DLL_HEAD_S       stDllKeyValueHead;     /* 请求体和URL ?号后面的key value对的解析 */
}_HTTP_REQUEST_INFO_S;

#define _HTTP_ALY_MAX_STATUS_DESC_LEN 31
typedef struct
{
    HTTP_PROTOCOL_VER_E eHttpVer;
    BOOL_T bIsKeepAlive;
    BOOL_T bIsSendHead;     /* 是否已经发送了头 */
    BOOL_T bIsFinish;  /* 用户已经调用了Finish接口 */
    UCHAR  ucResponseMode;  /* HTTP_ALY_RESPONSE_MODE_CLOSED/HTTP_ALY_RESPONSE_MODE_CHUNKED/HTTP_ALY_RESPONSE_MODE_LENGTH */
    UINT   ulStatusCode;  /* 回应状态码 */
    CHAR   szStatusDesc[_HTTP_ALY_MAX_STATUS_DESC_LEN + 1];
    DLL_HEAD_S stCookie;    /* 待发送的Cookie链 */
    DLL_HEAD_S stSendKeyValue; /* 需要在头中发送的Key-Value */
    VBUF_S stSendBuf;        /* 发送缓冲区 */
    UINT  ulChunkRmainLen;
}_HTTP_ALY_RESPONSE_INFO_S;

typedef struct
{
    BOOL_T bIfRemoveChunkFlag;
    MBUF_S *pstHeadMbuf;
    BOOL_T bIsHeadFin;
    MBUF_S *pstBodyMbuf;
    UINT  ulRecvBodyLen;

    /* for chunked */
    UINT  ulScanOffset;        /* 已经扫描到了哪里. 从本次会话体的开始外置为0 算起 */
    UINT  ulBeginOffset;       /* 存在协议解析器中的数据的起始位置,从本次会话体的开始外置为0 算起 */
    BOOL_T bIsRecvChunkedFlagOk;  /* 表示是否接收chunked标记完毕。 */
    CHAR   szChunkedString[16];   /* chunked标记, 从'\r\n'开始记录 */
    UINT  ulChunkedStringOffset; /* 当前chunked块的起始位置,从"\r\nchunkedflag\r\n"开始 */
    UINT  ulChunkedStringLen;    /* szChunkedString中已经记录的长度 */
    UINT  ulChunkedLen;     /* 本Chunked块长度, 不包括chunked标记 */
    UINT  ulChunkedEndLen;  /* 本Chunked块还剩下多少数据没有收齐并扫描 */
    BOOL_T bIsLastChunk;


    BOOL_T bIsBodyFin;
    MBUF_S *pstNextHttpMbuf;
    BOOL_T bIsConnClosed;
}_HTTP_ALY_READ_DATA_S;

typedef struct
{
    UCHAR           *pucBuf;
    UINT            ulBufLen;
    BOOL_T          bIsServer;   /* TRUE: 收request, 发response;  FALSE: 发request, 收response */
    _HTTP_REQUEST_INFO_S stHttpAlyRequestInfo;
    _HTTP_ALY_RESPONSE_INFO_S stHttpAlyResponseInfo;
    _HTTP_ALY_READ_DATA_S stReadData;

    PF_HTTP_ALY_READ   pfReadFunc;
    UINT              ulFd;
}_HTTP_ALY_CTRL_S;

/* ---func--- */

static HTTP_ALY_READ_RET_E _HTTP_ALY_DftReadFunc
(
    IN UINT hSslTcpId,
    OUT UCHAR *pucBuf,
    IN UINT ulBufLen,
    OUT UINT *pulReadLen
)
{
    BS_STATUS eRet;
    
    eRet = SSLTCP_Read(hSslTcpId, pucBuf, ulBufLen, pulReadLen);
    if (eRet == BS_OK)
    {
        if (*pulReadLen == ulBufLen)
        {
            return HTTP_ALY_CONTINUE;
        }

        return HTTP_ALY_STOP;
    }

    return HTTP_ALY_ERR;
}

static BS_STATUS _HTTP_ALY_AlyStatusLine(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl, IN CHAR *pcBuf, IN UINT ulLen)
{
    CHAR *pcBufTmp = pcBuf;
    CHAR *pcFoundSplite;
	UINT ulLineLen = ulLen;
    UINT ulStatusCodeLen;
    CHAR szStatusCode[20];
    
    /*get http ver*/
    {
        if (strncmp("HTTP/1.1", pcBufTmp, sizeof("HTTP/1.1") - 1) == 0)
        {
            pstHttpAlyCtrl->stHttpAlyResponseInfo.eHttpVer = HTTP_PROTOCOL_VER_11;
        }
        else
        {
            pstHttpAlyCtrl->stHttpAlyResponseInfo.eHttpVer = HTTP_PROTOCOL_VER_10;
        }
    }

    pcFoundSplite = TXT_Strnchr(pcBufTmp, ' ', ulLineLen);
    if (pcFoundSplite == NULL)
    {
        BS_WARNNING(("Bad http status line:%s", pcBuf));
        RETURN(BS_NOT_SUPPORT);
    }

    ulLineLen -= (pcFoundSplite - pcBufTmp + 1);
    pcBufTmp = pcFoundSplite + 1;
    
    /* get status code */
    {
        pcFoundSplite = TXT_Strnchr(pcBufTmp, ' ', ulLineLen);
        if (pcFoundSplite == NULL)
        {
            ulStatusCodeLen = ulLineLen;
            ulLineLen = 0;
        }
        else
        {
            ulStatusCodeLen = pcFoundSplite - pcBufTmp;
            ulLineLen -= (ulStatusCodeLen + 1);
        }

        if (ulStatusCodeLen >= sizeof(szStatusCode))
        {
            BS_WARNNING(("Bad http status line:%s", pcBuf));
            RETURN(BS_NOT_SUPPORT);
        }

        MEM_Copy(szStatusCode, pcBufTmp, ulStatusCodeLen);
        szStatusCode[ulStatusCodeLen] = '\0';
        TXT_Atoui(szStatusCode, &pstHttpAlyCtrl->stHttpAlyResponseInfo.ulStatusCode);

        pcBufTmp = pcFoundSplite + 1;
    }

    if (ulLineLen == 0)
    {
        return BS_OK;
    }
    
    /* get status des */
    {
        TXT_Strlcpy(pstHttpAlyCtrl->stHttpAlyResponseInfo.szStatusDesc,
            pcBufTmp, sizeof(pstHttpAlyCtrl->stHttpAlyResponseInfo.szStatusDesc));
    }

    return BS_OK;
}

static BS_STATUS _HTTP_ALY_AlyRequestLine(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl, IN CHAR *pcBuf, IN UINT ulLen)
{
    CHAR *pcBufTmp = pcBuf;
    CHAR *pcFoundSplite;
    CHAR aucPort[10];
    UINT ulLineLen = ulLen;

    /*get method*/
    {
        pcFoundSplite = TXT_Strnchr(pcBufTmp, ' ', ulLineLen);
        if (pcFoundSplite == NULL)
        {
            BS_WARNNING(("Bad http request line:%s", pcBuf));
            RETURN(BS_NOT_SUPPORT);
        }

        pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod = MEM_Malloc(pcFoundSplite - pcBufTmp + 1);
        if (pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod == NULL)
        {
            BS_WARNNING(("No memory!"));
            RETURN(BS_NO_MEMORY);
        }
        MEM_Copy(pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod, pcBufTmp, pcFoundSplite - pcBufTmp);
        pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod[pcFoundSplite - pcBufTmp] = '\0';

        ulLineLen -= (pcFoundSplite - pcBufTmp + 1);
        pcBufTmp = pcFoundSplite + 1;
    }

    /* get path */
    {
        pstHttpAlyCtrl->stHttpAlyRequestInfo.usPort = 80;
        
        if (strncmp(pcBufTmp, "http://", sizeof("http://") - 1) == 0)
        {
            CHAR *pcFoundTmp;
            
            ulLineLen -= (sizeof("http://") - 1);
            pcBufTmp += (sizeof("http://") - 1);
            
            pcFoundSplite = TXT_Strnchr(pcBufTmp, '/', ulLineLen);
            if (pcFoundSplite == NULL)
            {
                BS_WARNNING(("Bad http request line!"));
                RETURN(BS_NOT_SUPPORT);
            }
            pcFoundTmp = TXT_Strnchr(pcBufTmp, ':', pcFoundSplite - pcBufTmp);
            if (pcFoundTmp)
            {
                UINT port;
                pcFoundTmp += 1;
                
                if (pcFoundSplite - pcFoundTmp > (sizeof("65535") - 1))
                {
                    BS_WARNNING(("Bad http request line!"));
                    RETURN(BS_NOT_SUPPORT);
                }
                MEM_Copy(aucPort, pcFoundTmp, pcFoundSplite - pcFoundTmp);
                aucPort[pcFoundSplite - pcFoundTmp] = 0;
                sscanf(aucPort, "%u", &port);
                pstHttpAlyCtrl->stHttpAlyRequestInfo.usPort = port;
            }

            ulLineLen -= (pcFoundSplite - pcBufTmp + 1);
            pcBufTmp = pcFoundSplite;
        }
        
        if (NULL == (pcFoundSplite = TXT_Strnchr(pcBufTmp, '?', ulLineLen)))
        {
            pcFoundSplite = TXT_Strnchr(pcBufTmp, ' ', ulLineLen);
        }

        if (pcFoundSplite == NULL)
        {
            BS_WARNNING(("Bad http request line!"));
            RETURN(BS_NOT_SUPPORT);
        }

        pstHttpAlyCtrl->stHttpAlyRequestInfo.pucUrlPath = MEM_Malloc(pcFoundSplite - pcBufTmp + 1);
        if (pstHttpAlyCtrl->stHttpAlyRequestInfo.pucUrlPath == NULL)
        {
            BS_WARNNING(("No memory!"));
            RETURN(BS_NO_MEMORY);
        }
        MEM_Copy(pstHttpAlyCtrl->stHttpAlyRequestInfo.pucUrlPath, pcBufTmp, pcFoundSplite - pcBufTmp);
        pstHttpAlyCtrl->stHttpAlyRequestInfo.pucUrlPath[pcFoundSplite - pcBufTmp] = '\0';

        ulLineLen -= (pcFoundSplite - pcBufTmp + 1);
        pcBufTmp = pcFoundSplite + 1;
    }

    /* get query string */
    if (*pcFoundSplite == '?')
    {
        pcFoundSplite = TXT_Strnchr(pcBufTmp, ' ', ulLineLen);

        if (pcFoundSplite == NULL)
        {
            BS_WARNNING(("Bad http request line!"));
            RETURN(BS_NOT_SUPPORT);
        }

        pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString = MEM_Malloc(pcFoundSplite - pcBufTmp + 1);
        if (pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString == NULL)
        {
            BS_WARNNING(("No memory!"));
            RETURN(BS_NO_MEMORY);
        }
        MEM_Copy(pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString, pcBufTmp, pcFoundSplite - pcBufTmp);
        pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString[pcFoundSplite - pcBufTmp] = '\0';

        ulLineLen -= (pcFoundSplite - pcBufTmp + 1);
        pcBufTmp = pcFoundSplite + 1;
    }


    /*get http ver*/
    {
        if (strncmp("HTTP/1.1", pcBufTmp, sizeof("HTTP/1.1") - 1) == 0)
        {
            pstHttpAlyCtrl->stHttpAlyRequestInfo.eHttpVer = HTTP_PROTOCOL_VER_11;
        }
        else
        {
            pstHttpAlyCtrl->stHttpAlyRequestInfo.eHttpVer = HTTP_PROTOCOL_VER_10;
        }
    }

    return BS_OK;
}

/*
* 解析Cookie;
* 将类似uid=xxx;pas=bbb解析成key-value对的形式放在pstCookieList中
*/
static BS_STATUS _HTTP_ALY_ParseCookie(IN CHAR *pszCookie, OUT DLL_HEAD_S *pstCookieList)
{
    CHAR *pszCookieNode;
    CHAR *pcSplit1, *pcSplit2;
    UINT ulLen;
    _HTTP_ALY_COOKIE_S *pstCookieKey;
    UINT ulKeyLen;
    UINT ulValueLen;
    BOOL_T bEnd = FALSE;

    BS_DBGASSERT(NULL != pszCookie);
    BS_DBGASSERT(NULL != pstCookieList);

    pszCookieNode = pszCookie;

    while (bEnd == FALSE)
    {
        pcSplit2 = strchr(pszCookieNode, ';');

        if (NULL != pcSplit2)
        {
            ulLen = pcSplit2 - pszCookieNode;
        }
        else
        {
            ulLen = strlen(pszCookieNode);
            bEnd = TRUE;
        }

        if (ulLen == 0)
        {
            pszCookieNode += 1;
            continue;
        }

        pcSplit1 = TXT_Strnchr(pszCookieNode, '=', ulLen);
        if (NULL == pcSplit1)   /* 没有Value */
        {
            ulKeyLen = ulLen;
            ulValueLen = 0;
        }
        else
        {
            ulKeyLen = pcSplit1 - pszCookieNode;
            ulValueLen = ulLen - ulKeyLen - 1;
        }
        
        pstCookieKey = MEM_ZMalloc(sizeof(_HTTP_ALY_COOKIE_S) + ulLen + 2);
        if (NULL == pstCookieKey)
        {
            RETURN(BS_NO_MEMORY);
        }

        pstCookieKey->pucKey = (UCHAR*)(pstCookieKey + 1);
        MEM_Copy(pstCookieKey->pucKey, pszCookieNode, ulKeyLen);
        pstCookieKey->pucKey[ulKeyLen] = '\0';

        pstCookieKey->pucKeyValue = pstCookieKey->pucKey + ulKeyLen + 1;
        if (ulValueLen > 0)
        {
            MEM_Copy(pstCookieKey->pucKeyValue, pcSplit1 + 1, ulValueLen);
        }
        pstCookieKey->pucKeyValue[ulValueLen] = '\0';

        TXT_StrimAndMove((void*)pstCookieKey->pucKey);
        TXT_StrimAndMove((void*)pstCookieKey->pucKeyValue);

        DLL_ADD(pstCookieList, pstCookieKey);

        pszCookieNode += ulLen + 1;
    }

    return BS_OK;
}

/***************************************************
 说明     :  解析HTTP头第一行
 输入     :  pstHttpAlyCtrl: HTTP ALY实例
             pcFirstLine: HTTP头第一行
             uiFirstLineLen: 第一行的长度
 输出     :  None
 返回值   :  成功: BS_OK
             失败: 错误码
 注意     :  None
****************************************************/
static BS_STATUS http_aly_ParseFirstLine
(
    IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl,
    IN CHAR *pcFirstLine,
    IN UINT uiFirstLineLen
)
{
    BS_STATUS eRet;
    
    if (strncmp("HTTP", pcFirstLine, 4) == 0)
    {
        pstHttpAlyCtrl->bIsServer = FALSE;
        eRet = _HTTP_ALY_AlyStatusLine(pstHttpAlyCtrl, pcFirstLine, uiFirstLineLen);
        if (BS_OK != eRet)
        {
            return eRet;
        }
    }
    else
    {
        pstHttpAlyCtrl->bIsServer = TRUE;
        eRet = _HTTP_ALY_AlyRequestLine(pstHttpAlyCtrl, pcFirstLine, uiFirstLineLen);
        if (BS_OK != eRet)
        {
            return eRet;
        }
    }

    return BS_OK;
}

/***************************************************
 说明     :  解析所有的HeadField. pcHeadFiled以'\0'结束, 解析后将key-value对插入链表
 输入     :  pstHttpAlyCtrl: HTTP ALY实例
             pcHead: HTTP头的起始地址
             pcHeadField: HTTP头中Head Field起始位置
 输出     :  None
 返回值   :  成功: BS_OK
             失败: 错误码
 注意     :  None
****************************************************/
static BS_STATUS http_aly_ParseHeadField
(
    IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl,
    IN CHAR *pcHead,
    IN CHAR *pcHeadFiled
)
{
    _HTTP_ALY_KEY_S *pstHttpAlyKey;
    CHAR *pcLineNext = pcHeadFiled;
    CHAR *pcBufTmp = pcLineNext;
    UINT ulLineLen;
    BOOL_T bIsFoundLineEnd;
    CHAR *pcFoundSplite;
    DLL_HEAD_S *pstListHead;

    if (pstHttpAlyCtrl->bIsServer == TRUE)
    {
        pstListHead = &pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField;
    }
    else
    {
        pstListHead = &pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue;
    }

    /*取得参数列表*/
    while (pcBufTmp)
    {
        TXT_GetLine(pcBufTmp, &ulLineLen, &bIsFoundLineEnd, &pcLineNext);

        pstHttpAlyKey = MEM_ZMalloc(sizeof(_HTTP_ALY_KEY_S) + ulLineLen + 1);
        if (pstHttpAlyKey == NULL)
        {
            BS_WARNNING(("No memory!"));
            RETURN(BS_NO_MEMORY);
        }

        MEM_Copy((CHAR*)(pstHttpAlyKey+1), pcBufTmp, ulLineLen);
        pstHttpAlyKey->pucKey = (UCHAR*)(pstHttpAlyKey+1);
        pstHttpAlyKey->pucKey[ulLineLen] = '\0';
        
        pcFoundSplite = TXT_Strnchr((void*)pstHttpAlyKey->pucKey, ':', ulLineLen);
        if (pcFoundSplite == NULL)
        {
            MEM_Free(pstHttpAlyKey);
            BS_WARNNING(("Bad http line!"));
            RETURN(BS_NOT_SUPPORT);
        }
        *pcFoundSplite = '\0';

        pstHttpAlyKey->pucKeyValue = (UCHAR*)pcFoundSplite + 1;
        while (*(pstHttpAlyKey->pucKeyValue) == ' ')
        {
            pstHttpAlyKey->pucKeyValue++;
        }

        pstHttpAlyKey->ulValueOffsetInHead = (pstHttpAlyKey->pucKeyValue - pstHttpAlyKey->pucKey)
                                            + (pcBufTmp - pcHead);
        pstHttpAlyKey->ulKeyOffsetInHead = pcBufTmp - pcHead;
        
        DLL_ADD(pstListHead, &pstHttpAlyKey->stDllNode);

        pcBufTmp = pcLineNext;
    }

    return BS_OK;
}

/***************************************************
 说明     :  解析请求的常用参数
 输入     :  pstHttpAlyCtrl: HTTP ALY实例
 输出     :  None
 返回值   :  成功: BS_OK
             失败: 错误码
 注意     :  None
****************************************************/
static BS_STATUS http_aly_ParseRequestCommonParams
(
    IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl
)
{
    CHAR *pszCookie;
    CHAR *pcBufTmp;

    pstHttpAlyCtrl->stHttpAlyRequestInfo.pucHost = (UCHAR*)HTTP_ALY_GetField(pstHttpAlyCtrl, "Host");
    if (pstHttpAlyCtrl->stHttpAlyRequestInfo.pucHost)
    {
        TXT_Lower((CHAR*)pstHttpAlyCtrl->stHttpAlyRequestInfo.pucHost);
    }

    pszCookie = HTTP_ALY_GetField(pstHttpAlyCtrl, "Cookie");
    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stCookie);
	if (NULL != pszCookie)
	{
        _HTTP_ALY_ParseCookie (pszCookie, &pstHttpAlyCtrl->stHttpAlyRequestInfo.stCookie);
	}

    pstHttpAlyCtrl->stHttpAlyRequestInfo.pucReferer = (UCHAR*)HTTP_ALY_GetField(pstHttpAlyCtrl, "Referer");
    pcBufTmp = HTTP_ALY_GetField(pstHttpAlyCtrl, "Proxy-Connection");
    if (pcBufTmp != NULL)
    {
        pstHttpAlyCtrl->stHttpAlyRequestInfo.bIsProxyConnection = TRUE;
        if (strcmp(pcBufTmp, "Keep-Alive") == 0)
        {
            pstHttpAlyCtrl->stHttpAlyRequestInfo.bKeepAlive = TRUE;
        }
        else
        {
            pstHttpAlyCtrl->stHttpAlyRequestInfo.bKeepAlive = FALSE;
        }
    }
    else
    {
        pstHttpAlyCtrl->stHttpAlyRequestInfo.bIsProxyConnection = FALSE;
        pcBufTmp = HTTP_ALY_GetField(pstHttpAlyCtrl, "Connection");
        if (pcBufTmp == NULL)
        {
            pstHttpAlyCtrl->stHttpAlyRequestInfo.bKeepAlive = FALSE;
        }
        else
        {
            if (strcmp(pcBufTmp, "Keep-Alive") == 0)
            {
                pstHttpAlyCtrl->stHttpAlyRequestInfo.bKeepAlive = TRUE;
            }
            else
            {
                pstHttpAlyCtrl->stHttpAlyRequestInfo.bKeepAlive = FALSE;
            }
        }
    }
    
    pcBufTmp = HTTP_ALY_GetField(pstHttpAlyCtrl, "Content-Length");
    if (pcBufTmp != NULL)
    {
        sscanf(pcBufTmp, "%u", &pstHttpAlyCtrl->stHttpAlyRequestInfo.ulBodyLen);
        pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType = HTTP_ALY_BODY_LEN_KNOWN;
    }
    else if (strcmp("GET", (CHAR*)pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod) == 0)  /*GET  没有体*/
    {
        pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType = HTTP_ALY_BODY_LEN_KNOWN;
        pstHttpAlyCtrl->stHttpAlyRequestInfo.ulBodyLen = 0;
    }
    else if (((pcBufTmp = HTTP_ALY_GetField(pstHttpAlyCtrl, "transfer-encoding")) != 0)
        && (0 == stricmp(pcBufTmp, "chunked")))
    {
        pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType = HTTP_ALY_BODY_LEN_TRUNKED;
    }
    else if (((pcBufTmp = HTTP_ALY_GetField(pstHttpAlyCtrl, "Connection")) != 0)
        && (0 == stricmp(pcBufTmp, "close")))
    {
        pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType = HTTP_ALY_BODY_LEN_CLOSED;
    }
    else
    {
        BS_WARNNING(("Bad body len type"));
        pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType = HTTP_ALY_BODY_LEN_UNKNOWN;
    }

    pstHttpAlyCtrl->stHttpAlyRequestInfo.eMethod =
        HTTP_ALY_GetMethodByString((CHAR*)pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod);

    return BS_OK;
}

/***************************************************
 说明     :  解析应答的常用参数
 输入     :  pstHttpAlyCtrl: HTTP ALY实例
 输出     :  None
 返回值   :  成功: BS_OK
             失败: 错误码
 注意     :  None
****************************************************/
static BS_STATUS http_aly_ParseResponseCommonParams
(
    IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl
)
{
    CHAR *pcBufTmp;

    pcBufTmp = HTTP_ALY_GetField(pstHttpAlyCtrl, "Connection");
    if (pcBufTmp == NULL)
    {
        pstHttpAlyCtrl->stHttpAlyRequestInfo.bKeepAlive = FALSE;
    }
    else
    {
        if (strcmp(pcBufTmp, "Keep-Alive") == 0)
        {
            pstHttpAlyCtrl->stHttpAlyRequestInfo.bKeepAlive = TRUE;
        }
        else
        {
            pstHttpAlyCtrl->stHttpAlyRequestInfo.bKeepAlive = FALSE;
        }
    }

    if (HTTP_ALY_STATUS_CODE_OK != HTTP_ALY_GetResponseStatusCode(pstHttpAlyCtrl))   /*不是200的应答  没有体*/
    {
        pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType = HTTP_ALY_BODY_LEN_KNOWN;
        pstHttpAlyCtrl->stHttpAlyRequestInfo.ulBodyLen = 0;
    }

    return BS_OK;
}


/***************************************************
 说明     :  解析常用的参数
 输入     :  pstHttpAlyCtrl: HTTP ALY实例
 输出     :  None
 返回值   :  成功: BS_OK
             失败: 错误码
 注意     :  None
****************************************************/
static BS_STATUS http_aly_ParseCommonParams
(
    IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl
)
{
    if (pstHttpAlyCtrl->bIsServer == TRUE)
    {
        return http_aly_ParseRequestCommonParams(pstHttpAlyCtrl);
    }
    else
    {
        return http_aly_ParseResponseCommonParams(pstHttpAlyCtrl);
    }

}

BS_STATUS HTTP_ALY_AlyHead(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    CHAR *pcLineNext;
    CHAR *pcBufTmp;
    UINT ulLineLen;
    BOOL_T bIsFoundLineEnd;
    UINT ulLen;
    CHAR *pcBuf;

    if (! HTTP_ALY_IS_HEAD_RECVED(pstHttpAlyCtrl))
    {
        RETURN(BS_NOT_COMPLETE);
    }

    if (BS_OK != MBUF_MakeContinue (pstHttpAlyCtrl->stReadData.pstHeadMbuf, pstHttpAlyCtrl->stHttpAlyRequestInfo.ulHeadLen))
    {
        RETURN(BS_ERR);
    }

    pcBuf = MBUF_MTOD(pstHttpAlyCtrl->stReadData.pstHeadMbuf);
	pcBufTmp = pcBuf;

    ulLen = pstHttpAlyCtrl->stHttpAlyRequestInfo.ulHeadLen;
    
    pcBuf[ulLen - 4] = '\0';

    /*get first line*/
    TXT_GetLine(pcBufTmp, &ulLineLen, &bIsFoundLineEnd, &pcLineNext);

    /* 解析first line */
    if ((BS_OK != http_aly_ParseFirstLine(pstHttpAlyCtrl, pcBufTmp, ulLineLen))
        || (BS_OK != http_aly_ParseHeadField(pstHttpAlyCtrl, pcBuf, pcLineNext))
        || (BS_OK != http_aly_ParseCommonParams(pstHttpAlyCtrl)))
    {
        pcBuf[ulLen - 4] = '\r';
        RETURN(BS_ERR);
    }

    pcBuf[ulLen - 4] = '\r';

    return BS_OK;
}

/* return BS_OK; BS_ERR; BS_NOT_COMPLETE */
/* 返回的长度包含chunk字段 */
static BS_STATUS _HTTP_ALY_GetTrunkBodyLen(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl, OUT UINT *pulBodyLen)
{
    UINT ulChunkDataLen;
    UINT ulLen;
    UINT ulOffset;
    CHAR *pszSplit;
    UINT ulChunkedFlagLen;
    UINT ulCutLen;

    *pulBodyLen = 0;
    
    ulOffset = pstHttpAlyCtrl->stReadData.ulScanOffset - pstHttpAlyCtrl->stReadData.ulBeginOffset;
    ulLen = MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstBodyMbuf) - ulOffset;
    if (pstHttpAlyCtrl->stReadData.ulChunkedEndLen > ulLen)
    {
        pstHttpAlyCtrl->stReadData.ulChunkedEndLen -= ulLen;
        pstHttpAlyCtrl->stReadData.ulScanOffset += ulLen;
        if (pstHttpAlyCtrl->stReadData.bIfRemoveChunkFlag == TRUE)
        {
            if (pstHttpAlyCtrl->stReadData.ulChunkedEndLen == 1)
            {
                /* 还有一个字节没有收完,表示已经收到的最后一个字节是"\r\n"中的'\r', 应该去掉 */
                MBUF_CutPart(pstHttpAlyCtrl->stReadData.pstBodyMbuf, 
                    MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstBodyMbuf) - 1,
                    1);
                pstHttpAlyCtrl->stReadData.ulScanOffset -= 1;
            }
        }
        RETURN(BS_NOT_COMPLETE);
    }
    else
    {
        pstHttpAlyCtrl->stReadData.ulScanOffset += pstHttpAlyCtrl->stReadData.ulChunkedEndLen;

        if (pstHttpAlyCtrl->stReadData.bIfRemoveChunkFlag == TRUE)
        {
            /* 砍掉chunked块后面的"\r\n" */
            ulCutLen = MIN(pstHttpAlyCtrl->stReadData.ulChunkedEndLen, 2);
            MBUF_CutPart(pstHttpAlyCtrl->stReadData.pstBodyMbuf,
                ulOffset + pstHttpAlyCtrl->stReadData.ulChunkedEndLen -ulCutLen,  ulCutLen);
            pstHttpAlyCtrl->stReadData.ulScanOffset -= ulCutLen;            
        }

        pstHttpAlyCtrl->stReadData.ulChunkedEndLen = 0;        
    }

    while (1)
    {
        ulOffset = pstHttpAlyCtrl->stReadData.ulScanOffset - pstHttpAlyCtrl->stReadData.ulBeginOffset;

        if (pstHttpAlyCtrl->stReadData.bIsRecvChunkedFlagOk == FALSE)
        {
            /* 标记存在且没有收完,则拷贝chunked标记 */
            ulLen = MIN(sizeof(pstHttpAlyCtrl->stReadData.szChunkedString) - pstHttpAlyCtrl->stReadData.ulChunkedStringLen - 1,
                    MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstBodyMbuf) - ulOffset);
    
            if (ulLen == 0)
            {
                RETURN(BS_NOT_COMPLETE);
            }

            MBUF_CopyFromMbufToBuf(pstHttpAlyCtrl->stReadData.pstBodyMbuf,
                ulOffset, ulLen,
                pstHttpAlyCtrl->stReadData.szChunkedString + pstHttpAlyCtrl->stReadData.ulChunkedStringLen);

            pszSplit = strstr(pstHttpAlyCtrl->stReadData.szChunkedString, "\r\n") ;
    
            if (pszSplit != NULL)
            {
                pstHttpAlyCtrl->stReadData.bIsRecvChunkedFlagOk = TRUE;

                /* 计算chunked标记长度, 要加上最后的"\r\n" */
                ulLen = (pszSplit - pstHttpAlyCtrl->stReadData.szChunkedString + 2);

                ulChunkedFlagLen = ulLen - pstHttpAlyCtrl->stReadData.ulChunkedStringLen;
                pstHttpAlyCtrl->stReadData.ulChunkedStringLen = ulLen;
    
                *pszSplit = '\0';

                if (pstHttpAlyCtrl->stReadData.bIfRemoveChunkFlag)
                {
                    MBUF_CutPart(pstHttpAlyCtrl->stReadData.pstBodyMbuf, ulOffset, ulChunkedFlagLen);
                }
                else
                {
                    pstHttpAlyCtrl->stReadData.ulScanOffset += ulChunkedFlagLen;
                }
            }
            else
            {
                if (pstHttpAlyCtrl->stReadData.bIfRemoveChunkFlag)
                {
                    MBUF_CutPart(pstHttpAlyCtrl->stReadData.pstBodyMbuf, ulOffset, ulLen);
                }
                else
                {
                    pstHttpAlyCtrl->stReadData.ulScanOffset += ulLen;
                }

                pstHttpAlyCtrl->stReadData.ulChunkedStringLen += ulLen;
            }
        }

        /* if chunked标记存在且没有收完 */
        if (pstHttpAlyCtrl->stReadData.bIsRecvChunkedFlagOk == FALSE)
        {
            RETURN(BS_NOT_COMPLETE);
        }

        /* if chunked标记存在且收完. */
        if ((pstHttpAlyCtrl->stReadData.bIsRecvChunkedFlagOk == TRUE) && (pstHttpAlyCtrl->stReadData.ulChunkedStringLen != 0))
        {
            if (pstHttpAlyCtrl->stReadData.ulChunkedStringLen > sizeof("ffffffff\r\n") - 1)    /* chunked块长度超过4G */
            {
                RETURN(BS_NOT_SUPPORT);
            }

            TXT_XAtoui(pstHttpAlyCtrl->stReadData.szChunkedString, &ulChunkDataLen);

            if (ulChunkDataLen == 0)
            {
                pstHttpAlyCtrl->stReadData.bIsLastChunk = TRUE;
            }

            ulChunkDataLen += 2;    /* 加上每个Chunked块后面的"\r\n"的长度 */
            
            pstHttpAlyCtrl->stReadData.ulChunkedEndLen = ulChunkDataLen;
            pstHttpAlyCtrl->stReadData.ulChunkedLen = ulChunkDataLen;
            pstHttpAlyCtrl->stReadData.bIsRecvChunkedFlagOk = FALSE;
            pstHttpAlyCtrl->stReadData.ulChunkedStringLen = 0;
        }

        ulOffset = pstHttpAlyCtrl->stReadData.ulScanOffset - pstHttpAlyCtrl->stReadData.ulBeginOffset;

        if (pstHttpAlyCtrl->stReadData.ulChunkedEndLen > MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstBodyMbuf) - ulOffset)
        {
            pstHttpAlyCtrl->stReadData.ulChunkedEndLen -= MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstBodyMbuf) - ulOffset;
            pstHttpAlyCtrl->stReadData.ulScanOffset += MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstBodyMbuf) - ulOffset;
            RETURN(BS_NOT_COMPLETE);
        }

        pstHttpAlyCtrl->stReadData.ulScanOffset += pstHttpAlyCtrl->stReadData.ulChunkedEndLen;

        if (pstHttpAlyCtrl->stReadData.bIsLastChunk == TRUE)
        {
            *pulBodyLen = pstHttpAlyCtrl->stReadData.ulScanOffset;
            return BS_OK;
        }
        pstHttpAlyCtrl->stReadData.ulChunkedEndLen = 0;
        pstHttpAlyCtrl->stReadData.bIsRecvChunkedFlagOk = FALSE;
        pstHttpAlyCtrl->stReadData.ulChunkedStringLen = 0;
        pstHttpAlyCtrl->stReadData.ulChunkedStringOffset += pstHttpAlyCtrl->stReadData.ulChunkedStringLen + pstHttpAlyCtrl->stReadData.ulChunkedLen;
    }

    return BS_OK;
}

/*return BS_OK, BS_NOT_COMPLETE, Other...*/
static BS_STATUS _HTTP_ALY_ReadData(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl, OUT MBUF_S **ppstMbuf)
{
    MBUF_S *pstMbuf = NULL;
    MBUF_CLUSTER_S *pstCluster;
    UCHAR *pucBuf;
    UINT ulReservedLen = 0;
    UINT ulReadLen;
    HTTP_ALY_READ_RET_E eRet;

    do {
        pstCluster = MBUF_CreateCluster();
        if (pstCluster == NULL)
        {
            RETURN(BS_NO_MEMORY);
        }

        pucBuf = pstCluster->pucData;
        ulReservedLen = MBUF_CLUSTER_SIZE(pstCluster);

        eRet = pstHttpAlyCtrl->pfReadFunc(pstHttpAlyCtrl->ulFd, pucBuf, ulReservedLen, &ulReadLen);

        if ((eRet != HTTP_ALY_CONTINUE) && (eRet != HTTP_ALY_STOP))
        {
            MBUF_FreeCluster(pstCluster);
            RETURN(BS_ERR);
        }

        if (ulReadLen == 0)
        {
            MBUF_FreeCluster(pstCluster);
        }
        else
        {
            if (pstMbuf != NULL)
            {
                MBUF_AddCluster (pstMbuf, pstCluster, 0, ulReadLen);
            }
            else
            {
                pstMbuf = MBUF_CreateByCluster(pstCluster, 0, ulReadLen, MBUF_DATA_DATA);
                if (NULL == pstMbuf)
                {
                    MBUF_FreeCluster (pstCluster);
                    RETURN(BS_NO_MEMORY);
                }
            }
        }
    }while (HTTP_ALY_CONTINUE == eRet);

    if (pstMbuf == NULL)
    {
        RETURN(BS_NOT_COMPLETE);
    }

    *ppstMbuf = pstMbuf;

    return BS_OK;
}

/*return BS_OK; BS_ERR; BS_NOT_COMPLETE*/
static BS_STATUS _HTTP_ALY_IsBodyFinAndFraIt(_HTTP_ALY_CTRL_S *pstHttpAlyCtrl)
{
    UINT ulBodyLen = 0;
    BS_STATUS eRet;
    MBUF_S *pstMbuf;

    pstMbuf = pstHttpAlyCtrl->stReadData.pstBodyMbuf;
    
    /*if 是chunk, 调用_HTTP_ALY_IsChunkBodyFinish*/
    if (pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType == HTTP_ALY_BODY_LEN_TRUNKED)
    {
        if (NULL == pstMbuf)
        {
            RETURN(BS_NOT_COMPLETE);
        }
        
        eRet = _HTTP_ALY_GetTrunkBodyLen(pstHttpAlyCtrl, &ulBodyLen);

        if (BS_NOT_COMPLETE == RETCODE(eRet))
        {
            RETURN(BS_NOT_COMPLETE);
        }

        if (BS_OK != eRet)
        {
            RETURN(BS_ERR);
        }
        
        pstHttpAlyCtrl->stHttpAlyRequestInfo.ulBodyLen = ulBodyLen;
    }
    /*else if 是ContentLen, 判断是否OK*/
    else if (pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType == HTTP_ALY_BODY_LEN_KNOWN)
    {
        ulBodyLen = pstHttpAlyCtrl->stHttpAlyRequestInfo.ulBodyLen;

        if (ulBodyLen > 0)
        {
            if (NULL == pstMbuf)
            {
                RETURN(BS_NOT_COMPLETE);
            }
            
            if (ulBodyLen > pstHttpAlyCtrl->stReadData.ulRecvBodyLen)
            {
                RETURN(BS_NOT_COMPLETE);
            }
        }
    }
    /*else if 是connect-close, 是否err*/
    else if (pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType == HTTP_ALY_BODY_LEN_CLOSED)
    {
        if (pstHttpAlyCtrl->stReadData.bIsConnClosed == FALSE)
        {
            RETURN(BS_NOT_COMPLETE);
        }
    }
    else
    {
        RETURN(BS_ERR);
    }

    /* cut mbuf, 将剩余的部分存到nextHttpData */

    pstHttpAlyCtrl->stReadData.bIsBodyFin = TRUE;

    if (NULL == pstMbuf)
    {
        return BS_OK;
    }

    if (ulBodyLen == 0)
    {
        pstHttpAlyCtrl->stReadData.pstBodyMbuf = NULL;
        pstHttpAlyCtrl->stReadData.pstNextHttpMbuf = pstMbuf;
    }
    else if (ulBodyLen > 0)
    {
        if (ulBodyLen < MBUF_TOTAL_DATA_LEN(pstMbuf))
        {
            pstHttpAlyCtrl->stReadData.pstBodyMbuf = MBUF_Fragment(pstMbuf, ulBodyLen);
            pstHttpAlyCtrl->stReadData.pstNextHttpMbuf = pstMbuf;
        }
    }

    return BS_OK;
}

static BS_STATUS _HTTP_ALY_DelChunkCode(_HTTP_ALY_CTRL_S *pstHttpAlyCtrl)
{
    INT lTrunkCodeEndOff;
    INT lTrunkCodeStartOff = 0;
    UINT ulChunkDataLen;
    CHAR szChunk[10];

    if (! HTTP_ALY_IS_BODY_RECVED(pstHttpAlyCtrl))
    {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    do {
        lTrunkCodeEndOff = MBUF_Find(pstHttpAlyCtrl->stReadData.pstBodyMbuf, lTrunkCodeStartOff, (UCHAR*)"\r\n", 2);
        if (lTrunkCodeEndOff <= lTrunkCodeStartOff)
        {
            RETURN(BS_NOT_COMPLETE);
        }

        if (lTrunkCodeEndOff - lTrunkCodeStartOff > sizeof("ffffffff") - 1)
        {
            RETURN(BS_ERR);
        }

        MBUF_CopyFromMbufToBuf(pstHttpAlyCtrl->stReadData.pstBodyMbuf,
            lTrunkCodeStartOff, lTrunkCodeEndOff - lTrunkCodeStartOff, szChunk);

        szChunk[lTrunkCodeEndOff - lTrunkCodeStartOff] = '\0';
        TXT_Atoui(szChunk, &ulChunkDataLen);

        MBUF_CutHead(pstHttpAlyCtrl->stReadData.pstBodyMbuf, lTrunkCodeEndOff - lTrunkCodeStartOff + 2);
        
        lTrunkCodeStartOff += ulChunkDataLen;

        MBUF_CutPart(pstHttpAlyCtrl->stReadData.pstBodyMbuf, lTrunkCodeStartOff, 2);    /* 去掉chunk后的回车 */
    }while(ulChunkDataLen != 0);

    pstHttpAlyCtrl->stHttpAlyRequestInfo.ulBodyLen = MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstBodyMbuf);

    return BS_OK;
}

static BS_STATUS _HTTP_ALY_SaveProtoData(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl, IN UCHAR *pucSendBuf, IN UINT ulShouldSendLen)
{
    /* HTTP协议数据没有发送完成,  记录到发送缓冲中, 等待下次发送*/
    if (BS_OK != VBUF_CatFromBuf(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf, pucSendBuf, ulShouldSendLen))
    {
        RETURN(BS_ERR);
    }
    
    return BS_OK;
}

/* 
* return BS_OK; BS_NOT_COMPLETE, BS_ERR ...
*/
static BS_STATUS _HTTP_ALY_SendHttpProtoData(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl, IN UCHAR *pucSendBuf, IN UINT ulShouldSendLen)
{
    UINT ulWriteLen = 0;

    if (VBUF_GetDataLength(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf) > 0)
    {
        /* HTTP协议数据没有发送完成,  记录到发送缓冲中, 等待下次发送*/
        if (BS_OK != _HTTP_ALY_SaveProtoData(pstHttpAlyCtrl, pucSendBuf, ulShouldSendLen))
        {
            RETURN(BS_ERR);
        }

        return BS_NOT_COMPLETE;
    }
    
    if (BS_OK != SSLTCP_Write(pstHttpAlyCtrl->ulFd, pucSendBuf, ulShouldSendLen, &ulWriteLen))
    {
        RETURN(BS_ERR);
    }
    
    if (ulShouldSendLen > ulWriteLen)
    {
        VBUF_Init(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf);
        /* HTTP协议数据没有发送完成,  记录到发送缓冲中, 等待下次发送*/
        if (BS_OK != _HTTP_ALY_SaveProtoData(pstHttpAlyCtrl, pucSendBuf + ulWriteLen, ulShouldSendLen - ulWriteLen))
        {
            RETURN(BS_ERR);
        }
        
        return BS_NOT_COMPLETE;
    }

    return BS_OK;
}

static BS_STATUS _HTTP_ALY_UnCode(IN UCHAR *pucCode, OUT UCHAR *pucUnCode)
{
    UCHAR *pucFind;
    UCHAR *pucStart;
    UINT ulLen;
    UINT ulDstOffset = 0;
    UCHAR ucChar;
    
    BS_DBGASSERT(NULL != pucCode);
    BS_DBGASSERT(NULL != pucUnCode);

    pucStart = pucCode;
    ulLen = strlen((CHAR*)pucCode);

    do  {
        pucFind = (UCHAR*)strchr((CHAR*)pucStart ,'%');
        if (NULL == pucFind)
        {
            TXT_StrCpy((CHAR*)pucUnCode + ulDstOffset, (CHAR*)pucStart);
            ulDstOffset += ulLen - (pucStart - pucCode);
            break;
        }

        if (pucFind - pucCode + 3 > (INT)ulLen)
        {
            RETURN(BS_ERR);
        }

        MEM_Copy(pucUnCode + ulDstOffset, pucStart, pucFind - pucStart);
        ulDstOffset += pucFind - pucStart;

        pucFind++;

        if (BS_OK != HEX_2_UCHAR((CHAR*)pucFind, &ucChar))
        {
            RETURN(BS_ERR);
        }

        pucUnCode[ulDstOffset] = ucChar;
        ulDstOffset++;

        pucFind += 2;
        pucStart = pucFind;
    }while (pucFind != NULL);

    pucUnCode[ulDstOffset] = '\0';

    return BS_OK;
}

static _HTTP_ALY_KEY_S * _HTTP_ALY_GetField(_HTTP_ALY_CTRL_S *pstHttpAlyCtrl, IN CHAR *pszFieldName)
{
    _HTTP_ALY_KEY_S *pstHttpAlyKey;

    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField, pstHttpAlyKey)
    {
        if (stricmp(pszFieldName, (CHAR*)pstHttpAlyKey->pucKey) == 0)
        {
            return pstHttpAlyKey;
        }
    }

    return NULL;
}

/* 修改接受数据描述 */
static VOID _HTTP_ALY_ModifyReadDes(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl, IN UINT ulReadOutLen)
{
    pstHttpAlyCtrl->stReadData.ulBeginOffset += ulReadOutLen;
}

#if 0
/* 发送缓冲区中的数据 */
static BS_STATUS _HTTP_ALY_SendBufData(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl)
{
    UCHAR *pucData;
    UINT uiLen;
    HANDLE hSendBuf;
    UINT uiWriteLen;

    hSendBuf = &pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf;
    
    if (hSendBuf != NULL)
    {
        pucData = VBUF_GetData(hSendBuf);
        uiLen = VBUF_GetDataLength(hSendBuf);
        if (BS_OK != SSLTCP_Write(pstHttpAlyCtrl->ulFd, pucData, uiLen, &uiWriteLen))
        {
            RETURN(BS_ERR);
        }
        VBUF_CutHead(hSendBuf, uiWriteLen);

        if (VBUF_GetDataLength(hSendBuf) > 0)
        {
            RETURN(BS_NOT_COMPLETE);
        }
        else
        {
            VBUF_Finit(hSendBuf);
        }        
    }

    return BS_OK;
}
#endif

static BS_STATUS _HTTP_ALY_SplitBody(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl)
{
    INT lHeadLen;
    MBUF_S *pstMbuf;
    BS_STATUS eRet;

    pstMbuf = pstHttpAlyCtrl->stReadData.pstHeadMbuf;

    if (pstMbuf != NULL)
    {
        lHeadLen = MBUF_Find(pstMbuf, 0, (UCHAR*)"\r\n\r\n", sizeof("\r\n\r\n")-1);
        if (lHeadLen == 0)
        {
            RETURN(BS_ERR);
        }
        else if (lHeadLen >= 0)
        {
            pstHttpAlyCtrl->stReadData.bIsHeadFin = TRUE;
            pstHttpAlyCtrl->stHttpAlyRequestInfo.ulHeadLen = (UINT)lHeadLen + 4;

            /* 将头和体分开 */
            if (MBUF_TOTAL_DATA_LEN(pstMbuf) > pstHttpAlyCtrl->stHttpAlyRequestInfo.ulHeadLen)
            {
                pstHttpAlyCtrl->stReadData.pstHeadMbuf = MBUF_Fragment(pstMbuf,pstHttpAlyCtrl->stHttpAlyRequestInfo.ulHeadLen);
                pstHttpAlyCtrl->stReadData.pstBodyMbuf = pstMbuf;
                pstHttpAlyCtrl->stReadData.ulRecvBodyLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
            }

            if (BS_OK != HTTP_ALY_AlyHead(pstHttpAlyCtrl))
            {
                RETURN(BS_ERR);
            }

            eRet = _HTTP_ALY_IsBodyFinAndFraIt(pstHttpAlyCtrl);
            if ((BS_OK != eRet) && (BS_NOT_COMPLETE != RETCODE(eRet)))
            {
                RETURN(BS_ERR);
            }
            
            return BS_OK;
        }
        else if (MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstHeadMbuf) 
            >= MBUF_GET_DATABLOK_SIZE_BY_MBUF(pstHttpAlyCtrl->stReadData.pstHeadMbuf))
        {
            RETURN(BS_NOT_SUPPORT);
        }
    }

    return BS_NOT_COMPLETE;
}

/* 构造回应头并放到发送缓冲区中 */
static BS_STATUS _HTTP_ALY_FormatResponseHeader(IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl)
{
    CHAR szHeader[HTTP_ALY_MAX_HEAD_LEN + 1];
    CHAR *pszHeader;
    HTTP_PROTOCOL_VER_E eResponseVersion;
    CHAR * pszResponseVersion;
    UCHAR ucResponseMode;
    CHAR *pszResponseContentLen;
    UINT uiLen;
    _HTTP_ALY_KEY_S *pstNode;
    _HTTP_ALY_COOKIE_S *pstCookieKey;
    BOOL_T bIsRequestKeepAlive;
    BOOL_T bIsResponseKeepAlive = FALSE;
    UINT uiStatusCode;

    pstHttpAlyCtrl->stHttpAlyResponseInfo.bIsSendHead = TRUE;

    /* 如果没有设置过StatusCode, 则默认设置为200 OK */
    if (pstHttpAlyCtrl->stHttpAlyResponseInfo.ulStatusCode == 0)
    {
        HTTP_ALY_SetResponseStatusCode(pstHttpAlyCtrl, HTTP_ALY_STATUS_CODE_OK, "OK");
    }

    uiStatusCode = HTTP_ALY_GetResponseStatusCode(pstHttpAlyCtrl);
    eResponseVersion = HTTP_ALY_GetRequestVer(pstHttpAlyCtrl);
    pszResponseVersion = HTTP_ALY_GetProtocol(pstHttpAlyCtrl);
    bIsRequestKeepAlive = HTTP_ALY_IsRequestKeepAlive(pstHttpAlyCtrl);
    pszResponseContentLen = HTTP_ALY_GetResponseKeyValue(pstHttpAlyCtrl, "Content-Length");
    

    if ((bIsRequestKeepAlive == FALSE) || (HTTP_ALY_STATUS_CODE_OK != uiStatusCode))
    {
        ucResponseMode = HTTP_ALY_RESPONSE_MODE_CLOSED;
        bIsResponseKeepAlive = FALSE;
    }
    else
    {
        /* 确定回应模式:Closed/Length/Chunked */
        if (pszResponseContentLen == NULL)
        {
            switch (eResponseVersion)
            {
                case HTTP_PROTOCOL_VER_10:
                case HTTP_PROTOCOL_VER_UNKNOWN:
                    ucResponseMode = HTTP_ALY_RESPONSE_MODE_CLOSED;
                    bIsResponseKeepAlive = FALSE;
                    break;
                
                case HTTP_PROTOCOL_VER_11:
                    ucResponseMode = HTTP_ALY_RESPONSE_MODE_CHUNKED;
                    bIsResponseKeepAlive = TRUE;
                    break;

                default:
                    ucResponseMode = HTTP_ALY_RESPONSE_MODE_CLOSED;
                    bIsResponseKeepAlive = FALSE;
                    BS_DBGASSERT(0);
                    break;
            }
        }
        else
        {
            ucResponseMode = HTTP_ALY_RESPONSE_MODE_LENGTH;
            bIsResponseKeepAlive = TRUE;
        }
    }

    pstHttpAlyCtrl->stHttpAlyResponseInfo.ucResponseMode = ucResponseMode;

    if (ucResponseMode == HTTP_ALY_RESPONSE_MODE_CHUNKED)
    {
        HTTP_ALY_SetResponseHeadField(pstHttpAlyCtrl, "Transfer-Encoding", "chunked");
    }

    HTTP_ALY_SetResponseKeepAlive(pstHttpAlyCtrl, bIsResponseKeepAlive);

    /* 补充一些必要的key-value */
    {
        /* 如果没有设置时间,则客户端最好不进行缓存 */
        if (NULL == HTTP_ALY_GetResponseKeyValue(pstHttpAlyCtrl, "Last-Modified"))
        {
            HTTP_ALY_SetNoCache(pstHttpAlyCtrl);
        }
    }

    pszHeader = szHeader;

    /* 构造应答行 */
    uiLen = snprintf(pszHeader, HTTP_ALY_MAX_HEAD_LEN, "%s %d %s\r\n",
                pszResponseVersion,
                pstHttpAlyCtrl->stHttpAlyResponseInfo.ulStatusCode,
                pstHttpAlyCtrl->stHttpAlyResponseInfo.szStatusDesc);

    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue, pstNode)
    {
        uiLen += snprintf(pszHeader + uiLen, HTTP_ALY_MAX_HEAD_LEN - uiLen, "%s: %s\r\n",
                    pstNode->pucKey, pstNode->pucKeyValue);
    }

    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stCookie, pstCookieKey)
    {
        uiLen += snprintf(pszHeader + uiLen, HTTP_ALY_MAX_HEAD_LEN - uiLen, "Set-Cookie: %s=%s; path=%s\r\n",
                    pstCookieKey->pucKey,
                    pstCookieKey->pucKeyValue,
                    pstCookieKey->pszPath == NULL ? "/" : (CHAR*)pstCookieKey->pszPath);
    }

    uiLen += snprintf(pszHeader + uiLen, HTTP_ALY_MAX_HEAD_LEN - uiLen, "\r\n");

    VBUF_Init(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf);

    VBUF_CatFromBuf(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf, szHeader, uiLen);
    
    return BS_OK;
}

/* 发送缓存中的数据 */
static BS_STATUS _HTTP_ALY_SendBuffedData
(
    IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl
)
{
    UINT uiRemainLen;
    UCHAR *pucData;
    UINT uiWriteSize;

    pucData = VBUF_GetData(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf);
    if (NULL == pucData)
    {
        RETURN(BS_ERR);
    }

    uiRemainLen = VBUF_GetDataLength(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf);

    if (BS_OK != SSLTCP_Write(pstHttpAlyCtrl->ulFd, pucData, uiRemainLen, &uiWriteSize))
    {
        RETURN(BS_ERR);
    }

    VBUF_CutHead(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf, uiWriteSize);

    if (uiWriteSize < uiRemainLen)
    {
        return BS_NOT_COMPLETE;
    }

    return BS_OK;
}

static BS_STATUS _HTTP_ALY_SendDataNormal
(
    IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiWriteLen
)
{
    *puiWriteLen = 0;
    
    if (BS_OK != SSLTCP_Write(pstHttpAlyCtrl->ulFd, pucData, uiDataLen, puiWriteLen))
    {
        RETURN(BS_ERR);
    }

    if (uiDataLen > *puiWriteLen)
    {
        return BS_NOT_COMPLETE;
    }

    return BS_OK;
}

static BS_STATUS _HTTP_ALY_SendDataByChunk
(
    IN _HTTP_ALY_CTRL_S *pstHttpAlyCtrl,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiWriteLen
)
{
    UINT uiSendLen;
    UINT uiWriteSize;
    UINT uiRmainLen = uiDataLen;
    CHAR szContentLen[16];
    BS_STATUS eRet;

    *puiWriteLen = 0;
    
    for (;;)
    {
        /* 如果上一次的chunk数据没有发送完成,继续发送chunk数据块 */
        if (pstHttpAlyCtrl->stHttpAlyResponseInfo.ulChunkRmainLen > 0)
        {
            uiSendLen = MIN(pstHttpAlyCtrl->stHttpAlyResponseInfo.ulChunkRmainLen, uiDataLen);
            if (BS_OK != SSLTCP_Write(pstHttpAlyCtrl->ulFd, pucData, uiSendLen, &uiWriteSize))
            {
                RETURN(BS_ERR);
            }

            pstHttpAlyCtrl->stHttpAlyResponseInfo.ulChunkRmainLen -= uiWriteSize;

            *puiWriteLen += uiWriteSize;

            if (uiWriteSize < uiSendLen)
            {
                return BS_OK;
            }

            /* 发送完一次chunk数据，需要加换行 */
            if (pstHttpAlyCtrl->stHttpAlyResponseInfo.ulChunkRmainLen == 0)
            {
                eRet = _HTTP_ALY_SendHttpProtoData(pstHttpAlyCtrl, (UCHAR*)"\r\n", 2);
                if (eRet != BS_OK)
                {
                    return eRet;
                }
            }

            uiRmainLen -= uiWriteSize;
        }

        if (uiRmainLen == 0)
        {
            return BS_OK;
        }

        /* 发送chunk标记 */
        pstHttpAlyCtrl->stHttpAlyResponseInfo.ulChunkRmainLen = uiRmainLen;
        sprintf(szContentLen, "%x\r\n", uiRmainLen);
        eRet = _HTTP_ALY_SendHttpProtoData(pstHttpAlyCtrl, (UCHAR*)szContentLen, strlen(szContentLen));
        if (eRet != BS_OK)
        {
            return eRet;
        }
    }

    return BS_OK;
}

BS_STATUS HTTP_ALY_AddData(IN HANDLE hHandle, IN MBUF_S *pstMbuf)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    MBUF_CAT_EXT(pstHttpAlyCtrl->stReadData.pstHeadMbuf, pstMbuf);

    return _HTTP_ALY_SplitBody(pstHttpAlyCtrl);
}

HANDLE HTTP_ALY_Create()
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl;
    
    pstHttpAlyCtrl = MEM_Malloc(sizeof(_HTTP_ALY_CTRL_S));
    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("No memory!"));
        return 0;
    }

    Mem_Zero(pstHttpAlyCtrl, sizeof(_HTTP_ALY_CTRL_S));

    pstHttpAlyCtrl->pucBuf = MEM_Malloc(HTTP_PROTOCOL_MAX_HEAD_LEN + 1);
    if (pstHttpAlyCtrl->pucBuf == NULL)
    {
        MEM_Free(pstHttpAlyCtrl);
        BS_WARNNING(("Not enough memroy!"));
        return 0;
    }

    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField);
    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllKeyValueHead);
    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stCookie);
    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stCookie);
    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue);
    
    return pstHttpAlyCtrl;
}

BS_STATUS HTTP_ALY_Reset(IN HANDLE hHandle)
{
    DLL_NODE_S *pstNode, *pstNodeTmp;
    
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        RETURN(BS_NULL_PARA);
    }
    
    DLL_SAFE_SCAN(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllKeyValueHead, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllKeyValueHead, pstNode);
        MEM_Free(pstNode);
    }

    DLL_SAFE_SCAN(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField, pstNode);
        MEM_Free(pstNode);
    }

    DLL_SAFE_SCAN(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stCookie, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stCookie, pstNode);
        MEM_Free(pstNode);
    }

    DLL_SAFE_SCAN(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stCookie, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stCookie, pstNode);
        MEM_Free(pstNode);
    }

    DLL_SAFE_SCAN(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue, pstNode);
        MEM_Free(pstNode);
    }

    VBUF_Finit(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf);

    if (pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod)
    {
        MEM_Free(pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod);
        pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod = NULL;
    }
    if (pstHttpAlyCtrl->stHttpAlyRequestInfo.pucUrlPath)
    {
        MEM_Free(pstHttpAlyCtrl->stHttpAlyRequestInfo.pucUrlPath);
        pstHttpAlyCtrl->stHttpAlyRequestInfo.pucUrlPath = NULL;
    }
    if (pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString)
    {
        MEM_Free(pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString);
        pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString = NULL;
    }

    if (pstHttpAlyCtrl->stReadData.pstHeadMbuf)
    {
        MBUF_Free(pstHttpAlyCtrl->stReadData.pstHeadMbuf);
    }
	if (pstHttpAlyCtrl->stReadData.pstBodyMbuf)
    {
        MBUF_Free(pstHttpAlyCtrl->stReadData.pstBodyMbuf);
    }
	pstHttpAlyCtrl->stReadData.pstHeadMbuf = NULL;
    pstHttpAlyCtrl->stReadData.pstBodyMbuf = NULL;
	pstHttpAlyCtrl->stReadData.bIsHeadFin = FALSE;
	pstHttpAlyCtrl->stReadData.bIsBodyFin = FALSE;

    Mem_Zero(&pstHttpAlyCtrl->stHttpAlyRequestInfo, sizeof(_HTTP_REQUEST_INFO_S));
    Mem_Zero(&pstHttpAlyCtrl->stHttpAlyResponseInfo, sizeof(_HTTP_ALY_RESPONSE_INFO_S));

    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField);
    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllKeyValueHead);
    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stCookie);
    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stCookie);
    DLL_INIT(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue);
    pstHttpAlyCtrl->ulBufLen = 0;

    return BS_OK;
}


BS_STATUS HTTP_ALY_Delete(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        RETURN(BS_NULL_PARA);
    }

    HTTP_ALY_Reset(hHandle);

    MEM_Free(pstHttpAlyCtrl->pucBuf);
    MEM_Free(pstHttpAlyCtrl);

    return BS_OK;
}

BS_STATUS HTTP_ALY_SetReadFunc (IN HANDLE hHandle, IN PF_HTTP_ALY_READ pfFunc, IN UINT ulFd)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (NULL != pfFunc)
    {
        pstHttpAlyCtrl->pfReadFunc = pfFunc;
    }
    else
    {
        pstHttpAlyCtrl->pfReadFunc = _HTTP_ALY_DftReadFunc;
    }
    
    pstHttpAlyCtrl->ulFd       = ulFd;

    return BS_OK;
}

BS_STATUS HTTP_ALY_GetHead(IN HANDLE hHandle, OUT MBUF_S **ppstMbuf)
{
    MBUF_S *pstMbuf;
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    pstMbuf = pstHttpAlyCtrl->stReadData.pstHeadMbuf;

    pstHttpAlyCtrl->stReadData.pstHeadMbuf = NULL;

    if (NULL == pstMbuf)
    {
        RETURN(BS_NO_SUCH);
    }

    *ppstMbuf = pstMbuf;

    return BS_OK;
}

MBUF_S * HTTP_ALY_GetBody(IN HANDLE hHandle)
{
    MBUF_S *pstMbuf;
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    pstMbuf = pstHttpAlyCtrl->stReadData.pstBodyMbuf;

    pstHttpAlyCtrl->stReadData.pstBodyMbuf = NULL;

    if (NULL == pstMbuf)
    {
        return NULL;
    }

    _HTTP_ALY_ModifyReadDes(pstHttpAlyCtrl, MBUF_TOTAL_DATA_LEN(pstMbuf));

    return pstMbuf;
}

BS_STATUS HTTP_ALY_GetData(IN HANDLE hHandle, OUT MBUF_S **ppstMbuf)
{
    MBUF_S *pstMbuf;
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    pstMbuf = pstHttpAlyCtrl->stReadData.pstHeadMbuf;

    if (pstHttpAlyCtrl->stReadData.pstBodyMbuf != NULL)
    {
        _HTTP_ALY_ModifyReadDes(pstHttpAlyCtrl, MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstBodyMbuf));
        MBUF_CAT_EXT(pstMbuf, pstHttpAlyCtrl->stReadData.pstBodyMbuf);
    }

    pstHttpAlyCtrl->stReadData.pstBodyMbuf = NULL;
    pstHttpAlyCtrl->stReadData.pstHeadMbuf = NULL;

    *ppstMbuf = pstMbuf;

    return BS_OK;
}

/*
return BS_OK; BS_ERR;BS_NOT_COMPLETE
*/
BS_STATUS HTTP_ALY_TryHead (IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    MBUF_S *pstMbuf;
    BS_STATUS eRet;

    eRet = _HTTP_ALY_ReadData(pstHttpAlyCtrl, &pstMbuf);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    MBUF_CAT_EXT(pstHttpAlyCtrl->stReadData.pstHeadMbuf, pstMbuf);

    return _HTTP_ALY_SplitBody(pstHttpAlyCtrl);
}

BS_STATUS HTTP_ALY_ReadHead (IN HANDLE hHandle, OUT MBUF_S **ppMbuf)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    MBUF_S *pstMbuf;
    BS_STATUS eRet;

    if (! HTTP_ALY_IS_HEAD_RECVED(pstHttpAlyCtrl))
    {
        if (BS_OK != (eRet = HTTP_ALY_TryHead(hHandle)))
        {
            return eRet;
        }
    }

    pstMbuf = pstHttpAlyCtrl->stReadData.pstHeadMbuf;

    if (pstHttpAlyCtrl->stHttpAlyRequestInfo.ulHeadLen < MBUF_TOTAL_DATA_LEN(pstMbuf))
    {
        pstMbuf = MBUF_Fragment(pstMbuf, pstHttpAlyCtrl->stHttpAlyRequestInfo.ulHeadLen);
    }

    *ppMbuf = pstMbuf;

    return BS_OK;
}

/***************************************************
 Description  : 查询Body是否接收完成
 Input        : hHandle: 协议解析器句柄
 Output       : None
 Return       : TRUE
                FALSE
 Caution      : None
****************************************************/
BOOL_T HTTP_ALY_IsRecvBodyOK(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl->stReadData.bIsBodyFin == TRUE)
    {
        return TRUE;
    }

    return FALSE;
}

/*
return BS_OK; BS_ERR;BS_NOT_COMPLETE
*/
BS_STATUS HTTP_ALY_TryBody(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    BS_STATUS eRet;
    MBUF_S *pstMbuf;

    if (! HTTP_ALY_IS_HEAD_RECVED(pstHttpAlyCtrl))
    {
        if (BS_OK != (eRet = HTTP_ALY_TryHead(hHandle)))
        {
            return eRet;
        }
    }

    /*if body接收完成 return ok*/
    if (pstHttpAlyCtrl->stReadData.bIsBodyFin == TRUE)
    {
        return BS_OK;
    }
    
    /*read数据*/
    eRet = _HTTP_ALY_ReadData(pstHttpAlyCtrl, &pstMbuf);
    if (BS_OK != eRet)
    {
        /* 如果是connection close方式,并且读取出错,则认为完成 */
        if ((RETCODE(eRet) != BS_NOT_COMPLETE)
            && (pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType == HTTP_ALY_BODY_LEN_CLOSED))
        {
            return BS_OK;
        }

        return eRet;
    }

    pstHttpAlyCtrl->stReadData.ulRecvBodyLen += MBUF_TOTAL_DATA_LEN(pstMbuf);

    MBUF_CAT_EXT(pstHttpAlyCtrl->stReadData.pstBodyMbuf, pstMbuf);

    eRet = _HTTP_ALY_IsBodyFinAndFraIt(pstHttpAlyCtrl);

    return eRet;
}

BS_STATUS HTTP_ALY_ParseKeyValue(IN HANDLE hHandle)
{
    CHAR *pcKey, *pcValue, *pcNext;
    MBUF_S *pstMbuf;
    CHAR *pcBuf;
    _HTTP_ALY_KEY_S *pstHttpAlyKey;
    UINT ulLineLen;
    CHAR *pszQueryString;
    UINT ulQueryLen = 0;
    UINT ulBodyLen;
    
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl->stReadData.bIsBodyFin == FALSE)
    {
        RETURN(BS_ERR);
    }

    pstMbuf = pstHttpAlyCtrl->stReadData.pstBodyMbuf;

    if (NULL == pstMbuf)
    {
        ulBodyLen = 0;
    }
    else
    {    
        /* 如果是chunk, 删除其中的chunk字段 */
        if (pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType == HTTP_ALY_BODY_LEN_TRUNKED)
        {
            _HTTP_ALY_DelChunkCode(pstHttpAlyCtrl);
        }

        ulBodyLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

        if (ulBodyLen > 2048)
        {
            RETURN(BS_NOT_SUPPORT);
        }
    }

    pszQueryString = HTTP_ALY_GetQueryString(hHandle);
    if (pszQueryString == NULL)
    {
        pszQueryString = "";
    }

    ulQueryLen = strlen(pszQueryString);

    if (ulQueryLen + ulBodyLen == 0)
    {
        return BS_OK;
    }

    pcBuf = MEM_Malloc(ulQueryLen + ulBodyLen + 1);
    TXT_StrCpy(pcBuf, pszQueryString);
    if (ulBodyLen != 0)
    {
        MBUF_CopyFromMbufToBuf(pstMbuf, 0, ulBodyLen, pcBuf + ulQueryLen);
    }
    pcBuf[ulQueryLen + ulBodyLen] = '\0';

    pcNext = pcBuf;
    
    do {
        pcKey = pcNext;
        pcValue = strchr(pcKey, '=');
        if (NULL == pcValue)
        {
            MEM_Free(pcBuf);
            RETURN(BS_ERR);
        }

        pcNext = strchr(pcValue, '&');
        if (pcNext != NULL)
        {
            pcNext ++;
        }

        if (pcNext != NULL)
        {
            ulLineLen = pcNext - pcKey - 1;
        }
        else
        {
            ulLineLen = ulQueryLen + ulBodyLen - (pcKey - pcBuf);
        }

        pstHttpAlyKey = MEM_Malloc(sizeof(_HTTP_ALY_KEY_S) + ulLineLen + 2);
        if (pstHttpAlyKey == NULL)
        {
            BS_WARNNING(("No memory!"));
            RETURN(BS_NO_MEMORY);
        }
        Mem_Zero(pstHttpAlyKey, sizeof(_HTTP_ALY_KEY_S) + ulLineLen + 2);

        pstHttpAlyKey->pucKey = (UCHAR*)(pstHttpAlyKey+1);
        MEM_Copy(pstHttpAlyKey->pucKey, pcKey, pcValue - pcKey);
        pstHttpAlyKey->pucKey[pcValue - pcKey] = '\0';

        pstHttpAlyKey->pucKeyValue = pstHttpAlyKey->pucKey + (pcValue - pcKey) + 1;
        MEM_Copy(pstHttpAlyKey->pucKeyValue, pcValue + 1, ulLineLen - (pcValue - pcKey) -1);
        pstHttpAlyKey->pucKeyValue[ulLineLen - (pcValue - pcKey) -1] = '\0';

        TXT_ReplaceSubStr((CHAR*)pstHttpAlyKey->pucKey, " ", "", (CHAR*)pstHttpAlyKey->pucKey, strlen((CHAR*)pstHttpAlyKey->pucKey) + 1);
        TXT_ReplaceSubStr((CHAR*)pstHttpAlyKey->pucKeyValue, " ", "", (CHAR*)pstHttpAlyKey->pucKeyValue, strlen((CHAR*)pstHttpAlyKey->pucKeyValue) + 1);

        _HTTP_ALY_UnCode(pstHttpAlyKey->pucKey, pstHttpAlyKey->pucKey);
        _HTTP_ALY_UnCode(pstHttpAlyKey->pucKeyValue, pstHttpAlyKey->pucKeyValue);

        DLL_ADD(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllKeyValueHead, &pstHttpAlyKey->stDllNode);
    }while (NULL != pcNext);

    MEM_Free(pcBuf);

    return BS_OK;
}



/*
*成功: return BS_OK; BS_NOT_COMPLETE
*失败: 返回失败code
*/
BS_STATUS HTTP_ALY_SetNotModify(IN HANDLE hHttpHandle)
{
    BS_STATUS eRet = BS_OK;
    
    BS_DBGASSERT(NULL != hHttpHandle);

    eRet += HTTP_ALY_SetResponseStatusCode(hHttpHandle, HTTP_ALY_STATUS_CODE_NOT_MODIFIED, "Not Modified");
    eRet += HTTP_ALY_SetResponseHeadField(hHttpHandle, "Content-Type", "text/html");
    eRet += HTTP_ALY_SetResponseHeadField(hHttpHandle, "Content-Length", "0");
    eRet += HTTP_ALY_SetNoCache(hHttpHandle);

    if (eRet != BS_OK)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS HTTP_ALY_SetNotFound(IN HANDLE hHttpHandle)
{
    BS_STATUS eRet = BS_OK;

    BS_DBGASSERT(NULL != hHttpHandle);

    eRet += HTTP_ALY_SetResponseStatusCode(hHttpHandle, HTTP_ALY_STATUS_CODE_NOT_FOUND, "Not Found");
    eRet += HTTP_ALY_SetResponseHeadField(hHttpHandle, "Content-Type", "text/html");
    eRet += HTTP_ALY_SetResponseHeadField(hHttpHandle, "Content-Length", "0");
    eRet += HTTP_ALY_SetNoCache(hHttpHandle);

    if (eRet != BS_OK)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS HTTP_ALY_SetMethodNotAllowed(IN HANDLE hHttpHandle)
{
    BS_STATUS eRet = BS_OK;

    BS_DBGASSERT(NULL != hHttpHandle);

    eRet += HTTP_ALY_SetResponseStatusCode(hHttpHandle, HTTP_ALY_STATUS_CODE_METHOD_NOT_ALLOWED, NULL);
    eRet += HTTP_ALY_SetResponseHeadField(hHttpHandle, "Content-Type", "text/html");
    eRet += HTTP_ALY_SetResponseHeadField(hHttpHandle, "Content-Length", "0");
    eRet += HTTP_ALY_SetNoCache(hHttpHandle);

    if (eRet != BS_OK)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS HTTP_ALY_SetNoCache(IN HANDLE hHandle)
{
    HTTP_ALY_SetResponseHeadField(hHandle, "Progma", "no-cache");
    return HTTP_ALY_SetResponseHeadField(hHandle, "Cache-Control", "no-cache");
}

BS_STATUS HTTP_ALY_SetBaseRspHttpHeader(IN HANDLE hHandle, IN UCHAR *pucBuf)
{
    if (strlen((CHAR*)pucBuf) > HTTP_ALY_MAX_RSP_BASE_HEAD_LEN)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    //TODO:解析头部并设置SendKeyValue


    return BS_OK;
}

static inline BS_STATUS _HTTP_ALY_SetResponseHeadFieldRaw
(
    IN HANDLE hHandle,
    IN CHAR *pszKey,
    IN UINT uiKeyLen,
    IN CHAR *pszValue,
    IN UINT uiValueLen
)
{
    UINT ulLen = uiKeyLen + uiValueLen;
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    _HTTP_ALY_KEY_S *pstNode, *pstNodeTmp;

    pstNode = MEM_ZMalloc(sizeof(_HTTP_ALY_KEY_S) + ulLen + 2);

    pstNode->pucKey = (UCHAR*)(pstNode + 1);
    MEM_Copy(pstNode->pucKey, pszKey, uiKeyLen);
    pstNode->pucKey[uiKeyLen] = '\0';

    pstNode->pucKeyValue = pstNode->pucKey + uiKeyLen + 1;
    MEM_Copy(pstNode->pucKeyValue, pszValue, uiValueLen);
    pstNode->pucKeyValue[uiValueLen] = '\0';

    /* 释放原来同名的节点 */
    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue, pstNodeTmp)
    {
        if (strcmp((CHAR*)pstNodeTmp->pucKey, pszKey) == 0)
        {
            DLL_DEL(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue, pstNodeTmp);
            MEM_Free(pstNodeTmp);
            break;
        }
    }

    DLL_ADD(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue, pstNode);

    return BS_OK;
    
}

BS_STATUS HTTP_ALY_SetResponseHeadField(IN HANDLE hHandle, IN CHAR *pszKey, IN CHAR *pszValue)
{
    return _HTTP_ALY_SetResponseHeadFieldRaw(hHandle, pszKey, strlen(pszKey), pszValue, strlen(pszValue));
}

CHAR * HTTP_ALY_GetResponseKeyValue(IN HANDLE hHandle, IN CHAR *pszKey)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    _HTTP_ALY_KEY_S *pstNode;

    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendKeyValue, pstNode)
    {
        if (strcmp((CHAR*)pstNode->pucKey, pszKey) == 0)
        {
            return (CHAR*)pstNode->pucKeyValue;
        }
    }

    return NULL;
}

/* 设置一行KeyValue */
BS_STATUS HTTP_ALY_SetResponseHeadFieldByLine(IN HANDLE hHandle, IN CHAR *pszLine, IN UINT uiLineLen)
{
    CHAR *pcSplit;
    CHAR *pcValue;
    BS_STATUS eRet;
    UINT uiKeyLen;

    /* 查找冒号 */
    pcSplit = TXT_Strnchr(pszLine, ':', uiLineLen);
    if (NULL == pcSplit)
    {
        RETURN(BS_ERR);
    }
    uiKeyLen = pcSplit - pszLine;
    
    pcValue = pcSplit + 1;

    while (TXT_IS_BLANK_OR_RN(*pcValue))
    {
        pcValue++;
    }

    eRet = _HTTP_ALY_SetResponseHeadFieldRaw(hHandle, pszLine, uiKeyLen, pcValue, pszLine + uiLineLen - pcValue);

    return eRet;
}

/*
* 根据Buf内容设置KeyValue; 一行就是一个KeyValue
*/
BS_STATUS HTTP_ALY_SetResponseHeadFieldByBuf(IN HANDLE hHandle, IN CHAR *pszString)
{
    CHAR *pszLine;
    UINT uiLineLen;
    
    TXT_SCAN_LINE_BEGIN(pszString,pszLine,uiLineLen)
    {
        HTTP_ALY_SetResponseHeadFieldByLine(hHandle, pszLine, uiLineLen);
    }TXT_SCAN_LINE_END();

    return BS_OK;
}

BS_STATUS HTTP_ALY_SetCookie(IN HANDLE hHandle, IN CHAR *pszCookieKey, IN CHAR *pszCookieValue, IN CHAR *pszPath)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    UINT ulCookieLen, ulPathLen;
    _HTTP_ALY_COOKIE_S *pstCookieKey;

    BS_DBGASSERT(NULL != pszCookieKey);
    BS_DBGASSERT(NULL != pszCookieValue);

    ulPathLen = 0;

    ulCookieLen = strlen(pszCookieKey) + strlen(pszCookieValue);
    if (pszPath != NULL)
    {
        ulPathLen = strlen(pszPath);
    }

    if (ulCookieLen + ulPathLen + 8 > HTTP_ALY_MAX_COOKIE_LEN)
    {
        RETURN(BS_NOT_SUPPORT);
    }

    pstCookieKey = MEM_ZMalloc(sizeof(_HTTP_ALY_COOKIE_S) + ulCookieLen + ulPathLen + 3);
    
    pstCookieKey->pucKey = (UCHAR*)(pstCookieKey + 1);
    TXT_StrCpy((CHAR*)pstCookieKey->pucKey, pszCookieKey);

    pstCookieKey->pucKeyValue = pstCookieKey->pucKey + strlen(pszCookieKey) + 1;
    TXT_StrCpy((CHAR*)pstCookieKey->pucKeyValue, pszCookieValue);

    if (NULL != pszPath)
    {
        pstCookieKey->pszPath = pstCookieKey->pucKeyValue + strlen(pszCookieValue) + 1;
        TXT_StrCpy((CHAR*)pstCookieKey->pszPath, pszPath);
    }

    DLL_ADD(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stCookie, pstCookieKey);

    return BS_OK;
}

CHAR * HTTP_ALY_GetField(IN HANDLE hHandle, IN CHAR *pcFieldName)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    _HTTP_ALY_KEY_S *pstHttpAlyKey;

    if ((pcFieldName == NULL) || (hHandle == 0))
    {
        BS_WARNNING(("Null ptr!"));
        return NULL;
    }

    pstHttpAlyKey = _HTTP_ALY_GetField(pstHttpAlyCtrl, pcFieldName);
    if (NULL == pstHttpAlyKey)
    {
        return NULL;
    }

    return (CHAR*)pstHttpAlyKey->pucKeyValue;
}

/* 返回0表示没有找到 */
UINT HTTP_ALY_GetFieldValueOffset(IN HANDLE hHandle, IN CHAR *pcFieldName)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    DLL_NODE_S *pstNode;
    _HTTP_ALY_KEY_S *pstHttpAlyKey;

    if ((pcFieldName == NULL) || (hHandle == 0))
    {
        BS_WARNNING(("Null ptr!"));
        return 0;
    }

    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField, pstNode)
    {
        pstHttpAlyKey = (void*)pstNode;
        if (stricmp(pcFieldName, (CHAR*)pstHttpAlyKey->pucKey) == 0)
        {
            return pstHttpAlyKey->ulValueOffsetInHead;
        }
    }

    return 0;
}

/* 返回0表示没有找到 */
UINT HTTP_ALY_GetFieldKeyOffset(IN HANDLE hHandle, IN CHAR *pcFieldName)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    DLL_NODE_S *pstNode;
    _HTTP_ALY_KEY_S *pstHttpAlyKey;

    if ((pcFieldName == NULL) || (hHandle == 0))
    {
        BS_WARNNING(("Null ptr!"));
        return 0;
    }

    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField, pstNode)
    {
        pstHttpAlyKey = (void*)pstNode;
        if (stricmp(pcFieldName, (CHAR*)pstHttpAlyKey->pucKey) == 0)
        {
            return pstHttpAlyKey->ulKeyOffsetInHead;
        }
    }

    return 0;
}

CHAR * HTTP_ALY_GetKeyValue(IN HANDLE hHandle, IN CHAR *pcKey)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    DLL_NODE_S *pstNode;
    _HTTP_ALY_KEY_S *pstHttpAlyKey;

    if ((pcKey == NULL) || (hHandle == 0))
    {
        BS_WARNNING(("Null ptr!"));
        return NULL;
    }

    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllKeyValueHead, pstNode)
    {
        pstHttpAlyKey = (_HTTP_ALY_KEY_S *) pstNode;
        if (stricmp(pcKey, (CHAR*)pstHttpAlyKey->pucKey) == 0)
        {
            return (CHAR*)pstHttpAlyKey->pucKeyValue;
        }
    }

    return NULL;
}

BOOL_T HTTP_ALY_IsRequestKeepAlive(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return FALSE;
    }

    return pstHttpAlyCtrl->stHttpAlyRequestInfo.bKeepAlive;
}

BOOL_T HTTP_ALY_IsResponseKeepAlive(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return FALSE;
    }

    return pstHttpAlyCtrl->stHttpAlyResponseInfo.bIsKeepAlive;
}

BOOL_T HTTP_ALY_IsProxyConnection(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return FALSE;
    }

    return pstHttpAlyCtrl->stHttpAlyRequestInfo.bIsProxyConnection;
}

HTTP_PROTOCOL_VER_E HTTP_ALY_GetRequestVer(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return HTTP_PROTOCOL_VER_UNKNOWN;
    }

    return pstHttpAlyCtrl->stHttpAlyRequestInfo.eHttpVer;
}

CHAR * HTTP_ALY_GetProtocol(IN HANDLE hHandle)
{
    HTTP_PROTOCOL_VER_E eVer;

    eVer = HTTP_ALY_GetRequestVer(hHandle);

    if (eVer == HTTP_PROTOCOL_VER_10)
    {
        return "HTTP/1.0";
    }
    else if (eVer == HTTP_PROTOCOL_VER_11)
    {
        return "HTTP/1.1";
    }

    return NULL;        
}

USHORT HTTP_ALY_GetPort(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return 0;
    }

    return pstHttpAlyCtrl->stHttpAlyRequestInfo.usPort;
}

UINT HTTP_ALY_GetHeadLen(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return 0;
    }

    return pstHttpAlyCtrl->stHttpAlyRequestInfo.ulHeadLen;
}

HTTP_ALY_BODY_LEN_TYPE_E HTTP_ALY_GetBodyLenType(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return HTTP_ALY_BODY_LEN_UNKNOWN;
    }

    return pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType;
}

UINT HTTP_ALY_GetBodyLen(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return 0;
    }

    if (pstHttpAlyCtrl->stHttpAlyRequestInfo.eBodyLenType == HTTP_ALY_BODY_LEN_KNOWN)
    {
        return pstHttpAlyCtrl->stHttpAlyRequestInfo.ulBodyLen;
    }

    return 0;
}

CHAR * HTTP_ALY_GetHost(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return NULL;
    }

    return (CHAR*)pstHttpAlyCtrl->stHttpAlyRequestInfo.pucHost;
}

CHAR * HTTP_ALY_GetQueryString(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return NULL;
    }

    return (CHAR*)pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString;
}

CHAR * HTTP_ALY_GetUrl(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return NULL;
    }

    return (CHAR*)pstHttpAlyCtrl->stHttpAlyRequestInfo.pucUrlPath;
}

CHAR * HTTP_ALY_GetCookie(IN HANDLE hHandle, IN CHAR *pszCookieKey)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    _HTTP_ALY_COOKIE_S *pstCookieKey;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return NULL;
    }

    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stCookie, pstCookieKey)
    {
        if (strcmp(pszCookieKey, (CHAR*)pstCookieKey->pucKey) == 0)
        {
            return (CHAR*)pstCookieKey->pucKeyValue;
        }
    }

    return NULL;
}

CHAR * HTTP_ALY_GetReferer(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return NULL;
    }

    return (CHAR*)pstHttpAlyCtrl->stHttpAlyRequestInfo.pucReferer;
}

CHAR * HTTP_ALY_GetMethodString(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return NULL;
    }

    return (CHAR*)pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod;
}

HTTP_ALY_METHOD_E HTTP_ALY_GetMethodByString(IN CHAR *pcMethod)
{
    CHAR *apMethodString[] = {"GET", "POST", "HEAD", "OPTIONS", "PUT", "DELETE", "TRACE"};
    HTTP_ALY_METHOD_E aeMethod[] = {
            HTTP_ALY_METHOD_GET,
            HTTP_ALY_METHOD_POST,
            HTTP_ALY_METHOD_HEAD,
            HTTP_ALY_METHOD_OPTIONS,
            HTTP_ALY_METHOD_PUT,
            HTTP_ALY_METHOD_DELETE,
            HTTP_ALY_METHOD_TRACE
        };
    UINT i;

    for (i=0; i<sizeof(apMethodString)/sizeof(CHAR *); i++)
    {
        if (stricmp(pcMethod, apMethodString[i]) == 0)
        {
            return aeMethod[i];
        }
    }

    return HTTP_ALY_METHOD_UNKNOWN;
}

HTTP_ALY_METHOD_E HTTP_ALY_GetMethod(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return HTTP_ALY_METHOD_UNKNOWN;
    }

    return pstHttpAlyCtrl->stHttpAlyRequestInfo.eMethod;
}

UINT HTTP_ALY_GetAvailableDataSize(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
	UINT ulLen;

    if (pstHttpAlyCtrl->stReadData.pstHeadMbuf == NULL)
    {
        return 0;
    }

    ulLen = MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstHeadMbuf);

	if (pstHttpAlyCtrl->stReadData.pstBodyMbuf != NULL)
	{
		ulLen += MBUF_TOTAL_DATA_LEN(pstHttpAlyCtrl->stReadData.pstBodyMbuf);
	}

	return ulLen;
}

UINT HTTP_ALY_GetFileId(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (NULL == pstHttpAlyCtrl)
    {
        return 0;
    }

    return pstHttpAlyCtrl->ulFd;
}

UINT HTTP_ALY_GetResponseStatusCode(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        return FALSE;
    }

    return pstHttpAlyCtrl->stHttpAlyResponseInfo.ulStatusCode;
}

BS_STATUS HTTP_ALY_SetResponseStatusCode(IN HANDLE hHandle, IN UINT ulStatusCode, IN CHAR *pszDesc)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    if (pstHttpAlyCtrl == NULL)
    {
        BS_WARNNING(("Null ptr!"));
        RETURN(BS_NULL_PARA);
    }

    pstHttpAlyCtrl->stHttpAlyResponseInfo.ulStatusCode = ulStatusCode;
    pstHttpAlyCtrl->stHttpAlyResponseInfo.szStatusDesc[0] = '\0';
    if (pszDesc != NULL)
    {
        TXT_Strlcpy(pstHttpAlyCtrl->stHttpAlyResponseInfo.szStatusDesc, pszDesc,
            sizeof(pstHttpAlyCtrl->stHttpAlyResponseInfo.szStatusDesc));
    }

    return BS_OK;
}

BS_STATUS HTTP_ALY_SetResponseContentLen(IN HANDLE hHandle, IN ULONG ulContentLen)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    CHAR szContentLen[HTTP_ALY_MAX_UINT_STRING_LEN + 1];

    sprintf(szContentLen, "%ld", ulContentLen);

    return HTTP_ALY_SetResponseHeadField(pstHttpAlyCtrl, "Content-Length", szContentLen);
}


BS_STATUS HTTP_ALY_SetResponseKeepAlive(IN HANDLE hHandle, IN BOOL_T bIsKeepAlive)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;

    pstHttpAlyCtrl->stHttpAlyResponseInfo.bIsKeepAlive = bIsKeepAlive;

    if (TRUE == bIsKeepAlive)
    {
        HTTP_ALY_SetResponseHeadField(pstHttpAlyCtrl, "Connection", "Keep-Alive");
    }
    else
    {
        HTTP_ALY_SetResponseHeadField(pstHttpAlyCtrl, "Connection", "closed");
    }

    return BS_OK;
}


BS_STATUS HTTP_ALY_NotBuildHeadField(IN HANDLE hHandle, IN CHAR *pszFieldName)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    _HTTP_ALY_KEY_S *pstNode;

    pstNode = _HTTP_ALY_GetField(pstHttpAlyCtrl, pszFieldName);
    if (NULL == pstNode)
    {
        return BS_OK;
    }

    pstNode->bIsBuildHttpHeadDisable = TRUE;

    return BS_OK;
}

/* HTTP_ALY_NotBuildHeadField的反向函数 */
BS_STATUS HTTP_ALY_BuildHeadField(IN HANDLE hHandle, IN CHAR *pszFieldName)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    _HTTP_ALY_KEY_S *pstNode;

    pstNode = _HTTP_ALY_GetField(pstHttpAlyCtrl, pszFieldName);
    if (NULL == pstNode)
    {
        return BS_OK;
    }

    pstNode->bIsBuildHttpHeadDisable = TRUE;

    return BS_OK;
}

BS_STATUS HTTP_ALY_AddRequestHeadField(IN HANDLE hHandle, IN CHAR *pszFieldName, IN CHAR *pszFieldValue)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
    _HTTP_ALY_KEY_S *pstNode;
    UINT ulFieldNameLen, ulFieldValueLen;

    ulFieldNameLen = strlen(pszFieldName) ;
    ulFieldValueLen = strlen(pszFieldValue);

    pstNode = MEM_ZMalloc(sizeof(_HTTP_ALY_KEY_S) + ulFieldNameLen + ulFieldValueLen + 2);
    if (NULL == pstNode)
    {
        RETURN(BS_NO_MEMORY);
    }

    pstNode->pucKey = (UCHAR*)(pstNode + 1);
    TXT_StrCpy((CHAR*)pstNode->pucKey, pszFieldName);
    pstNode->pucKeyValue = pstNode->pucKey + ulFieldNameLen + 1;
    TXT_StrCpy((CHAR*)pstNode->pucKeyValue, pszFieldValue);

    DLL_ADD(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField, pstNode);

    HTTP_ALY_NotBuildHeadField(hHandle, pszFieldName);

    return BS_OK;
}

MBUF_S * HTTP_ALY_BuildRequestHead(IN HANDLE hHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHandle;
	UINT ulLen;
    MBUF_S *pstMbuf;
    CHAR szHttpHead[HTTP_ALY_MAX_HEAD_LEN + 1] = "";
    _HTTP_ALY_KEY_S *pstNode;

    if (pstHttpAlyCtrl->bIsServer == TRUE)
    {
        /* 构造请求行 */
        ulLen = sprintf(szHttpHead, "%s %s%s%s HTTP/%s\r\n",
                    pstHttpAlyCtrl->stHttpAlyRequestInfo.pucMethod, 
                    pstHttpAlyCtrl->stHttpAlyRequestInfo.pucUrlPath,
                    pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString == NULL ? "" : "?",
                    pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString == NULL ? "" : (CHAR*)(pstHttpAlyCtrl->stHttpAlyRequestInfo.pucQueryString),
                    pstHttpAlyCtrl->stHttpAlyRequestInfo.eHttpVer == HTTP_PROTOCOL_VER_10 ? "1.0" : "1.1");
    }
    else
    {
        /* 构造应答行 */
        ulLen = sprintf(szHttpHead, "HTTP/%s %d\r\n",
                    pstHttpAlyCtrl->stHttpAlyRequestInfo.eHttpVer == HTTP_PROTOCOL_VER_10 ? "1.0":"1.1",
                    pstHttpAlyCtrl->stHttpAlyResponseInfo.ulStatusCode);
    }

    /*  根据原来的HTTP报文信息组装新的HTTP头 */
    DLL_SCAN(&pstHttpAlyCtrl->stHttpAlyRequestInfo.stDllHeadField, pstNode)
    {
        if (pstNode->bIsBuildHttpHeadDisable == TRUE)   /* 指定了不组装这个Field */
        {
            continue;
        }

        ulLen += sprintf(szHttpHead + ulLen, "%s: %s\r\n", pstNode->pucKey, pstNode->pucKeyValue);
    }

    ulLen += sprintf(szHttpHead + ulLen, "\r\n");

    pstMbuf = MBUF_CreateByCopyBuf(0, szHttpHead, ulLen, MBUF_DATA_DATA);

    return pstMbuf;
}

VOID HTTP_ALY_SetRemoveChunkFlag(IN HANDLE hHttpHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHttpHandle;

    pstHttpAlyCtrl->stReadData.bIfRemoveChunkFlag = TRUE;
}

/* OK/NOT_COMPLETE/其他错误值 */
BS_STATUS HTTP_ALY_SetRedirectTo(IN HANDLE hHttpHandle, IN CHAR *pszRedirectTo)
{
    BS_STATUS eRet = BS_OK;

    BS_DBGASSERT(NULL != pszRedirectTo);

    eRet += HTTP_ALY_SetResponseStatusCode(hHttpHandle, HTTP_ALY_STATUS_CODE_MOVED_PMT, "Movied");
    eRet += HTTP_ALY_SetResponseHeadField(hHttpHandle, "Content-Type", "text/html");
    eRet += HTTP_ALY_SetResponseHeadField(hHttpHandle, "Content-Length", "0");
    eRet += HTTP_ALY_SetResponseHeadField(hHttpHandle, "Location", pszRedirectTo);
    eRet += HTTP_ALY_SetNoCache(hHttpHandle);

    if (eRet != BS_OK)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS HTTP_ALY_BuildResponseHead(IN HANDLE hHttpHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHttpHandle;

    if (pstHttpAlyCtrl->stHttpAlyResponseInfo.bIsSendHead == FALSE)
    {
        if (BS_OK != _HTTP_ALY_FormatResponseHeader(pstHttpAlyCtrl))
        {
            RETURN(BS_ERR);
        }
    }

    return BS_OK;
}

/* pucData可以为NULL，这时uiDataLen必须为0. 这种情况下只发送缓冲区中的数据 */
BS_STATUS HTTP_ALY_Send(IN HANDLE hHttpHandle, IN UCHAR *pucData, IN UINT uiDataLen, OUT UINT *pulSendLen)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHttpHandle;
    BS_STATUS eRet;

    /* 已经调用了Finish，却还尝试发送用户数据是不对的. */
    if (uiDataLen > 0)
    {
        if (pstHttpAlyCtrl->stHttpAlyResponseInfo.bIsFinish == TRUE)
        {
            BS_DBGASSERT(0);
            RETURN(BS_ERR);
        }
    }

    /* 还未发送头,则先构造头 */
    if (pstHttpAlyCtrl->stHttpAlyResponseInfo.bIsSendHead == FALSE)
    {
        if (BS_OK != _HTTP_ALY_FormatResponseHeader(pstHttpAlyCtrl))
        {
            RETURN(BS_ERR);
        }
    }

    /* 发送缓冲区中的协议数据 */
    if (VBUF_GetDataLength(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf) > 0)
    {
        eRet = _HTTP_ALY_SendBuffedData(pstHttpAlyCtrl);
        if (eRet != BS_OK)
        {
            return eRet;
        }
    }

    if (pstHttpAlyCtrl->stHttpAlyResponseInfo.ucResponseMode == HTTP_ALY_RESPONSE_MODE_CHUNKED)
    {
        return _HTTP_ALY_SendDataByChunk(pstHttpAlyCtrl, pucData, uiDataLen, pulSendLen);
    }
    else
    {
        return _HTTP_ALY_SendDataNormal(pstHttpAlyCtrl, pucData, uiDataLen, pulSendLen);
    }

    return BS_OK;
}

UCHAR HTTP_ALY_GetResponseMode(IN HANDLE hHttpHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHttpHandle;

    return pstHttpAlyCtrl->stHttpAlyResponseInfo.ucResponseMode;
}

/* 数据发送完成.但是这种情况下缓冲区中可能还有数据 */
BS_STATUS HTTP_ALY_Finish(IN HANDLE hHttpHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHttpHandle;
    BS_STATUS eRet;

    pstHttpAlyCtrl->stHttpAlyResponseInfo.bIsFinish = TRUE;

    /* 如果是Chunked发送,则发送Chunked结束标志 */
    if (pstHttpAlyCtrl->stHttpAlyResponseInfo.ucResponseMode == HTTP_ALY_RESPONSE_MODE_CHUNKED)
    {
        eRet = _HTTP_ALY_SendHttpProtoData(pstHttpAlyCtrl, (UCHAR*)"0\r\n\r\n", 5);
        if ((eRet != BS_OK) && (eRet != BS_NOT_COMPLETE))
        {
            return eRet;
        }
    }

    return BS_OK;
}

/***************************************************
 Description  : 得到发送缓冲区中还有多少数据数据
 Input        : hHttpHandle: HTTP ALY实例
 Output       : None
 Return       : 成功: BS_OK
                失败: 错误码
 Caution      : None
****************************************************/
UINT HTTP_ALY_GetSendDataSize(IN HANDLE hHttpHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHttpHandle;

    /* 如果没有构造过头，则构造头 */
    if (pstHttpAlyCtrl->stHttpAlyResponseInfo.bIsSendHead == FALSE)
    {
        _HTTP_ALY_FormatResponseHeader(pstHttpAlyCtrl);
    }

    return VBUF_GetDataLength(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf);
}


/***************************************************
 Description  : 发送缓冲区中的数据
 Input        : hHttpHandle: HTTP ALY实例
 Output       : None
 Return       : 成功: BS_OK
                失败: 错误码
 Caution      : None
****************************************************/
BS_STATUS HTTP_ALY_Flush(IN HANDLE hHttpHandle)
{
    _HTTP_ALY_CTRL_S *pstHttpAlyCtrl = (_HTTP_ALY_CTRL_S *)hHttpHandle;
    BS_STATUS eRet;

    if (VBUF_GetDataLength(&pstHttpAlyCtrl->stHttpAlyResponseInfo.stSendBuf) > 0)
    {
        eRet = _HTTP_ALY_SendBuffedData(pstHttpAlyCtrl);
        if (eRet != BS_OK)
        {
            return eRet;
        }
    }

    return BS_OK;
}

