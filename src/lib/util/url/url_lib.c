/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-10-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/url_lib.h"

typedef enum
{
    HTML_LIB_URL_WORD_TYPE_CURRENT = 0, 
    HTML_LIB_URL_WORD_TYPE_BACK,        
    HTML_LIB_URL_WORD_TYPE_NORMAL       
}HTML_LIB_URL_WORD_TYPE_E;

static inline HTML_LIB_URL_WORD_TYPE_E html_lib_GetUrlWordType(IN CHAR *pcUrl, IN UINT uiUrlLen)
{
    if ((uiUrlLen >= 2) && (pcUrl[0] == '.') && (pcUrl[1] == '/'))
    {
        return HTML_LIB_URL_WORD_TYPE_CURRENT;
    }

    if ((uiUrlLen >= 3) && (pcUrl[0] == '.') && (pcUrl[1] == '.') && (pcUrl[2] == '/'))
    {
        return HTML_LIB_URL_WORD_TYPE_BACK;
    }

    return HTML_LIB_URL_WORD_TYPE_NORMAL;
}

URL_LIB_URL_TYPE_E URL_LIB_GetUrlType(IN CHAR *pcUrl, IN UINT uiUrlLen)
{
    CHAR *pcFind;
    UCHAR ucFirst = *pcUrl;
    UCHAR ucSecond;

    if (('/' == ucFirst) || ('\\' == ucFirst))
    {
        if (uiUrlLen > 1)
        {
            ucSecond = pcUrl[1];
            if (('/' == ucSecond) || ('\\' == ucSecond))
            {
                return URL_TYPE_ABSOLUTE_SIMPLE;
            }
        }

        return URL_TYPE_RELATIVE_ROOT;
    }

    if ('.' == ucFirst)
    {
        return URL_TYPE_RELATIVE_PAGE;
    }

    pcFind = (CHAR *)TXT_Strnstr(pcUrl, "://", uiUrlLen);
    if (NULL != pcFind)
    {
        return URL_TYPE_ABSOLUTE;
    }

    if (isalpha(ucFirst) || isdigit(ucFirst))
    {
        return URL_TYPE_RELATIVE_PAGE;
    }

    return URL_TYPE_NOT_URL;
}


BS_STATUS ULR_LIB_ParseSimpleAbsUrl
(
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    OUT LSTR_S *pstHost,
    OUT LSTR_S *pstPort,
    OUT LSTR_S *pstPath
)
{
    CHAR *pcSplit;
    CHAR *pcUrlTmp = pcUrl;
    UINT uiReservedLen = uiUrlLen;

    pstHost->pcData = NULL;
    pstHost->uiLen = 0;
    pstPath->pcData = "/";
    pstPath->uiLen = 1;
    pstPort->pcData = NULL;
    pstPort->uiLen = 0;

    BS_DBGASSERT(URL_LIB_GetUrlType(pcUrl, uiUrlLen) == URL_TYPE_ABSOLUTE_SIMPLE);

    
    uiReservedLen -= 2;
    pcUrlTmp += 2;

    pcSplit = TXT_MStrnchr(pcUrlTmp, uiReservedLen, "/:");
    if (NULL == pcSplit)
    {
        pstHost->pcData = pcUrlTmp;
        pstHost->uiLen = uiReservedLen;
        return BS_OK;
    }

    pstHost->pcData = pcUrlTmp;
    pstHost->uiLen = pcSplit - pcUrlTmp;

    if (*pcSplit == '/')
    {
        pstPath->pcData = pcSplit;
        pstPath->uiLen = uiReservedLen - pstHost->uiLen;
        return BS_OK;
    }
    
    pcUrlTmp = pcSplit + 1;
    uiReservedLen -= (pstHost->uiLen + 1);

    pcSplit = TXT_Strnchr(pcUrlTmp, '/', uiReservedLen);
    if (NULL == pcSplit)
    {
        pstPort->pcData = pcUrlTmp;
        pstPort->uiLen = uiReservedLen;
        return BS_OK;
    }

    pstPort->pcData = pcUrlTmp;
    pstPort->uiLen = pcSplit - pcUrlTmp;

    pstPath->pcData = pcSplit;
    pstPath->uiLen = uiReservedLen - pstPort->uiLen;

    return BS_OK;
}



BS_STATUS ULR_LIB_ParseAbsUrl
(
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    OUT LSTR_S *pstProtocol,
    OUT LSTR_S *pstHost,
    OUT LSTR_S *pstPort,
    OUT LSTR_S *pstPath
)
{
    CHAR *pcSplit;
    CHAR *pcUrlTmp = pcUrl;
    UINT uiReservedLen = uiUrlLen;

    pstProtocol->pcData = NULL;
    pstProtocol->uiLen = 0;
    pstHost->pcData = NULL;
    pstHost->uiLen = 0;
    pstPath->pcData = "/";
    pstPath->uiLen = 1;
    pstPort->pcData = NULL;
    pstPort->uiLen = 0;

    BS_DBGASSERT(URL_LIB_GetUrlType(pcUrl, uiUrlLen) == URL_TYPE_ABSOLUTE);

    pcSplit = TXT_Strnstr(pcUrlTmp, "://", uiReservedLen);
    if (NULL == pcSplit)
    {
        return BS_ERR;
    }
    pstProtocol->pcData = pcUrl;
    pstProtocol->uiLen = pcSplit - pcUrl;

    uiReservedLen -= (pstProtocol->uiLen + 1);
    pcUrlTmp = pcSplit + 1;

    return ULR_LIB_ParseSimpleAbsUrl(pcUrlTmp, uiReservedLen, pstHost, pstPort, pstPath);
}



UINT URL_LIB_GetUrlBackNum(IN CHAR *pcUrl, IN UINT uiUrlLen)
{
    UINT uiReservedLen = uiUrlLen;
    CHAR *pcUrlTmp = pcUrl;
    HTML_LIB_URL_WORD_TYPE_E eType;
    UINT uiNum = 0;

    if (*pcUrl != '.')
    {
        return 0;
    }

    while (uiReservedLen > 0)
    {
        eType = html_lib_GetUrlWordType(pcUrlTmp, uiReservedLen);
        if (eType == HTML_LIB_URL_WORD_TYPE_NORMAL)
        {
            break;
        }
        else if (eType == HTML_LIB_URL_WORD_TYPE_CURRENT)
        {
            pcUrlTmp += 2;
            uiReservedLen -= 2;
        }
        else if (eType == HTML_LIB_URL_WORD_TYPE_BACK)
        {
            pcUrlTmp += 3;
            uiReservedLen -= 3;
            uiNum ++;
        }
    }

    return uiNum;
}

CHAR * URL_LIB_FullUrl2AbsPath(IN LSTR_S *pstFullUrl, OUT LSTR_S *pstAbsPath )
{
    URL_LIB_URL_TYPE_E eType;
    LSTR_S stProto;
    LSTR_S stHost;
    LSTR_S stPort;
    LSTR_S stPath;

    if ((NULL == pstFullUrl) || (pstFullUrl->uiLen == 0))
    {
        if (NULL != pstAbsPath)
        {
            LSTR_Init(pstAbsPath);
        }
        return NULL;
    }
    
    eType = URL_LIB_GetUrlType(pstFullUrl->pcData, pstFullUrl->uiLen);
    if (eType != URL_TYPE_ABSOLUTE)
    {
        if (NULL != pstAbsPath)
        {
            pstAbsPath->pcData = pstFullUrl->pcData;
            pstAbsPath->uiLen = pstFullUrl->uiLen;
        }

        return pstFullUrl->pcData;
    }

    if (BS_OK != ULR_LIB_ParseAbsUrl(pstFullUrl->pcData, pstFullUrl->uiLen, &stProto, &stHost, &stPort, &stPath))
    {
        if (NULL != pstAbsPath)
        {
            LSTR_Init(pstAbsPath);
        }

        return NULL;
    }

    if (NULL != pstAbsPath)
    {
        pstAbsPath->pcData = stPath.pcData;
        pstAbsPath->uiLen = stPath.uiLen;
    }

    return stPath.pcData;
}


BS_STATUS URL_LIB_BuildUrl
(
    IN LSTR_S *pstProto,
    IN LSTR_S *pstHost,
    IN LSTR_S *pstPort,
    IN LSTR_S *pstPath,
    OUT CHAR *pcUrl
)
{
    CHAR *pcTmp;

    pcTmp = pcUrl;
    memcpy(pcTmp, pstProto->pcData, pstProto->uiLen);
    pcTmp += pstProto->uiLen;
    memcpy(pcTmp, "://", 3);
    pcTmp += 3;
    memcpy(pcTmp, pstHost->pcData, pstHost->uiLen);
    pcTmp += pstHost->uiLen;
    if (!((LSTR_StrCmp(pstPort, "0") == 0)
        || ((LSTR_StrCaseCmp(pstProto, "http") == 0) && (LSTR_StrCmp(pstPort, "80") == 0))
        || ((LSTR_StrCaseCmp(pstProto, "https") == 0) && (LSTR_StrCmp(pstPort, "443") == 0))))
    {
        memcpy(pcTmp, ":", 1);
        pcTmp += 1;
        memcpy(pcTmp, pstPort->pcData, pstPort->uiLen);
        pcTmp += pstPort->uiLen;
    }

    if (NULL != pstPath)
    {
        memcpy(pcTmp, pstPath->pcData, pstPath->uiLen);
        pcTmp += pstPath->uiLen;
    }
    else
    {
        memcpy(pcTmp, "/", 1);
        pcTmp += 1;
    }

    *pcTmp = '\0';

    return BS_OK;
}


BS_STATUS URL_LIB_ParseUrl(IN CHAR *url, IN UINT url_len, OUT URL_FIELD_S *fields)
{
    char *find;
    char *tmp = url;
    INT left_len = url_len;

    memset(fields, 0, sizeof(URL_FIELD_S));

    
    find = TXT_Strnstr(url, "://", url_len);
    if (NULL == find) {
        return BS_NOT_FOUND;
    }
    fields->protocol.pcData = url;
    fields->protocol.uiLen = find - url;
    tmp = find + 3;
    left_len -= fields->protocol.uiLen;
    left_len -= 3;

    if (left_len <= 0) {
        return BS_OK;
    }

    
    fields->host.pcData = tmp;
    find = TXT_MStrnchr(tmp, left_len, ":/");
    if (NULL == find) {
        fields->host.uiLen = left_len;
        return BS_OK;
    }
    fields->host.uiLen = find - tmp;
    tmp = find;
    left_len -= fields->host.uiLen;

    
    if (*find == ':') {
        tmp = find + 1;
        left_len -= 1;
        if (left_len <= 0) {
            return BS_OK;
        }
        fields->port.pcData = tmp;
        find = TXT_Strnchr(tmp, '/', left_len);
        if (NULL == find) {
            fields->port.uiLen = left_len;
            return BS_OK;
        }
        fields->port.uiLen = find - tmp;
        tmp = find;
        left_len -= fields->port.uiLen;
    }

    
    fields->path.pcData = tmp;
    find = TXT_Strnchr(tmp, '?', left_len);
    if (find == NULL) {
        fields->path.uiLen = left_len;
    }
    fields->path.uiLen = find - tmp;
    tmp = find + 1;
    left_len -= fields->path.uiLen;
    left_len -= 1;

    if (left_len <= 0) {
        return BS_OK;
    }

    
    fields->query.pcData = tmp;
    fields->query.uiLen = left_len;

    return BS_OK;
}
