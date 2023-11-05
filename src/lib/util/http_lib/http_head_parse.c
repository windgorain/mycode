/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-21
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/bit_opt.h"
#include "utl/array_bit.h"
#include "utl/http_lib.h"

#define HTTP_TIME_REMAIN_MAX_LEN  18
#define HTTP_TIME_ISOC_REMAIN_LEN 14
#define HTTP_COOKIE_ULTOA_LEN     32UL
#define HTTP_STATUS_CODE_LEN      3UL
#define HTTP_COPY_LEN             2UL


#define HTTP_SPACE_FLAG                   " "             
#define HTTP_HEAD_FIELD_SPLIT_FLAG        ": "            

#define HTTP_WHITESPACE                   " \t\r\n"
#define HTTP_URI_HEAD_STR                 "://"
#define HTTP_URI_HEAD_STR_LEN             3UL

#define HTTP_VERSION_STR_LEN              8UL
#define HTTP_VERSION_0_9_STR              "HTTP/0.9"
#define HTTP_VERSION_1_0_STR              "HTTP/1.0"
#define HTTP_VERSION_1_1_STR              "HTTP/1.1"
#define HTTP_VERSION_UNKNOWN_STR          "HTTP/Unknown"

#define HTTP_CONN_KEEP_ALIVE              "Keep-Alive"          
#define HTTP_CONN_CLOSE                   "close"               
#define HTTP_TRANSFER_ENCODE_CHUNKED      "chunked"             
#define HTTP_MULTIPART_STR                "multipart"               
#define HTTP_CONTENT_TYPE_OCSP            "application/ocsp-response"       
#define HTTP_CONTENT_TYPE_JSON            "application/json"                

#define HTTP_SET_STR(pcDest, pcSrc)  ((pcDest) = (pcSrc))


typedef enum tagHTTP_TimeStandard
{
    NORULE = 0,
    RFC822,   
    RFC850,   
    ISOC,     
} HTTP_TIME_STANDARD_E;




typedef struct tagHTTP_UriInfo
{
    CHAR * pcFullUri;      
    CHAR * pcUriPath;      
    CHAR * pcUriQuery;     
    CHAR * pcUriParam;     
    CHAR * pcUriFragment;  
    CHAR * pcUriAbsPath;   
    CHAR * pcSimpleAbsPath;
}HTTP_URI_INFO_S;

typedef struct tagHTTP_Head{
    HTTP_VERSION_E enHttpVersion;           
    HTTP_METHOD_E enMethod;                 
    CHAR *pcMethodData;                     
    HTTP_URI_INFO_S stUriInfo;              
    HTTP_STATUS_CODE_E  enStatusCode;       
    HTTP_RANGE_S *pstRange;                 
    CHAR *pcReason;                         
    DLL_HEAD_S  stHeadFieldList;            
    MEMPOOL_HANDLE hMemPool;
    char *head_fields[HTTP_HF_MAX];         
}HTTP_HEAD_S;

typedef struct tagHTTP_TokData
{
    CHAR *pcData;       
    ULONG ulDataLen;    
}HTTP_TOK_DATA_S;

typedef struct tagHTTPSmartBuf
{
    CHAR   *pcBuf;             
    ULONG  ulBufLen;           
    ULONG  ulBufOffset;        
    BOOL_T bErrorFlag;         
}HTTP_SMART_BUF_S;    

typedef BS_STATUS (*HTTP_SCAN_LINE_PF)(IN CHAR *pcLineStart, IN ULONG ulLineLen, IN VOID *pUserContext);


static UINT  g_auiHttpDay[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static const CHAR *g_apcHttpParseMethodStrTable[]= 
{
    "OPTIONS",
    "GET",        
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "CONNECT"
};

static const CHAR *g_apcHttpStatueTable[] =
{
    "100 continue"                        ,
    "101 Switching Protocols"             ,
    "200 OK"                              ,
    "201 Created"                         ,
    "202 Accepted"                        ,
    "203 Non-Authoritative Information"   ,
    "204 No Content"                      ,
    "205 Reset Content"                   ,
    "206 Partial Content"                 ,
    "300 Multiple Choices"                ,
    "301 Moved Permanently"               ,
    "302 Found"                           ,
    "303 See Other"                       ,
    "304 Not Modified"                    ,
    "305 Use Proxy"                       ,
    "307 Temporary Redirect"              ,
    "400 Bad Request"                     ,
    "401 Unauthorized"                    ,
    "402 Payment Required"                ,
    "403 Forbidden"                       ,
    "404 Not Found"                       ,
    "405 Method Not Allowed"              ,
    "406 Not Acceptable"                  ,
    "407 Proxy Authentication Required"   ,
    "408 Request Time-out"                ,
    "409 Conflict"                        ,
    "410 Gone"                            ,
    "411 Length Required"                 ,
    "412 Precondition Failed"             ,
    "413 Request Entity Too Large"        ,
    "414 Request-URI Too Large"           ,
    "415 Unsupported Media Type"          ,
    "416 Requested range not satisfiable" ,
    "417 Expectation Failed"              ,
    "500 Internal Server Error"           ,
    "501 Method Not Implemented"          ,
    "502 Bad Gateway"                     ,
    "503 Service Unavailable"             ,
    "504 Gateway Time-out"                ,
    "505 HTTP Version not supported"                                                                      
};

static UCHAR g_aucHttpStausCodeMap[] = 
{
    
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_CONTINUE, HTTP_STATUS_SWITCH_PROTO, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_OK, HTTP_STATUS_CREATED, HTTP_STATUS_ACCEPTED, HTTP_STATUS_NON_AUTHO, HTTP_STATUS_NO_CONTENT,
    HTTP_STATUS_RESET_CONTENT, HTTP_STATUS_PARTIAL_CONTENT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_MULTI_CHCS, HTTP_STATUS_MOVED_PMT, HTTP_STATUS_MOVED_TEMP, HTTP_STATUS_SEE_OTHER, HTTP_STATUS_NOT_MODI,
    HTTP_STATUS_USE_PXY, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_REDIRECT_TEMP, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_BAD_RQ, HTTP_STATUS_UAUTHO, HTTP_STATUS_PAY_RQ, HTTP_STATUS_FORBIDDEN, HTTP_STATUS_NOT_FOUND,
    HTTP_STATUS_NOT_ALLOWED, HTTP_STATUS_NOT_ACC, HTTP_STATUS_PXY_AUTH_RQ, HTTP_STATUS_RQ_TIME_OUT, HTTP_STATUS_CONFLICT,
    HTTP_STATUS_GONE, HTTP_STATUS_LEN_RQ, HTTP_STATUS_PRECOND_FAIL, HTTP_STATUS_BODY_TOO_LARGE, HTTP_STATUS_URL_TOO_LARGE,
    HTTP_STATUS_USUPPORT_MEDIA, HTTP_STATUS_RANGE_ERR, HTTP_STATUS_EXPECT_FAILED, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,
    HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT, HTTP_STATUS_CODE_BUTT,

    
    HTTP_STATUS_INTERNAL_ERR, HTTP_STATUS_NOT_IMPLE, HTTP_STATUS_BAD_GATEWAY, HTTP_STATUS_SERVICE_UAVAIL, HTTP_STATUS_GATWAY_TIME_OUT,
    HTTP_STATUS_VER_NOT_SUPPORT    
};

CHAR *g_apcHttpCookieType[ HTTP_COOKIE_BUTT ] = 
{
    "Set-Cookie",
    "Set-Cookie2",
    "Cookie",
    "Cookie2"
};

CHAR *g_apcHttpCookieAV[ COOKIE_AV_BUTT ] = 
{
    
    "Comment",
    "CommentURL",   
    "Domain",    
    "Path",
    "Port",

    
    "Max-Age",
    "Version",

    
    "Discard",
    "Secure"
};

static CHAR *g_pcEncode = "=`~!#$%^&()+{}|:\"<>?[]\\;\',/\t";

static const CHAR *g_apcWkday[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const CHAR *g_apcMonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static char * g_http_head_hf_fields[] = {
#define _(a,b) b,
    HTTP_HF_DEFS
#undef _
    "unknown"
};
                                                       

static BS_STATUS http_InitBufData(IN HTTP_HEAD_S *pstHttpHead, IN ULONG ulBufLen, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    CHAR *pcBuf;

    
    BS_DBGASSERT(NULL != pstBuf);
    BS_DBGASSERT(0 != ulBufLen);

    pcBuf = MEMPOOL_Alloc(pstHttpHead->hMemPool, (UINT)ulBufLen);
    if(NULL == pcBuf)
    {
        return BS_ERR;
    }

    pcBuf[0] = '\0';    
    Mem_Zero(pstBuf, sizeof(HTTP_SMART_BUF_S));
    
    pstBuf->pcBuf      = pcBuf;
    pstBuf->ulBufLen   = ulBufLen;
    pstBuf->bErrorFlag = FALSE;

    return BS_OK;
}


static CHAR *http_MoveBufData(INOUT HTTP_SMART_BUF_S *pstBuf, OUT ULONG *pulDataLen)
{
    CHAR  *pcBuf = NULL;

    
    BS_DBGASSERT(NULL != pstBuf);
    BS_DBGASSERT(NULL != pulDataLen);
    
    *pulDataLen = 0;
    if(TRUE != pstBuf->bErrorFlag)
    {
        pcBuf = pstBuf->pcBuf;
        *pulDataLen = pstBuf->ulBufOffset;
    }
    return pcBuf;
}


static BS_STATUS http_AppendBufStr(IN CHAR *pcStr, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    ULONG ulLen;
    
        
    BS_DBGASSERT(NULL != pstBuf);
    BS_DBGASSERT(NULL != pcStr);
    
    if(TRUE == pstBuf->bErrorFlag)
    {
        return BS_ERR;
    }
    ulLen = strlen(pcStr);
    if((pstBuf->ulBufOffset + ulLen) >= pstBuf->ulBufLen)
    {
        pstBuf->bErrorFlag = TRUE;
        return BS_ERR;
    }
    memcpy(pstBuf->pcBuf + pstBuf->ulBufOffset, pcStr, ulLen);
    pstBuf->pcBuf[pstBuf->ulBufOffset + ulLen] = '\0';
    pstBuf->ulBufOffset += ulLen;
    return BS_OK;
}



static ULONG http_GetCharsNumInString(IN CHAR *pcString, IN CHAR cChar)
{
    CHAR *pcPos = NULL;
    CHAR *pcEnd = NULL;
    ULONG ulCount = 0;
    
    BS_DBGASSERT(NULL != pcString);
    
    pcPos = pcString;
    pcEnd = pcPos + strlen(pcString);
    while(pcPos < pcEnd)
    {
        if (cChar == *pcPos)
        {
            ulCount++;
        }
        pcPos++;
    }

    return ulCount;
}


static BOOL_T http_CheckStringIsDigital(IN CHAR *pcString, IN ULONG ulLen)
{
    CHAR *pcPos, *pcEnd;

    
    BS_DBGASSERT(pcString != NULL);
    BS_DBGASSERT(ulLen != 0);

    pcPos = pcString;
    pcEnd = pcString + ulLen;

    
    if(ulLen > HTTP_MAX_UINT64_LEN)
    {
        return FALSE;
    }

    while(pcPos < pcEnd)
    {
        if (isdigit(*pcPos))
        {
            pcPos++;
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}



HTTP_HEAD_PARSER HTTP_CreateHeadParser()
{
    HTTP_HEAD_S *pstHttpHead;

    pstHttpHead = (HTTP_HEAD_S *)MEM_ZMalloc(sizeof(HTTP_HEAD_S));
    if(NULL == pstHttpHead)
    {
        return NULL;
    }

    pstHttpHead->hMemPool = MEMPOOL_Create(0);
    if (NULL == pstHttpHead->hMemPool)
    {
        MEM_Free(pstHttpHead);
        return NULL;
    }

    pstHttpHead->enHttpVersion = HTTP_VERSION_BUTT;
    pstHttpHead->enMethod      = HTTP_METHOD_BUTT;
    pstHttpHead->enStatusCode  = HTTP_STATUS_CODE_BUTT;
    DLL_INIT(&(pstHttpHead->stHeadFieldList));

    return pstHttpHead;
}


VOID HTTP_DestoryHeadParser(IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead;
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    
    if( NULL != pstHttpHead )
    {
        if (NULL != pstHttpHead->hMemPool)
        {
            MEMPOOL_Destory(pstHttpHead->hMemPool);
            pstHttpHead->hMemPool = NULL;
        }
        
        MEM_Free(pstHttpHead);
    }
    return ;
}


UINT HTTP_GetHeadLen(IN CHAR *pcHttpData, IN ULONG ulDataLen)
{
    CHAR *pcFound = NULL;
    UINT tmp= 0;
    CHAR *pcTemp = NULL;

    
    if (NULL == pcHttpData) {
        return 0;
    }

    if (ulDataLen <= HTTP_CRLFCRLF_LEN) {
        return 0;
    }

    
    pcTemp = HTTP_StrimHead(pcHttpData, ulDataLen, HTTP_WHITESPACE);
    if(NULL == pcTemp) {
        return 0;
    }

    tmp = ulDataLen - ((ULONG)pcTemp - (ULONG)pcHttpData);
    if (tmp <= HTTP_CRLFCRLF_LEN) {
        return 0;
    }
    
    
    pcFound = (CHAR*)MEM_Find(pcTemp, tmp, HTTP_CRLFCRLF, HTTP_CRLFCRLF_LEN);
    if (NULL == pcFound) {
        return 0;
    }

    tmp= (UINT)(pcFound - pcHttpData);

    return tmp + HTTP_CRLFCRLF_LEN;
}


static BOOL_T http_IsHaveContinueLine(IN CHAR *pcHttpHead, IN ULONG ulHeadLen)
{
    
    BS_DBGASSERT(NULL != pcHttpHead);
    if (NULL != MEM_Find(pcHttpHead, ulHeadLen, "\r\n ", strlen("\r\n ")))
    {
        return TRUE;
    }

    if (NULL != MEM_Find(pcHttpHead, ulHeadLen, "\r\n\t", strlen("\r\n\t")))
    {
        return TRUE;
    }

    return FALSE;
}

CHAR * HTTP_StrimHead(IN CHAR *pcData, IN ULONG ulDataLen, IN CHAR *pcSkipChars)
{
    CHAR *pcTemp = pcData;
    CHAR *pcEnd = pcData + ulDataLen;

    
    if((NULL == pcData) || (NULL == pcSkipChars))
    {
        return NULL;
    }


    if (0 == ulDataLen)
    {
        return pcData;
    }
    while (pcTemp < pcEnd)
    {
        if(NULL == strchr(pcSkipChars, *pcTemp))
        {
            break;
        }        
        pcTemp ++;
    }

    return pcTemp;
}


ULONG HTTP_StrimTail(IN CHAR *pcData, IN ULONG ulDataLen, IN CHAR *pcSkipChars)
{
    CHAR *pcTemp;

    
    if ((NULL == pcData) || (NULL == pcSkipChars))
    {
        return 0;
    }

    if (0 == ulDataLen)
    {
        return 0;
    }

    pcTemp = (pcData + ulDataLen) - 1;

    while (pcTemp >= pcData)
    {
        if (NULL == strchr(pcSkipChars, *pcTemp))
        {
            break;
        }
        pcTemp --;
    }

    return ((ULONG)(pcTemp + 1) - (ULONG)pcData);
}


CHAR * HTTP_Strim(IN CHAR *pcData, IN ULONG ulDataLen, IN CHAR *pcSkipChars, OUT ULONG *pulNewLen)
{
    CHAR *pcTemp;
    ULONG ulDataLenTemp;

    
    if ((NULL == pcData) || (NULL == pcSkipChars) || (NULL == pulNewLen))
    {
        return NULL;
    }
    
    pcTemp = HTTP_StrimHead(pcData, ulDataLen, pcSkipChars);
    if(NULL == pcTemp)
    {
        return NULL;
    }
    ulDataLenTemp = ulDataLen - ((ULONG)pcTemp - (ULONG)pcData);
    *pulNewLen = HTTP_StrimTail(pcTemp, ulDataLenTemp, pcSkipChars);
    
    return pcTemp;
}


static BS_STATUS http_ScanLine(IN CHAR *pcData, IN ULONG ulDataLen, IN HTTP_SCAN_LINE_PF pfFunc, IN VOID *pUserContext)
{
    CHAR *pcLineBegin = pcData;
    CHAR *pcLineEnd = NULL;
    CHAR *pcEnd = NULL;
    ULONG ulOffset = 0;
    ULONG ulError = BS_ERR;

    
    BS_DBGASSERT(NULL != pcData);
    BS_DBGASSERT(0 != ulDataLen);
    BS_DBGASSERT(NULL != pfFunc);
    BS_DBGASSERT(NULL != pUserContext);


    
    pcEnd = pcData + ulDataLen;    
    while (pcLineBegin < pcEnd)
    {
        
        pcLineEnd = (CHAR*)MEM_Find(pcLineBegin, ((ULONG)pcEnd - (ULONG)pcLineBegin), 
                                    HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);

        
        if (NULL == pcLineEnd)
        {
            ulOffset = (ULONG)pcEnd - (ULONG)pcLineBegin;
        }
        else
        {
            ulOffset = (ULONG)pcLineEnd - (ULONG)pcLineBegin;
        }
        
        ulError = pfFunc(pcLineBegin, ulOffset, pUserContext);
        if (BS_OK != ulError)
        {
            break;
        }
        
        pcLineBegin += ulOffset + HTTP_LINE_END_FLAG_LEN;
    }
    
    return ulError;
}


static BS_STATUS http_CompressHeadLine(IN CHAR *pcLineStart, IN ULONG ulLineLen, IN VOID *pUserContext)
{
    CHAR *pcHead = NULL;
    ULONG ulHeadLen = 0;


    
    BS_DBGASSERT(NULL != pcLineStart);
    BS_DBGASSERT(0 != ulLineLen);
    BS_DBGASSERT(NULL != pUserContext);

    pcHead = pUserContext;
    ulHeadLen = strlen(pcHead);

    
    if ((!TXT_IS_BLANK(*pcLineStart)) && (0 != ulHeadLen))
    {
        memcpy(pcHead + ulHeadLen, HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);
        ulHeadLen += HTTP_LINE_END_FLAG_LEN;       
    }
    
    memcpy(pcHead + ulHeadLen, pcLineStart, ulLineLen);
    ulHeadLen += ulLineLen;
    
    
    pcHead[ulHeadLen] = '\0';
    
    return BS_OK;
}


static CHAR * http_CompressHead(IN HTTP_HEAD_S *pstHttpHead, IN CHAR *pcHttpHead, IN ULONG ulHeadLen)
{
    CHAR *pcHead = NULL;

    
    BS_DBGASSERT(NULL != pcHttpHead);  
    BS_DBGASSERT(0 != ulHeadLen);    

    pcHead = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulHeadLen + 1); 
    if (NULL == pcHead)
    {
        return NULL;
    }
    pcHead[0] = '\0';
    
    
    (VOID) http_ScanLine(pcHttpHead, ulHeadLen, http_CompressHeadLine, pcHead);

    return pcHead;
}

static inline void * http_DupStr(HTTP_HEAD_S *pstHttp, void *data, ULONG data_len)
{
    char *pcWord = (CHAR*)MEMPOOL_Alloc(pstHttp->hMemPool, data_len + 1);
    if (! pcWord) {
        return NULL;
    }

    if (data_len > 0) {
        memcpy(pcWord, data, data_len);
    }

    pcWord[data_len] = '\0';

    return pcWord;
}


static inline CHAR * http_GetWord(IN HTTP_HEAD_S *pstHttpHead, IN CHAR *pcSrc, IN ULONG ulHeadLen)
{
    CHAR *pcRealBegin = NULL;
    ULONG ulRealLen = 0;

    
    BS_DBGASSERT(NULL != pcSrc);    

    
    pcRealBegin = HTTP_Strim(pcSrc, ulHeadLen, HTTP_SP_HT_STRING, &ulRealLen);
    if(NULL == pcRealBegin)
    {
        return NULL;
    }

    return http_DupStr(pstHttpHead, pcRealBegin, ulRealLen);
}

static inline int http_AddDynamicHeadField(HTTP_HEAD_S *pstHttp, char *key, ULONG key_len, char *val, ULONG val_len)
{
    HTTP_HEAD_FIELD_S *pstHeadField = MEMPOOL_ZAlloc(pstHttp->hMemPool, sizeof(HTTP_HEAD_FIELD_S));
    if(NULL == pstHeadField) {
        return BS_ERR;
    }

    pstHeadField->pcFieldName = http_GetWord(pstHttp, key, key_len);
    if (NULL == pstHeadField->pcFieldName) {
        MEMPOOL_Free(pstHttp->hMemPool, pstHeadField);
        return BS_ERR;
    }
    pstHeadField->uiFieldNameLen = strlen(pstHeadField->pcFieldName);
    pstHeadField->pcFieldValue = http_GetWord(pstHttp, val, val_len);

    DLL_ADD(&(pstHttp->stHeadFieldList), (DLL_NODE_S *)pstHeadField);        

    return BS_OK;
}


static BS_STATUS http_ParseField(IN CHAR *pcLineStart, IN ULONG ulLineLen, IN VOID *pUserContext)
{
    CHAR *pcFieldName = NULL;
    CHAR *pcValue = NULL;
    ULONG ulFieldNameLen;
    ULONG ulFieldValueLen;
    HTTP_HEAD_S *pstHttpHead = pUserContext;
    CHAR *pcSplit;

    BS_DBGASSERT(NULL != pUserContext);
    BS_DBGASSERT(0 != pcLineStart);
    BS_DBGASSERT(0 != ulLineLen);

    pcFieldName = pcLineStart;
    pcSplit = memchr(pcLineStart, HTTP_HEAD_FIELD_SPLIT_CHAR, ulLineLen);
    if(NULL == pcSplit)
    {
        
        return BS_OK;
    }
    ulFieldNameLen = (ULONG)pcSplit - (ULONG)pcLineStart;

    pcValue = pcSplit + 1;
    ulFieldValueLen = ulLineLen - (ulFieldNameLen + 1);

    return http_AddDynamicHeadField(pstHttpHead, pcFieldName, ulFieldNameLen, pcValue, ulFieldValueLen);
}


static HTTP_METHOD_E http_MethodParse(IN CHAR *pcMethodData)
{
    ULONG i = 0;
    
    
    BS_DBGASSERT(NULL != pcMethodData);
    
    for(i=0; i<HTTP_METHOD_BUTT; i++)
    {
        if(0 == strcmp(pcMethodData, g_apcHttpParseMethodStrTable[i]))
        {
            return (HTTP_METHOD_E)i;
        }
    }
    return HTTP_METHOD_BUTT;
}


static BS_STATUS http_HeadSetSimpleUri(IN HTTP_HEAD_S *pstHttpHead)
{
    CHAR *pcAbsUriDecoded = NULL;
    CHAR *pcSimpleAbsUri = NULL;
    size_t ulSrcUriLen = 0;
    ULONG ulToklen = 0;
    CHAR cTemp;
    CHAR cPre1;
    CHAR *pcStart;
    CHAR *pcSlash;
    CHAR *pcWalk;
    CHAR *pcOut;
    USHORT usPre;
    HTTP_URI_INFO_S *pstUriInfo;


    
    BS_DBGASSERT(pstHttpHead != NULL);

    pstUriInfo = &pstHttpHead->stUriInfo;

    
    if( NULL == pstUriInfo->pcUriAbsPath )
    {
        pstUriInfo->pcSimpleAbsPath = NULL;
        return BS_OK;
    }

    ulSrcUriLen = strlen(pstUriInfo->pcUriAbsPath);

    
    pcAbsUriDecoded = HTTP_UriDecode(pstHttpHead->hMemPool, pstUriInfo->pcUriAbsPath, ulSrcUriLen);
    if(NULL == pcAbsUriDecoded)
    {
        return BS_ERR;
    }
    
    ulSrcUriLen = strlen(pcAbsUriDecoded);

    
    
    pcSimpleAbsUri = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulSrcUriLen+1);
    if( NULL == pcSimpleAbsUri)
    {
        MEMPOOL_Free(pstHttpHead->hMemPool, pcAbsUriDecoded);
        return BS_ERR;
    }
    pcSimpleAbsUri[0] = '\0';

    pcWalk = pcAbsUriDecoded;
    pcStart = pcSimpleAbsUri;
    pcOut   = pcSimpleAbsUri;
    pcSlash = pcSimpleAbsUri;

    
    for (; '\0' != *pcWalk; pcWalk++)
    {
        if (*pcWalk == '\\') 
        {
            *pcWalk = '/';
        }
    }

    pcWalk = pcAbsUriDecoded;
    cPre1  = *(pcWalk++);
    cTemp  = *(pcWalk++);
    usPre  = (UCHAR)cPre1;
   
    *(pcOut++) = cPre1;

    for(;;)
    {
        
        if (('/' == cTemp) || ('\0' == cTemp)) 
        {
            
            ulToklen = (ULONG)pcOut - (ULONG)pcSlash;

            
            if ((3 == ulToklen) && ((('.' << 8) | '.') == usPre))
            {
                pcOut = pcSlash;
                if (pcOut > pcStart) 
                {
                    pcOut--;
                    while ((pcOut > pcStart) && (*pcOut != '/') )
                    {
                        pcOut--;
                    }
                }
                if ('\0' == cTemp)
                {
                    pcOut++;
                }
            } 
            
            if ((1 == ulToklen) || ((('/' << 8) | '.') == usPre))
            {
                pcOut = pcSlash;
                if ('\0' == cTemp)
                {
                    pcOut++;
                }
            }
            
            pcSlash = pcOut;
        }

        
        if ('\0' == cTemp)
        {
            break;  
        } 

        cPre1  = cTemp;
        usPre  = (USHORT)(((UINT)usPre << 8) & 0xff00);
        usPre |= (USHORT)(UCHAR)cPre1;
        cTemp  = *pcWalk;
        *pcOut = cPre1;

        pcOut++;
        pcWalk++;
    }
    
    *pcOut = '\0';
     
       
    HTTP_SET_STR(pstUriInfo->pcSimpleAbsPath, pcSimpleAbsUri);

    MEMPOOL_Free(pstHttpHead->hMemPool, pcAbsUriDecoded);

    return BS_OK;       
}

static BS_STATUS http_HeadSetAbsUri(IN HTTP_HEAD_S *pstHttpHead)
{
    ULONG ulMatchLen = 0;
    CHAR *pcFullPath = NULL;
    CHAR *pcTemp = NULL;
    ULONG ulSrcUriLen = 0;
    CHAR *pcGetChr = NULL;
    CHAR *pcGetChrChr = NULL;
    CHAR *pcTempPath = NULL;
    ULONG ulPathLen = 0;
    HTTP_URI_INFO_S *pstUriInfo;

    BS_DBGASSERT(NULL != pstHttpHead);
    pstUriInfo = &pstHttpHead->stUriInfo;
    BS_DBGASSERT(NULL != pstUriInfo->pcUriPath);

    pcFullPath = pstUriInfo->pcUriPath;   
    ulSrcUriLen = strlen(pcFullPath);
    
    pcGetChr = (CHAR*)MEM_Find(pcFullPath, ulSrcUriLen, HTTP_URI_HEAD_STR, HTTP_URI_HEAD_STR_LEN);

    pcTempPath = pcFullPath;

    if(NULL == pcGetChr)
    {
        
        ulPathLen = strlen(pcFullPath);
        pcTemp = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulPathLen+1);
        if(NULL == pcTemp)
        {
            return BS_ERR;
        }
        TXT_Strlcpy(pcTemp, pcFullPath, ulPathLen+1);
        HTTP_SET_STR(pstUriInfo->pcUriAbsPath, pcTemp);
        return BS_OK;
    }
    
    ulMatchLen = (ULONG)pcGetChr - (ULONG)pcFullPath;
    ulSrcUriLen -= (ulMatchLen + HTTP_URI_HEAD_STR_LEN);
    pcTempPath = (pcFullPath + ulMatchLen) + HTTP_URI_HEAD_STR_LEN;           

    pcGetChrChr = memchr(pcTempPath, HTTP_BACKSLASH_CHAR, ulSrcUriLen);
    if(NULL != pcGetChrChr)
    {   
        
        ulMatchLen = (ULONG)pcGetChrChr - (ULONG)pcTempPath;  
        ulSrcUriLen -= ulMatchLen;
        pcTemp = http_GetWord(pstHttpHead, pcGetChrChr, ulSrcUriLen);
        if(NULL == pcTemp)
        {
            return BS_ERR;
        }
        HTTP_SET_STR(pstUriInfo->pcUriAbsPath, pcTemp);
        return BS_OK;
    }
    else
    {
        pstUriInfo->pcUriAbsPath = NULL;    
    }
    
    return BS_OK;     
}


static BS_STATUS http_ParseSubUri (IN HTTP_HEAD_S *pstHttpHead)
{
    CHAR *pcFullUri = NULL;
    CHAR *pcTemp = NULL;
    CHAR *pcGetPos = NULL;
    ULONG ulTemp = 0;
    ULONG ulWord = 0;
    ULONG ulSrc = 0;
    HTTP_URI_INFO_S *pstUriInfo;
 

    BS_DBGASSERT(NULL != pstHttpHead);

    pstUriInfo = &pstHttpHead->stUriInfo;

    BS_DBGASSERT(NULL != pstUriInfo->pcFullUri);

    pcFullUri = pstUriInfo->pcFullUri;
    ulSrc = strlen(pcFullUri); 

        
    pcGetPos = memchr(pcFullUri, HTTP_HEAD_URI_FRAGMENT_CHAR, ulSrc);
    if(NULL != pcGetPos)
    {
        ulTemp = (ULONG)pcGetPos - (ULONG)pcFullUri;
        ulWord = ulSrc - (ulTemp + 1);
        pcTemp = http_GetWord(pstHttpHead, pcGetPos+1, ulWord);
        if (NULL == pcTemp)
        {
            return BS_ERR;
        }
        ulSrc = ulTemp;
    }
    HTTP_SET_STR(pstUriInfo->pcUriFragment, pcTemp);
    pcTemp = NULL;
    
    
    pcGetPos = memchr(pcFullUri, HTTP_HEAD_URI_QUERY_CHAR, ulSrc);
    if(NULL != pcGetPos)
    {
        ulTemp = (ULONG)pcGetPos - (ULONG)pcFullUri;
        ulWord = ulSrc - (ulTemp + 1);
        pcTemp = http_GetWord(pstHttpHead, pcGetPos+1, ulWord);
        if (NULL == pcTemp)
        {
            return BS_ERR;
        }
        ulSrc = ulTemp;
    }
    HTTP_SET_STR(pstUriInfo->pcUriQuery, pcTemp);
    pcTemp = NULL;

    
    pcGetPos = memchr(pcFullUri, HTTP_SEMICOLON_CHAR, ulSrc);
    if(NULL != pcGetPos)
    {
        ulTemp = (ULONG)pcGetPos - (ULONG)pcFullUri;
        ulWord = ulSrc - (ulTemp + 1);
        pcTemp = http_GetWord(pstHttpHead, pcGetPos+1, ulWord);
        if (NULL == pcTemp)
        {
            return BS_ERR;
        }
        ulSrc = ulTemp;
    }
    HTTP_SET_STR(pstUriInfo->pcUriParam, pcTemp);
    pcTemp = NULL; 

    
    pcTemp = http_GetWord(pstHttpHead, pcFullUri, ulSrc);
    if(NULL == pcTemp)
    {
        return BS_ERR;
    }
    HTTP_SET_STR(pstUriInfo->pcUriPath, pcTemp);  
    
    return BS_OK;
}



static BS_STATUS http_HeadSetUri (IN HTTP_HEAD_S *pstHttpHead)
{
    ULONG ulRet = BS_ERR;

    
    BS_DBGASSERT(NULL != pstHttpHead);     

    if(BS_OK != http_ParseSubUri(pstHttpHead))
    {
        return BS_ERR;
    }
   
    
    ulRet = http_HeadSetAbsUri(pstHttpHead);
    if(BS_OK != ulRet)
    {
        return BS_ERR;
    }
    
    ulRet = http_HeadSetSimpleUri(pstHttpHead);

    return ulRet;
}


static BS_STATUS http_StrTok(IN CHAR *pcDelim, INOUT HTTP_TOK_DATA_S *pstData, OUT HTTP_TOK_DATA_S *pstFound)
{
    CHAR *pcBegin;
    ULONG ulNewLen;
    CHAR *pcTemp;
    CHAR *pcData;
    ULONG ulDataLen;
    CHAR *pcEnd;
    ULONG ulTokLen;

    BS_DBGASSERT(NULL != pstData);
    BS_DBGASSERT(NULL != pstData->pcData);
    BS_DBGASSERT(NULL != pstFound);
    BS_DBGASSERT(NULL != pcDelim);

    pcData = pstData->pcData;
    ulDataLen = pstData->ulDataLen;
    pcEnd = pcData + ulDataLen;

    pcBegin = HTTP_StrimHead(pcData, ulDataLen, pcDelim);
    if(NULL == pcBegin)
    {
        return BS_NOT_FOUND;
    }

    ulNewLen = ulDataLen - ((ULONG)pcBegin - (ULONG)pcData);
    if (0 == ulNewLen)
    {
        return BS_NOT_FOUND;
    }

    pcTemp = pcBegin;
    while (pcTemp < pcEnd)
    {
        if(NULL != strchr(pcDelim, *pcTemp))
        {
            break;
        }
        pcTemp++;
    }

    ulTokLen = (ULONG)pcTemp - (ULONG)pcBegin;

    pstData->pcData = pcBegin + ulTokLen;
    pstData->ulDataLen = ulNewLen - ulTokLen;

    pstFound->pcData = pcBegin;
    pstFound->ulDataLen = ulTokLen;

    return BS_OK;
}



static BS_STATUS http_ParseHeadFileds
(
    IN HTTP_HEAD_S *pstHttpHead, 
    IN CHAR *pcHeadFields, 
    IN ULONG ulHeadFieldsLen
)
{
    CHAR *pcRealHeadField = pcHeadFields;
    BOOL_T bIsContinue = FALSE;
    ULONG ulRealHeadFieldLen = 0;
    ULONG ulRet;

    
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pcHeadFields);
    BS_DBGASSERT(0 != ulHeadFieldsLen);

    
    bIsContinue = http_IsHaveContinueLine(pcHeadFields, ulHeadFieldsLen);
    if(TRUE == bIsContinue)
    {
        
        pcRealHeadField = http_CompressHead(pstHttpHead, pcHeadFields, ulHeadFieldsLen);
        if(NULL == pcRealHeadField)
        {
            return BS_ERR;
        }
        ulRealHeadFieldLen = strlen(pcRealHeadField);
    }
    else
    {
        
        pcRealHeadField = pcHeadFields;
        ulRealHeadFieldLen = ulHeadFieldsLen;
    }
    
    
    ulRet = http_ScanLine(pcRealHeadField, ulRealHeadFieldLen, http_ParseField, pstHttpHead);

    return ulRet;
}


static BS_STATUS http_GetStatusCode(IN HTTP_TOK_DATA_S *pstTok, OUT HTTP_STATUS_CODE_E *penStatusCode)
{
    CHAR *pcData;
    ULONG ulDataLen;
    UINT uiStatusCode;
    CHAR szStatus[HTTP_STATUS_CODE_LEN + 1];
    BS_DBGASSERT(NULL != pstTok);
    BS_DBGASSERT(NULL != penStatusCode);
    
    pcData    = pstTok->pcData;
    ulDataLen = pstTok->ulDataLen;
    if (HTTP_STATUS_CODE_LEN != ulDataLen)
    {
        return BS_ERR;
    }
    if(TRUE != http_CheckStringIsDigital(pcData, ulDataLen))
    {
        return BS_ERR;
    }
    memcpy(szStatus, pcData, ulDataLen);
    szStatus[ulDataLen] = '\0';
    uiStatusCode = (UINT)atoi(szStatus);
    if (uiStatusCode > (sizeof(g_aucHttpStausCodeMap) - 1))
    {
        *penStatusCode = HTTP_STATUS_CODE_BUTT;
    }
    else
    {
        *penStatusCode = (HTTP_STATUS_CODE_E)g_aucHttpStausCodeMap[uiStatusCode];
    }
    return BS_OK;
}


static HTTP_VERSION_E http_ParseVersion(IN CHAR *pcData, IN ULONG ulDataLen)
{
    CHAR szVersion[HTTP_VERSION_STR_LEN + 1];
    HTTP_VERSION_E enVersion;

    
    BS_DBGASSERT(NULL != pcData);
    
    if(ulDataLen > HTTP_VERSION_STR_LEN)
    {
        return HTTP_VERSION_BUTT;
    }
    memcpy(szVersion, pcData, ulDataLen);
    szVersion[ulDataLen] = '\0';
    
    if(0 == strcmp(szVersion, HTTP_VERSION_1_1_STR))
    {
        enVersion = HTTP_VERSION_1_1;
    }
    else if(0 == strcmp(szVersion, HTTP_VERSION_1_0_STR))
    {
        enVersion = HTTP_VERSION_1_0;
    }
    else if(0 == strcmp(szVersion, HTTP_VERSION_0_9_STR))
    {
        enVersion = HTTP_VERSION_0_9;
    }
    else
    {
        enVersion = HTTP_VERSION_BUTT;
    }
    return enVersion;
}



static BS_STATUS http_ParseRequestFirstLine(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFirstLine, IN ULONG ulFirstLineLen)
{
    CHAR *pcTemp = NULL;
    HTTP_VERSION_E enVer = HTTP_VERSION_BUTT;
    HTTP_HEAD_S *pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    HTTP_TOK_DATA_S stData;
    HTTP_TOK_DATA_S stTok;

    
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pcFirstLine);
    BS_DBGASSERT(0 != ulFirstLineLen);

    memset(&stData, 0, sizeof(stData));
    memset(&stTok, 0, sizeof(stTok));

    stData.pcData = pcFirstLine;
    stData.ulDataLen = ulFirstLineLen;

    
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        return BS_ERR;
    }
    pcTemp = http_GetWord(pstHttpHead, stTok.pcData, stTok.ulDataLen);
    if (NULL == pcTemp)
    {
        return BS_ERR;
    }
    HTTP_SET_STR(pstHttpHead->pcMethodData, pcTemp);
    pstHttpHead->enMethod = http_MethodParse(pstHttpHead->pcMethodData);

    
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        return BS_ERR;
    }
    pcTemp = http_GetWord(pstHttpHead, stTok.pcData, stTok.ulDataLen);
    if (NULL == pcTemp)
    {
        return BS_ERR;
    }
    HTTP_SET_STR(pstHttpHead->stUriInfo.pcFullUri, pcTemp);
    if (BS_OK != http_HeadSetUri(pstHttpHead))
    {
        return BS_ERR;
    }

    
    if (BS_OK == http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        enVer = http_ParseVersion(stTok.pcData, stTok.ulDataLen);   
    }
    
    (VOID)HTTP_SetVersion (hHttpInstance, enVer); 

    return BS_OK;
}



static BS_STATUS http_ParseResponseFirstLine
(
    IN HTTP_HEAD_PARSER hHttpInstance, 
    IN CHAR *pcFirstLine, 
    IN ULONG ulFirstLineLen
)
{
    CHAR *pcTemp = NULL;
    HTTP_VERSION_E enVer = HTTP_VERSION_BUTT;
    HTTP_HEAD_S *pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;  
    HTTP_TOK_DATA_S stData;
    HTTP_TOK_DATA_S stTok;
    HTTP_STATUS_CODE_E enStatusCode = HTTP_STATUS_CODE_BUTT;

    
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pcFirstLine);
    BS_DBGASSERT(0 != ulFirstLineLen);

    memset(&stData, 0, sizeof(stData));
    memset(&stTok, 0, sizeof(stTok));

    stData.pcData = pcFirstLine;
    stData.ulDataLen = ulFirstLineLen;

    
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        return BS_ERR;
    }
    
    enVer = http_ParseVersion(stTok.pcData, stTok.ulDataLen);   

    (VOID)HTTP_SetVersion (hHttpInstance, enVer);

    
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        return BS_ERR;
    }
    
    if (BS_OK != http_GetStatusCode(&stTok, &enStatusCode))
    {
        return BS_ERR;
    }
    pstHttpHead->enStatusCode = enStatusCode; 

    
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        HTTP_SET_STR(pstHttpHead->pcReason, NULL);
        return BS_OK;
    }
    
    
    pcTemp = http_GetWord(pstHttpHead, stTok.pcData, stTok.ulDataLen);
    if(NULL == pcTemp)
    {
        return BS_ERR;
    }
    HTTP_SET_STR(pstHttpHead->pcReason, pcTemp);
    
    return BS_OK;
}



static BS_STATUS http_ParseFCGI
(
    IN HTTP_HEAD_PARSER hHttpInstance, 
    IN CHAR *pcHead, 
    IN ULONG ulHeadLen
)
{    
    CHAR *pcTemp = NULL;
    HTTP_HEAD_S *pstHttpHead;
    HTTP_TOK_DATA_S stData;
    HTTP_TOK_DATA_S stTok;
    CHAR *pcStatusValue;
    HTTP_STATUS_CODE_E enStatusCode = HTTP_STATUS_CODE_BUTT;

    
    BS_DBGASSERT(NULL != hHttpInstance);
    BS_DBGASSERT(NULL != pcHead);

    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    if (0 != ulHeadLen)
    {
        if (BS_OK != http_ParseHeadFileds(pstHttpHead, pcHead, ulHeadLen))
        {
            return BS_ERR;
        }
    }

    
    pcStatusValue = HTTP_GetHeadField(hHttpInstance, HTTP_HEAD_FIELD_STATUS);
    if (NULL == pcStatusValue)
    {
        if (NULL == HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_LOCATION))
        {
            pstHttpHead->enStatusCode = HTTP_STATUS_OK;
        }
        else
        {
            pstHttpHead->enStatusCode = HTTP_STATUS_MOVED_TEMP;
        }
        return BS_OK;
    }

    
    memset(&stData, 0, sizeof(stData));
    memset(&stTok, 0, sizeof(stTok));
    stData.pcData = pcStatusValue;
    stData.ulDataLen = strlen(pcStatusValue);

    
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        return BS_ERR;
    }

    if (BS_OK != http_GetStatusCode(&stTok, &enStatusCode))
    {
        return BS_ERR;
    }
    pstHttpHead->enStatusCode = enStatusCode;

    
    if (BS_OK == http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        pcTemp = http_GetWord(pstHttpHead, stTok.pcData, stTok.ulDataLen);
        if(NULL == pcTemp)
        {
            return BS_ERR;
        }
        HTTP_SET_STR(pstHttpHead->pcReason, pcTemp);        
    }

    
    (VOID) HTTP_DelHeadField(hHttpInstance, HTTP_HEAD_FIELD_STATUS);
    
    return BS_OK;
}


static BS_STATUS http_ParseFirstLine
(
    IN HTTP_HEAD_PARSER hHttpInstance, 
    IN CHAR *pcFirstLine, 
    IN ULONG ulFirstLineLen, 
    IN HTTP_SOURCE_E enSourceType
)
{
    ULONG ulRet = 0;
    HTTP_HEAD_S *pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pcFirstLine);
    BS_DBGASSERT(0 != ulFirstLineLen);
    
    switch(enSourceType)
    {
        case HTTP_REQUEST:  
        {
            
            ulRet = http_ParseRequestFirstLine(pstHttpHead, pcFirstLine, ulFirstLineLen);
            break;
        }
        case HTTP_RESPONSE:
        {
            
            ulRet = http_ParseResponseFirstLine(pstHttpHead, pcFirstLine, ulFirstLineLen);
            break;
        }                    
        default:
        {
            ulRet = BS_ERR;
            break;
        }
    }

    return ulRet;
}


BS_STATUS HTTP_ParseHead(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcHead, IN ULONG ulHeadLen, IN HTTP_SOURCE_E enSourceType)
{
    HTTP_HEAD_S *pstHttpHead = NULL;                        
    ULONG ulUsefulHeadLen = 0;                               
    CHAR *pcRealHead;
    CHAR *pcLineEnd;
    ULONG ulFirstLineLen;
    CHAR *pcHeadFields;
    ULONG ulHeadFieldsLen;

    
    if((NULL == hHttpInstance) || (NULL == pcHead) || (ulHeadLen < HTTP_CRLFCRLF_LEN))
    {
        return BS_ERR;
    }

    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    
    pcRealHead = HTTP_Strim(pcHead, ulHeadLen, HTTP_WHITESPACE, &ulUsefulHeadLen);   
    if(NULL == pcRealHead)
    {
        return BS_ERR;
    }

    if (HTTP_FCGI_RESPONSE == enSourceType)
    {
        return http_ParseFCGI(hHttpInstance, pcRealHead, ulUsefulHeadLen);
    }

    if(0 == ulUsefulHeadLen)
    {
        
        return BS_ERR;
    }

    if (HTTP_PART_HEADER == enSourceType)
    {
        return http_ParseHeadFileds(pstHttpHead, pcHead, ulUsefulHeadLen);
    }
    

    
    pcLineEnd = (CHAR*)MEM_Find(pcRealHead, ulUsefulHeadLen, HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);
    if (NULL == pcLineEnd)
    {
        ulFirstLineLen = ulUsefulHeadLen;
    }
    else
    {
        ulFirstLineLen = (ULONG)pcLineEnd - (ULONG)pcRealHead;
    }
    
    
    if (BS_OK != http_ParseFirstLine(hHttpInstance, pcRealHead, ulFirstLineLen, enSourceType))
    {
        return BS_ERR;
    }

    
    if (NULL == pcLineEnd)
    {
        return BS_OK;
    }

    
    pcHeadFields = pcLineEnd + HTTP_LINE_END_FLAG_LEN;
    ulHeadFieldsLen = ulUsefulHeadLen - (ulFirstLineLen + HTTP_LINE_END_FLAG_LEN);
    if (BS_OK != http_ParseHeadFileds(pstHttpHead, pcHeadFields, ulHeadFieldsLen))
    {
        return BS_ERR;
    }
    return BS_OK;
}

time_t HTTP_StrToDate(IN UCHAR *pucValue, IN ULONG ulLen)
{
    UCHAR *pucPos;
    UCHAR *pucEnd;
    INT iMonth;
    UINT uiDay;
    UINT uiYear;
    UINT uiHour;
    UINT uiMin;
    UINT uiSec;
    ULONG ulTime;
    HTTP_TIME_STANDARD_E enFmt;

    
    if((NULL == pucValue)||(0 == ulLen))
    {
        return 0;
    }

    enFmt = NORULE;
    uiDay = 0;
    uiYear = 0;
    uiHour = 0;
    uiMin = 0;
    uiSec = 0;
    pucPos = pucValue;
    pucEnd = pucValue + ulLen;

    
    for (pucPos = pucValue; pucPos < pucEnd; pucPos++) 
    {
        if (',' == *pucPos) 
        {
            break;
        }

        if (' '==*pucPos) 
        {
            enFmt = ISOC;
            break;
        }
    }

    
    for (pucPos++; pucPos < pucEnd; pucPos++)
    {
        if (' ' != *pucPos) 
        {
            break;
        }
    }

    
    if (HTTP_TIME_REMAIN_MAX_LEN > (pucEnd - pucPos)) 
    {
        return 0;
    }

    if (ISOC != enFmt) 
    {
        if (('0' > *pucPos) || 
            ('9' < *pucPos) || 
            ('0' > *(pucPos + 1)) || 
            ('9' < *(pucPos + 1))) 
        {
            return 0;
        }
        
        uiDay = (((*pucPos - '0') * 10) + (*(pucPos + 1))) - '0';
        
        pucPos += 2; 
        if (' ' == *pucPos) 
        {
            if (HTTP_TIME_REMAIN_MAX_LEN > (pucEnd - pucPos)) 
            {
                return 0;
            }
            
            enFmt = RFC822;

        } 
        else if ('-' == *pucPos) 
        {
            
            enFmt = RFC850;
        } 
        else 
        {
            return 0;
        }

        pucPos++;
    }

    
    switch (*pucPos) 
    {

        case 'J':
        {
            iMonth = (*(pucPos + 1) == 'a' )? 0 : ((*(pucPos + 2) == 'n' )? 5 : 6);   
            break;
        }

        case 'F':
        {
            iMonth = 1;
            break;
        }

        case 'M':
        {
            iMonth = (*(pucPos + 2) == 'r') ? 2 : 4;  
            break;
        }

        case 'A':
        {
            iMonth = (*(pucPos + 1) == 'p') ? 3 : 7; 
            break;
        }
        case 'S':
        {
            iMonth = 8;
            break;
        }
        case 'O':
        {
            iMonth = 9;
            break;
        }
        case 'N':
        {
            iMonth = 10;
            break;
        }
        case 'D':
        {
            iMonth = 11;
            break;
        }
        default:
        {
            return 0;
        }
    }

    
    pucPos += 3;

    
    if (((RFC822 == enFmt) && (' ' != *pucPos)) || 
        ((RFC850 == enFmt) && ('-' != *pucPos))) 
    {
        return 0;
    }

    pucPos++;
    
    if (RFC822 == enFmt) 
    {
        if (('0' > *pucPos) || 
            ('9' < *pucPos) || 
            ('0' > *(pucPos + 1)) || 
            ('9' < *(pucPos + 1)) || 
            ('0' > *(pucPos + 2)) || 
            ('9' < *(pucPos + 2)) || 
            ('0' > *(pucPos + 3)) || 
            ('9' < *(pucPos + 3)))
        {
            return 0;
        }

        uiYear = ((*pucPos - '0') * 1000) + 
                 ((*(pucPos + 1) - '0') * 100 )+ 
                 ((*(pucPos + 2) - '0') * 10) + 
                  (*(pucPos + 3) - '0'); 
        pucPos += 4;

    } 
    else if (RFC850 == enFmt) 
    {
        if (('0' > *pucPos) || 
            ('9' < *pucPos) || 
            ('0' > *(pucPos + 1)) || 
            ('9' < *(pucPos + 1))) 
        {
            return 0;
        }

        uiYear = (((*pucPos - '0') * 10) + *(pucPos + 1)) - '0';
        uiYear += (uiYear < 70) ? 2000 : 1900; 
        pucPos += 2;
    }
    else
    {
        
    }

    if (ISOC == enFmt) 
    {
        
        if (' ' == *pucPos) 
        {
            pucPos++;
        }

        if (('0' > *pucPos) || 
            ('9' < *pucPos)) 
        {
            return 0;
        }

        uiDay = *pucPos++ - '0'; 

        if (' ' != *pucPos) 
        {
            if (('0' > *pucPos) || 
                ('9' < *pucPos)) 
            {
                return 0;
            }

            uiDay = ((uiDay * 10) + *pucPos++) - '0'; 
        }
        
        if (HTTP_TIME_ISOC_REMAIN_LEN > (pucEnd - pucPos)) 
        {
            return 0;
        }
    }

    if (' ' != *pucPos++) 
    {
        return 0;
    }

    
    if (('0' > *pucPos) || 
        ('9' < *pucPos) || 
        ('0' > *(pucPos + 1)) || 
        ('9' < *(pucPos + 1))) 
    {
        return 0;
    }

    
    uiHour = (((*pucPos - '0') * 10) + *(pucPos + 1)) - '0';
    pucPos += 2;

    if (HTTP_HEAD_FIELD_SPLIT_CHAR != *pucPos++) 
    {
        return 0;
    }

    if (('0' > *pucPos) || 
        ('9' < *pucPos) || 
        ('0' > *(pucPos + 1)) || 
        ('9' < *(pucPos + 1))) 
    {
        return 0;
    }
    
    uiMin = (((*pucPos - '0') * 10) + *(pucPos + 1)) - '0';
    pucPos += 2;

    if (HTTP_HEAD_FIELD_SPLIT_CHAR != *pucPos++) 
    {
        return 0;
    }

    if (('0' > *pucPos) || 
        ('9' < *pucPos) || 
        ('0' > *(pucPos + 1)) || 
        ('9' < *(pucPos + 1))) 
    {
        return 0;
    }
    
    uiSec = (((*pucPos - '0') * 10) + *(pucPos + 1)) - '0';

    
    if (ISOC == enFmt) 
    {
        pucPos += 2;

        if (' ' != *pucPos++) 
        {
            return 0;
        }

        if (('0' > *pucPos) || 
            ('9' < *pucPos) || 
            ('0' > *(pucPos + 1)) || 
            ('9' < *(pucPos + 1)) || 
            ('0' > *(pucPos + 2)) || 
            ('9' < *(pucPos + 2)) || 
            ('0' > *(pucPos + 3)) || 
            ('9' < *(pucPos + 3)))
        {
            return 0;
        }

        uiYear = (((*pucPos - '0') * 1000) + 
                 ((*(pucPos + 1) - '0') * 100 )+ 
                 ((*(pucPos + 2) - '0') * 10) + 
                  *(pucPos + 3)) - '0';
    }

    if ((23 < uiHour) || 
        (59 < uiMin) || 
        (59 < uiSec)) 
    {
         return 0;
    }

    
    if ((29 == uiDay) && 
        (1 == iMonth)) 
    {
        if ((0 < (uiYear & 3)) || 
            ((0 == (uiYear % 100)) && (0 != (uiYear % 400)))) 
        {
            return 0;
        }

    }
    
    else if (uiDay > g_auiHttpDay[iMonth]) 
    {
        return 0;
    }
    else
    {
        
    }

    

    if (0 >= --iMonth) 
    {
        
        iMonth += 12;
        uiYear -= 1;
    }

    

    ulTime = (ULONG)365 * uiYear;
    ulTime += uiYear / 4;
    ulTime -= uiYear / 100;
    ulTime += uiYear / 400;
    ulTime += ((367 * (ULONG)((UINT)iMonth)) / 12);
    ulTime = (ulTime + uiDay) - 719499;
    ulTime = ulTime * 86400;
    ulTime += ((uiHour * 3600UL) + (uiMin * 60UL) + uiSec);

    return (time_t) ulTime;
}


BS_STATUS HTTP_DateToStr(IN time_t stTimeSrc, OUT CHAR szDataStr[HTTP_RFC1123_DATESTR_LEN + 1])
{    
    LONG   lYday;
    ULONG  ulTime, ulSec, ulMin, ulHour, ulMday, ulMon, ulYear, ulWday, ulDays, ulLeap;
    ULONG  ulTemp;
    

    

    ulTime = (unsigned int) stTimeSrc;

    ulDays = ulTime / 86400;

    

    ulWday = (4 + ulDays) % 7;

    ulTime %= 86400;
    ulHour = ulTime / 3600;
    ulTime %= 3600;
    ulMin = ulTime / 60;
    ulSec = ulTime % 60;

    

    
    ulDays = (ulDays - (31 + 28)) + 719527;

    

    ulYear = ((ulDays + 2) * 400 )/ (((365 * 400) + (100 - 4)) + 1);

    ulTemp = (365 * ulYear) + (ulYear / 4);
    ulTemp = ulTemp - (ulYear / 100);
    ulTemp = ulTemp + (ulYear / 400);
    lYday = (LONG)ulDays - (LONG)ulTemp;

    if (lYday < 0) {
        if((0 == (ulYear % 4)) && (((ulYear % 100) != 0) || ((ulYear % 400) == 0)))
        {
            ulLeap = 1;
        }
        else
        {
            ulLeap = 0;
        }
        lYday = (365 + (LONG)ulLeap) + lYday;
        ulYear--;
    }

    

    ulMon = (((ULONG)lYday + 31) * 10) / 306;

    

    ulMday = ((ULONG)lYday - (((367 * ulMon) / 12) - 30)) + 1;

    if (lYday >= 306) {

        ulYear++;
        ulMon -= 10;

        

    } else {

        ulMon += 2;

        
    }    

    if( ( ulWday >= 7 ) || ( ulMon > 12 ) || ( ulMon < 1 ) )
    {
        return BS_ERR;
    }

    
    (VOID)scnprintf( szDataStr, HTTP_RFC1123_DATESTR_LEN + 1, "%3s, %02lu %3s %lu %02lu:%02lu:%02lu GMT",
                    g_apcWkday[ulWday],
                    ulMday,
                    g_apcMonth[ulMon - 1],
                    ulYear,
                    ulHour,
                    ulMin,
                    ulSec );
    
    return BS_OK;
}


VOID HTTP_DelHeadField(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFieldName)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    DLL_NODE_S *pstCurrentNode = NULL;
    HTTP_HEAD_FIELD_S *pstHeadField = NULL;
    HTTP_HEAD_FIELD_S *pstHeadFieldFound = NULL;

    
    if((NULL == hHttpInstance) || (NULL == pcFieldName))
    {
        BS_DBGASSERT(0);
        return;
    }

    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    
    DLL_SCAN(&(pstHttpHead->stHeadFieldList), pstCurrentNode)
    {
        pstHeadField = (HTTP_HEAD_FIELD_S *)pstCurrentNode;
        if(0 == stricmp(pstHeadField->pcFieldName, pcFieldName))
        {
            pstHeadFieldFound = pstHeadField;
            break;
        }
    }

    if (NULL != pstHeadFieldFound)
    {
        HTTP_ClearHF(pstHttpHead);

        DLL_DEL(&(pstHttpHead->stHeadFieldList), pstHeadFieldFound);

        if (NULL != pstHeadFieldFound->pcFieldName)
        {
            MEMPOOL_Free(pstHttpHead->hMemPool, pstHeadFieldFound->pcFieldName);
        }
        if (NULL != pstHeadFieldFound->pcFieldValue)
        {
            MEMPOOL_Free(pstHttpHead->hMemPool, pstHeadFieldFound->pcFieldValue);
        }

        MEMPOOL_Free(pstHttpHead->hMemPool, pstHeadFieldFound);
    }
    
    return;    
}


BS_STATUS HTTP_SetHeadField(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFieldName, IN CHAR *pcFieldValue)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    HTTP_HEAD_FIELD_S *pstHeadField = NULL;
    CHAR *pcTempName = NULL;
    CHAR *pcTempValue = NULL;    

        
    if((NULL == hHttpInstance) || (NULL == pcFieldName) || (NULL == pcFieldValue))
    {
        return BS_ERR;
    }
    
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    
    pstHeadField = (HTTP_HEAD_FIELD_S *)MEMPOOL_ZAlloc(pstHttpHead->hMemPool, sizeof(HTTP_HEAD_FIELD_S));
    if(NULL == pstHeadField)
    {
        return BS_ERR;
    }

    
    pcTempName  = MEMPOOL_Strdup(pstHttpHead->hMemPool, pcFieldName);;
    if (! pcTempName) {
        MEMPOOL_Free(pstHttpHead->hMemPool, pstHeadField);
        RETURN(BS_NO_MEMORY);
    }
    pcTempValue = MEMPOOL_Strdup(pstHttpHead->hMemPool, pcFieldValue);
    if (! pcTempValue) {
        MEMPOOL_Free(pstHttpHead->hMemPool, pstHeadField);
        MEMPOOL_Free(pstHttpHead->hMemPool, pcTempName);
        RETURN(BS_NO_MEMORY);
    }

    pstHeadField->pcFieldName  = pcTempName;
    pstHeadField->pcFieldValue = pcTempValue;
    pstHeadField->uiFieldNameLen = strlen(pcTempName);

    
    DLL_ADD(&(pstHttpHead->stHeadFieldList), (DLL_NODE_S *)pstHeadField);
    return BS_OK;    
}

VOID HTTP_SetNoCache(IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_PRAGMA, "no-cache");
    HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_CATCH_CONTROL, "no-cache");
}


VOID HTTP_SetRevalidate(IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_CATCH_CONTROL, "max-age=0");
}


CHAR * HTTP_GetHeadField(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFieldName)
{
    HTTP_HEAD_S *pstHttpHead = hHttpInstance;
    DLL_NODE_S *pstCurrentNode = NULL;
    HTTP_HEAD_FIELD_S *pstHeadField = NULL;

    BS_DBGASSERT(NULL != hHttpInstance);
    BS_DBGASSERT(NULL != pcFieldName);

    UCHAR len = strlen(pcFieldName);

    
    DLL_SCAN(&(pstHttpHead->stHeadFieldList), pstCurrentNode)
    {
        pstHeadField = (HTTP_HEAD_FIELD_S *)pstCurrentNode;
        if ((len == pstHeadField->uiFieldNameLen) && (0 == stricmp(pstHeadField->pcFieldName, pcFieldName)))
        {
            return pstHeadField->pcFieldValue;
        }
    }
    return NULL;    
}


CHAR * HTTP_GetHF(HTTP_HEAD_PARSER hHttpInstance, HTTP_HEAD_FIELD_E field)
{
    HTTP_HEAD_S *pstHttpHead = hHttpInstance;
    char *value = NULL;

    BS_DBGASSERT(field < HTTP_HF_MAX);

    if (pstHttpHead->head_fields[field] == UINT_HANDLE(1)) {
        return NULL;
    }

    if (pstHttpHead->head_fields[field]) {
        return pstHttpHead->head_fields[field];
    }

    value = HTTP_GetHeadField(pstHttpHead, g_http_head_hf_fields[field]);

    if (value) {
        pstHttpHead->head_fields[field] = value;
    } else {
        pstHttpHead->head_fields[field] = UINT_HANDLE(1);
    }

    return value;
}

void HTTP_ClearHF(HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = hHttpInstance;
    int i;

    for (i=0; i<HTTP_HF_MAX; i++) {
        pstHttpHead->head_fields[i] = NULL;
    }
}


HTTP_HEAD_FIELD_S * HTTP_GetNextHeadField(IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_HEAD_FIELD_S *pstHeadFieldNode)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    HTTP_HEAD_FIELD_S *pstResultNode = NULL;

    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    if (NULL == pstHeadFieldNode)
    {
        pstResultNode = DLL_FIRST(&(pstHttpHead->stHeadFieldList));
    }
    else
    {
        pstResultNode = DLL_NEXT(&(pstHttpHead->stHeadFieldList), pstHeadFieldNode);
    }

    return pstResultNode;
}



BS_STATUS HTTP_GetContentLen(IN HTTP_HEAD_PARSER hHttpInstance, OUT UINT64 *puiContentLen)
{
    
    CHAR *pcContentLen;
    UINT uiLen = 0;
    
    
    if(NULL == hHttpInstance)
    {
        return BS_NULL_PARA;
    }
    
    pcContentLen = HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_CONTENT_LENGTH);
    
    if (NULL == pcContentLen)
    {
        return BS_NO_SUCH;
    }

    TXT_Atoui(pcContentLen, &uiLen);
    *puiContentLen = uiLen;

    return BS_OK;
}


BS_STATUS HTTP_SetContentLen(IN HTTP_HEAD_PARSER hHttpInstance, IN UINT64 uiValue)
{
    
    CHAR szContentLen[HTTP_MAX_UINT64_LEN+1];

    
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    
    
    scnprintf(szContentLen, sizeof(szContentLen), "%llu", uiValue);
    return HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_CONTENT_LENGTH, szContentLen);
}


CHAR * HTTP_GetUriPath (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;

    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcUriPath;
}


BS_STATUS HTTP_SetUriPath(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcUri, IN UINT uiUriLen)
{ 
    HTTP_HEAD_S *pstHttpHead = NULL;
    CHAR *pcTemp = NULL;

    
    if((NULL == hHttpInstance)||(NULL == pcUri))
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    pcTemp = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, uiUriLen + 1);
    if(NULL == pcTemp)
    {
        return BS_ERR;
    }
    pcTemp[0] = '\0';
    TXT_Strlcpy(pcTemp, pcUri, uiUriLen + 1);
    
    HTTP_SET_STR(pstHttpHead->stUriInfo.pcUriPath, pcTemp);
    return BS_OK;
}



CHAR * HTTP_GetUriAbsPath (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcUriAbsPath;
}


CHAR * HTTP_GetSimpleUriAbsPath (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcSimpleAbsPath;
}


CHAR * HTTP_GetFullUri (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcFullUri;
}


CHAR * HTTP_GetUriQuery (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcUriQuery;
}

VOID HTTP_ClearUriQuery(IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    pstHttpHead->stUriInfo.pcUriQuery = NULL;
}


BS_STATUS HTTP_SetUriQuery (IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcQuery)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    size_t ulSize = 0;
    CHAR *pcTemp = NULL;
    
    
    if((NULL == hHttpInstance)||(NULL == pcQuery))
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    ulSize = strlen(pcQuery);
    pcTemp = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulSize + 1);
    if(NULL == pcTemp)
    {
        return BS_ERR;
    }
    pcTemp[0] = '\0';
    TXT_Strlcpy(pcTemp, pcQuery, ulSize + 1);

    HTTP_SET_STR(pstHttpHead->stUriInfo.pcUriQuery , pcTemp);    
    return BS_OK;
}


CHAR * HTTP_GetUriFragment (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcUriFragment;
}


BS_STATUS HTTP_SetUriFragment(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFragment)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    size_t ulSize = 0;
    CHAR *pcTemp = NULL;
    
    
    if((NULL == hHttpInstance) || (NULL == pcFragment))
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    ulSize = strlen(pcFragment);
    pcTemp = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulSize + 1);
    if(NULL == pcTemp)
    {
        return BS_ERR;
    }
    pcTemp[0] = '\0';
    TXT_Strlcpy(pcTemp, pcFragment, ulSize + 1);

    HTTP_SET_STR(pstHttpHead->stUriInfo.pcUriFragment , pcTemp);    
    return BS_OK;
}


HTTP_VERSION_E HTTP_GetVersion(IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return HTTP_VERSION_BUTT;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->enHttpVersion;
}


CHAR*  HTTP_VersionConversionStr(IN HTTP_VERSION_E enVer)
{
    CHAR *pcVersionStr = NULL;
    
    switch(enVer)
    {
        case HTTP_VERSION_1_1:
        {
            pcVersionStr = HTTP_VERSION_1_1_STR;
            break;
        }   
        case HTTP_VERSION_1_0:
        {
            pcVersionStr = HTTP_VERSION_1_0_STR;
            break;
        }            
        case HTTP_VERSION_0_9:
        {
            pcVersionStr = HTTP_VERSION_0_9_STR;
            break;
        }            
        default:
        {
            pcVersionStr = HTTP_VERSION_UNKNOWN_STR;
            break;
        }                   
    }
    
    return pcVersionStr;
}


BS_STATUS HTTP_SetVersion (IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_VERSION_E enVer)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    pstHttpHead->enHttpVersion = enVer;
    return BS_OK;
}


HTTP_METHOD_E HTTP_GetMethod (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return HTTP_METHOD_BUTT;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->enMethod;
}


BS_STATUS  HTTP_SetMethod (IN HTTP_HEAD_PARSER hHttpInstance,  IN HTTP_METHOD_E enMethod)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    pstHttpHead->enMethod = enMethod;
    return BS_OK;
}


CHAR* HTTP_GetMethodStr(IN HTTP_METHOD_E enMethod )
{
    
    if(enMethod > HTTP_METHOD_CONNECT)
    {
        return NULL;
    }
    return (CHAR *)g_apcHttpParseMethodStrTable[enMethod];
}


CHAR* HTTP_GetMethodData (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->pcMethodData;
}


HTTP_STATUS_CODE_E HTTP_GetStatusCode( IN HTTP_HEAD_PARSER hHttpInstance )
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return HTTP_STATUS_CODE_BUTT;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->enStatusCode;
}


BS_STATUS HTTP_SetStatusCode( IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_STATUS_CODE_E enStatusCode)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    pstHttpHead->enStatusCode = enStatusCode;
    return BS_OK;
}


CHAR * HTTP_GetReason ( IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->pcReason;
}


BS_STATUS HTTP_SetReason (IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcReason)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    size_t ulSize = 0;
    CHAR *pcTemp = NULL;

    
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }

    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    if(NULL != pcReason)
    {
        ulSize = strlen(pcReason);
        pcTemp = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulSize + 1);
        if(NULL == pcTemp)
        {
            return BS_ERR;
        }
        pcTemp[0] = '\0';
        TXT_Strlcpy(pcTemp, pcReason, ulSize + 1);
    }    

    HTTP_SET_STR(pstHttpHead->pcReason, pcTemp);  

    return BS_OK;
}


static BS_STATUS http_DecodeHex(IN CHAR *pcStrBegin, IN CHAR *pcStrEnd, OUT CHAR *pcValue)
{
    CHAR szTmp[HTTP_COPY_LEN+1] = {0};
    INT iValue = 0;
    
    BS_DBGASSERT(NULL != pcStrBegin);
    BS_DBGASSERT(NULL != pcStrEnd);
    BS_DBGASSERT(NULL != pcValue); 
    
    
    if(((ULONG)pcStrEnd - (ULONG)pcStrBegin) < HTTP_COPY_LEN)
    {
        return BS_ERR;
    }
    if((!isxdigit(pcStrBegin[1])) || (!isxdigit(pcStrBegin[2])))
    {
        return BS_ERR;
    }

    memcpy( szTmp, pcStrBegin + 1, HTTP_COPY_LEN );
    szTmp[HTTP_COPY_LEN] = '\0';
    iValue = (INT)strtol(szTmp, (CHAR **)NULL, 16);

    
    *pcValue = (CHAR)iValue;
    return BS_OK;
}


CHAR * HTTP_UriDecode(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcUri, IN ULONG ulUriLen)
{
    
    CHAR  *pcDesTmp = NULL;
    CHAR  *pcSrcTmp = NULL;
    CHAR  *pcSrcEnd = NULL;
    CHAR  *pcBuf = NULL;
    ULONG ulErrCode = BS_OK;
    CHAR cValue;

    
    if((NULL == pcUri) || (0 == ulUriLen))
    {
        return NULL;
    }
    
    
    if ( ulUriLen > HTTP_MAX_HEAD_LENGTH )
    {
        return NULL;
    }

    pcSrcTmp = pcUri;
    pcSrcEnd = pcUri + ulUriLen;
    
    
    pcBuf = (CHAR *)MEMPOOL_Alloc(hMemPool, ulUriLen + 1 );
    if( NULL == pcBuf )
    {
        return NULL;
    }
    pcBuf[0] = '\0';

    
    pcDesTmp = pcBuf;
    while( pcSrcTmp < pcSrcEnd )
    {
        cValue = *pcSrcTmp;
                
        if ('%' == cValue)
        {
            ulErrCode = http_DecodeHex(pcSrcTmp, (pcSrcEnd-1), &cValue);    
            if(BS_OK != ulErrCode)
            {
                break;
            }
            pcSrcTmp += (HTTP_COPY_LEN + 1);
        }
        else
        {
            pcSrcTmp++;
        }
        *pcDesTmp = cValue;
        pcDesTmp++;
    }
    
    *pcDesTmp = '\0';
    if(BS_OK != ulErrCode)
    {
        MEMPOOL_Free(hMemPool, pcBuf);
        pcBuf = NULL;
    }

    return pcBuf;
}


CHAR * HTTP_UriEncode(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcUri, IN ULONG ulUriLen)
{
    
    CHAR *pcSrcTmp = NULL;
    CHAR *pcSrcEnd = NULL;    
    CHAR *pcDstTmp = NULL;
    CHAR *pcBuf = NULL;
    ULONG ulLen = 0;

    
    if((NULL == pcUri) || (0 == ulUriLen))
    {
        return NULL;
    }

    pcSrcTmp = pcUri;
    pcSrcEnd = pcUri + ulUriLen;

    
    ulLen = (3 * ulUriLen) + 1;
    pcBuf = (CHAR *)MEMPOOL_Alloc(hMemPool, ulLen);
    if( NULL == pcBuf )
    {
        return NULL;
    }
    pcBuf[0] = '\0';

    pcDstTmp = pcBuf;
    while ( pcSrcTmp < pcSrcEnd )
    {
        if( ('\t' == *pcSrcTmp) ||
            (' '  == *pcSrcTmp) ||
            ('%'  == *pcSrcTmp) )
        {
            
            (VOID)scnprintf( pcDstTmp, ulLen, "%%" );
            pcDstTmp++;
            ulLen--;

            (VOID)scnprintf( pcDstTmp, ulLen, "%02X", *pcSrcTmp );
            pcDstTmp += 2;
            ulLen -= 2;
            
            pcSrcTmp++;            
            continue;
        }

        *pcDstTmp = *pcSrcTmp;
        pcDstTmp++;
        pcSrcTmp++;
        ulLen--;
    }

    *pcDstTmp = '\0';
    
    return pcBuf;
}


CHAR * HTTP_DataDecode(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcData, IN ULONG ulDataLen)
{
    
    CHAR  *pcDesTmp = NULL;
    CHAR  *pcSrcTmp = NULL;
    CHAR  *pcSrcEnd = NULL;    
    CHAR  *pcBuf = NULL;
    ULONG ulErrCode = BS_OK;    
    CHAR  cValue;
    
    
    if((NULL == pcData) || (0 == ulDataLen))
    {
        return NULL;
    }

    pcSrcTmp = pcData;
    pcSrcEnd = pcData + ulDataLen;
    
    
    pcBuf = (CHAR *)MEMPOOL_Alloc(hMemPool, ulDataLen + 1 );
    if( NULL == pcBuf )
    {
        return NULL;
    }
    pcBuf[0] = '\0';


    
    pcDesTmp = pcBuf;
    while( pcSrcTmp < pcSrcEnd )
    {
        cValue = *pcSrcTmp;
        
                
        if ('%' == cValue)
        {
            ulErrCode = http_DecodeHex(pcSrcTmp, (pcSrcEnd-1), &cValue);    
            if(BS_OK != ulErrCode)
            {
                break;
            }
            pcSrcTmp += (HTTP_COPY_LEN + 1);
        }

        else if('+' == cValue)
        {
            cValue = ' ';
            pcSrcTmp++;
        }
        else
        {
            pcSrcTmp++;
        }

        
        *pcDesTmp = cValue;
        pcDesTmp++;
    }

    *pcDesTmp = '\0';
    if(BS_OK != ulErrCode)
    {
        MEMPOOL_Free(hMemPool, pcBuf);
        pcBuf = NULL;
    }

    return pcBuf;
}


CHAR * HTTP_DataEncode(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcData, IN ULONG ulDataLen)
{    
        
    CHAR *pcSrcTmp = NULL;
    CHAR *pcSrcEnd = NULL;
    CHAR *pcDstTmp = NULL;
    CHAR *pcBuf = NULL;
    ULONG ulLen = 0;

    
    if((NULL == pcData) || (0 == ulDataLen))
    {
        return NULL;
    }

    pcSrcTmp = pcData;
    pcSrcEnd = pcData + ulDataLen;
    
    
    ulLen = (3 * ulDataLen) + 1;
    pcBuf = (CHAR *)MEMPOOL_Alloc(hMemPool, ulLen);
    if( NULL == pcBuf )
    {
        return NULL;
    }   
    pcBuf[0] = '\0';
 
    pcDstTmp = pcBuf;
    while ( pcSrcTmp < pcSrcEnd )
    {
        if(' ' == *pcSrcTmp)
        {
            
            (VOID)scnprintf( pcDstTmp, ulLen, "+" );
            pcDstTmp++;
            pcSrcTmp++;
            ulLen--;
            continue;
        }

        if(NULL != strchr(g_pcEncode, *pcSrcTmp))
        {      
            (VOID)scnprintf( pcDstTmp, ulLen, "%%" );
            pcDstTmp++;
            ulLen--;
            
            (VOID)scnprintf( pcDstTmp, ulLen, "%02X", *pcSrcTmp );
            pcDstTmp += 2;
            ulLen -= 2;

            pcSrcTmp++;            
            continue;
        }

        *pcDstTmp = *pcSrcTmp;
        pcDstTmp++;
        pcSrcTmp++;
        ulLen--;
    }

    *pcDstTmp = '\0';

    return pcBuf;
}


CHAR * HTTP_GetUriParam (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcUriParam;
}


BS_STATUS HTTP_SetUriParam (IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcParam)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    size_t ulSize = 0;
    CHAR *pcTemp = NULL;

    
    if((NULL == hHttpInstance) || (NULL == pcParam))
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    ulSize = strlen(pcParam);
    pcTemp = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulSize + 1);
    if(NULL == pcTemp)
    {
        return BS_ERR;
    }
    pcTemp[0] = '\0';
    TXT_Strlcpy(pcTemp, pcParam, ulSize + 1);

    HTTP_SET_STR(pstHttpHead->stUriInfo.pcUriParam, pcTemp);
    return BS_OK;
}


static BS_STATUS http_BuildRequestMethod(IN HTTP_METHOD_E enMethod, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    CHAR *pcGetWord = NULL;
    ULONG ulRet;

    
    BS_DBGASSERT(NULL != pstBuf);

    
    pcGetWord = HTTP_GetMethodStr(enMethod);
    if(NULL == pcGetWord)
    {
        return BS_ERR;
    }

    ulRet  = http_AppendBufStr(pcGetWord, pstBuf);
    ulRet |= http_AppendBufStr(HTTP_SPACE_FLAG, pstBuf);

    if(BS_OK != ulRet)
    {
        ulRet = BS_ERR;
    }
    
    return ulRet;
}


static BS_STATUS http_BuildRequestFullURI (IN HTTP_URI_INFO_S *pstURIInfo, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    CHAR *pcUriPathTmp;
    ULONG ulRet;
    
        
    BS_DBGASSERT (NULL != pstURIInfo);
    BS_DBGASSERT (NULL != pstBuf);

    
    if(NULL != pstURIInfo->pcUriPath)
    {
        pcUriPathTmp = pstURIInfo->pcUriPath;
    }
    else
    {
            
        pcUriPathTmp = HTTP_BACKSLASH_STRING;
    }

    
    ulRet  = http_AppendBufStr(pcUriPathTmp, pstBuf);

    
    if(NULL != pstURIInfo->pcUriParam)
    {
        
        ulRet |= http_AppendBufStr(HTTP_SEMICOLON_STRING, pstBuf);
        ulRet |= http_AppendBufStr(pstURIInfo->pcUriParam, pstBuf);
    }    

    
    if(NULL != pstURIInfo->pcUriQuery)
    {
        
        ulRet |= http_AppendBufStr(HTTP_HEAD_URI_QUERY_STRING, pstBuf);
        ulRet |= http_AppendBufStr(pstURIInfo->pcUriQuery, pstBuf);
    }

    
    if(NULL != pstURIInfo->pcUriFragment)
    {
        
        ulRet |= http_AppendBufStr(HTTP_HEAD_URI_FRAGMENT_STRING, pstBuf);
        ulRet |= http_AppendBufStr(pstURIInfo->pcUriFragment, pstBuf);
    }

    
    ulRet |= http_AppendBufStr(HTTP_SPACE_FLAG, pstBuf);

    if(BS_OK != ulRet)
    {
        ulRet = BS_ERR;
    }        
    return ulRet;     
}


static BS_STATUS http_BuildHeadField(IN HTTP_HEAD_FIELD_S *pstHeadField, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    ULONG ulRet;

    
    BS_DBGASSERT(NULL != pstHeadField);
    BS_DBGASSERT(NULL != pstBuf);

    
    ulRet  = http_AppendBufStr(pstHeadField->pcFieldName, pstBuf);
        
    ulRet |= http_AppendBufStr(HTTP_HEAD_FIELD_SPLIT_FLAG, pstBuf);

    

    
    ulRet |= http_AppendBufStr(pstHeadField->pcFieldValue, pstBuf);
    
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, pstBuf);
    
    if(BS_OK != ulRet)
    {
        ulRet = BS_ERR;
    }        
    return ulRet;    
}



static BS_STATUS http_BuildVersion(IN HTTP_VERSION_E enVersion, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    ULONG ulRet = BS_OK;
    CHAR *pcStr = NULL;

    
    BS_DBGASSERT(NULL != pstBuf);

    switch(enVersion)
    {
        case HTTP_VERSION_1_1:
        {
            pcStr = HTTP_VERSION_1_1_STR;
            break;
        }   
        case HTTP_VERSION_1_0:
        {
            pcStr = HTTP_VERSION_1_0_STR;
            break;
        }            
        case HTTP_VERSION_0_9:
        {
            pcStr = HTTP_VERSION_0_9_STR;
            break;
        }            
        default:
        {
            ulRet = BS_ERR;
            break;
        }           
    }

    
    if (BS_OK == ulRet)
    {
        ulRet = http_AppendBufStr(pcStr, pstBuf);
    }
    
    return ulRet;
}


static CHAR* http_GetStatusStr(IN HTTP_STATUS_CODE_E enStatusCode)
{
    if(enStatusCode >= HTTP_STATUS_CODE_BUTT)
    {
        return NULL;
    }
    return (CHAR *)g_apcHttpStatueTable[enStatusCode];
}



static BS_STATUS http_BuildReason(IN HTTP_HEAD_S *pstHttpHead, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    ULONG ulRet = BS_OK;
    CHAR *pcGetWord = NULL;
    CHAR szStatusCode[HTTP_STATUS_CODE_LEN+1];

    
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pstBuf);

    
    pcGetWord = http_GetStatusStr(pstHttpHead->enStatusCode);
    if(NULL == pcGetWord)
    {
        return BS_ERR;
    }
    
    if(NULL != pstHttpHead->pcReason)
    {
        
        memcpy(szStatusCode, pcGetWord, (size_t)HTTP_STATUS_CODE_LEN);
        szStatusCode[HTTP_STATUS_CODE_LEN] = '\0';

        ulRet |= http_AppendBufStr(szStatusCode, pstBuf);
        ulRet |= http_AppendBufStr(HTTP_SPACE_FLAG, pstBuf);
        pcGetWord = pstHttpHead->pcReason;        
    }

    ulRet |= http_AppendBufStr(pcGetWord, pstBuf);
        
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, pstBuf);

    if(BS_OK != ulRet)
    {
        ulRet = BS_ERR;
    }    
    return ulRet;
}



static CHAR * http_BuildHttpRequestHead(IN HTTP_HEAD_S *pstHttpHead, OUT ULONG *pulHeadLen)
{
    CHAR *pcHead = NULL;
    DLL_NODE_S *pstNode = NULL;
    HTTP_SMART_BUF_S stBufData;
    ULONG ulRet;

    
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pulHeadLen);


        
    ulRet = http_InitBufData(pstHttpHead, HTTP_MAX_HEAD_LENGTH + 1, &stBufData);    
    if(BS_OK != ulRet)
    {
        return NULL;
    }

    
    ulRet  = http_BuildRequestMethod(pstHttpHead->enMethod, &stBufData);
         
    ulRet |= http_BuildRequestFullURI(&(pstHttpHead->stUriInfo), &stBufData);
    
    ulRet |= http_BuildVersion(pstHttpHead->enHttpVersion, &stBufData);
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, &stBufData);
    
    
    DLL_SCAN(&(pstHttpHead->stHeadFieldList), pstNode)
    {
        ulRet |= http_BuildHeadField((HTTP_HEAD_FIELD_S *)pstNode, &stBufData);
    }

    
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, &stBufData);

    if(BS_OK == ulRet)
    {
        pcHead = http_MoveBufData(&stBufData, pulHeadLen);
    }

    return pcHead; 
}


static CHAR * http_BuildHttpResponseHead(IN HTTP_HEAD_S *pstHttpHead, OUT ULONG *pulHeadLen)
{
    CHAR *pcHead = NULL;
    DLL_NODE_S *pstNode = NULL;
    ULONG ulRet;
    HTTP_SMART_BUF_S stBufData;

    
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pulHeadLen);


        
    ulRet = http_InitBufData(pstHttpHead, HTTP_MAX_HEAD_LENGTH + 1, &stBufData);    
    if(BS_OK != ulRet)
    {
        return NULL;
    }

    
    ulRet |= http_BuildVersion(pstHttpHead->enHttpVersion, &stBufData);
    ulRet |= http_AppendBufStr(HTTP_SPACE_FLAG, &stBufData);

    
    ulRet |= http_BuildReason(pstHttpHead, &stBufData);

    
    DLL_SCAN(&(pstHttpHead->stHeadFieldList), pstNode)
    {
        ulRet |= http_BuildHeadField((HTTP_HEAD_FIELD_S *)pstNode, &stBufData);
    }

    
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, &stBufData);

    if(BS_OK == ulRet)
    {
        pcHead = http_MoveBufData(&stBufData, pulHeadLen);
    }

    return pcHead;
}


CHAR * HTTP_BuildHttpHead(IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_SOURCE_E enHeadType, OUT ULONG *pulHeadLen)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    CHAR *pcBuild = NULL;

        
    if((NULL == hHttpInstance) || (HTTP_RESPONSE < enHeadType) || (NULL == pulHeadLen))
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    if(HTTP_REQUEST == enHeadType )
    {
        
        pcBuild = http_BuildHttpRequestHead(pstHttpHead, pulHeadLen);
    }
    else if(HTTP_RESPONSE == enHeadType )
    {
        
        pcBuild = http_BuildHttpResponseHead(pstHttpHead, pulHeadLen);
    }
    else
    {
        pcBuild = NULL;
    } 

    return pcBuild; 
}


HTTP_TRANSFER_ENCODING_E HTTP_GetTransferEncoding (IN HTTP_HEAD_PARSER hHttpInstance)
{
    CHAR *pcTransEncode = NULL;

    BS_DBGASSERT(NULL != hHttpInstance);

    pcTransEncode = HTTP_GetHF(hHttpInstance, HTTP_HF_TRANSFER_ENCODING);
    if(NULL != pcTransEncode)
    {
        if(0 == strcmp(pcTransEncode, HTTP_TRANSFER_ENCODE_CHUNKED))
        {
            return HTTP_TRANSFER_ENCODING_CHUNK;
        }
    }
    return HTTP_TRANSFER_ENCODING_NOCHUNK;
}

HTTP_CONNECTION_E HTTP_GetConnection (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    CHAR *pcTransEncode = NULL;

    BS_DBGASSERT(NULL != hHttpInstance);

    pcTransEncode = HTTP_GetHF(hHttpInstance, HTTP_HF_CONNECTION);
    if(NULL != pcTransEncode)
    {
        if(0 == stricmp(pcTransEncode, HTTP_CONN_KEEP_ALIVE))
        {
            return HTTP_CONNECTION_KEEPALIVE;            
        }
        else
        {
            return HTTP_CONNECTION_CLOSE;
        }
    }   

    
    if(HTTP_VERSION_1_1 == pstHttpHead->enHttpVersion)
    {
        return HTTP_CONNECTION_KEEPALIVE;
    }
    return HTTP_CONNECTION_CLOSE;
}


BS_STATUS HTTP_SetConnection (IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_CONNECTION_E enConnType)
{
    ULONG ulResult = 0;

    
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }

    switch(enConnType)
    {
        case HTTP_CONNECTION_KEEPALIVE:
        {
            
            ulResult = HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_CONNECTION, HTTP_CONN_KEEP_ALIVE);
            if(BS_OK != ulResult)
            {
                return BS_ERR;
            }
            break;  
        }
        case HTTP_CONNECTION_CLOSE:
        {
            
            ulResult = HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_CONNECTION, HTTP_CONN_CLOSE);
            if(BS_OK != ulResult)
            {
                return BS_ERR;
            }
            break;  
        }
        default:
        {
            return BS_ERR;
        }  
    }

    return BS_OK;
}



HTTP_BODY_TRAN_TYPE_E HTTP_GetBodyTranType (IN HTTP_HEAD_PARSER hHttpInstance )
{
    BS_DBGASSERT(NULL != hHttpInstance);

    if (HTTP_TRANSFER_ENCODING_CHUNK == HTTP_GetTransferEncoding(hHttpInstance))
    {
        return HTTP_BODY_TRAN_TYPE_CHUNKED;
    }

    if (NULL != HTTP_GetHF(hHttpInstance, HTTP_HF_CONTENT_LENGTH))
    {
        return HTTP_BODY_TRAN_TYPE_CONTENT_LENGTH;
    }

    if (HTTP_CONNECTION_CLOSE == HTTP_GetConnection(hHttpInstance))
    {
        return HTTP_BODY_TRAN_TYPE_CLOSED;
    }

    return HTTP_BODY_TRAN_TYPE_NONE;
}




HTTP_BODY_CONTENT_TYPE_E HTTP_GetContentType (IN HTTP_HEAD_PARSER hHttpInstance )
{
    CHAR *pcContentType = NULL;

    
    if(NULL == hHttpInstance)
    {
        return HTTP_BODY_CONTENT_TYPE_MAX;
    }

    
    pcContentType = HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_CONTENT_TYPE);
    if(NULL != pcContentType)
    {
        if(0 == strnicmp(pcContentType, HTTP_MULTIPART_STR, strlen(HTTP_MULTIPART_STR)))
        {
            return HTTP_BODY_CONTENT_TYPE_MULTIPART;
        }
        if (0 == strnicmp(pcContentType, HTTP_CONTENT_TYPE_OCSP, strlen(HTTP_CONTENT_TYPE_OCSP)))
        {
            return HTTP_BODY_CONTENT_TYPE_OCSP;
        }
        if (0 == strnicmp(pcContentType, HTTP_CONTENT_TYPE_JSON, strlen(HTTP_CONTENT_TYPE_JSON)))
        {
            return HTTP_BODY_CONTENT_TYPE_JSON;
        }
    }

    return HTTP_BODY_CONTENT_TYPE_NORMAL;
}


BS_STATUS HTTP_SetChunk (IN HTTP_HEAD_PARSER hHttpInstance)
{
    
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    return HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_TRANSFER_ENCODING, HTTP_TRANSFER_ENCODE_CHUNKED);    
}



static BS_STATUS http_range_DoParse
(
    IN HTTP_RANGE_PART_S *pstRangePart, 
    IN CHAR *pcStart, 
    IN CHAR *pcDash, 
    IN CHAR *pcEnd
)
{                                   
    ULONG ulLowValueLen;            
    ULONG ulHighValueLen;           
    CHAR *pcStartLowValue;          
    CHAR *pcStartHighValue;         
    CHAR *pcLowWithoutSpace;        
    CHAR *pcHighWithoutSpace;       
    CHAR szTmpBuf[HTTP_MAX_UINT64_LEN+1];
    ULONG ulOutLen;
    

    
    BS_DBGASSERT(NULL != pstRangePart);
    BS_DBGASSERT(NULL != pcStart);
    BS_DBGASSERT(NULL != pcDash);
    BS_DBGASSERT(NULL != pcEnd);
    
    
    ulLowValueLen = (ULONG)pcDash - (ULONG)pcStart;
    pcStartLowValue = pcStart;
    
    if (pcDash == pcStart)
    {
        pstRangePart->uiKeyLow = HTTP_INVALID_VALUE;
    }
    else
    {
        pcLowWithoutSpace = HTTP_Strim(pcStartLowValue, ulLowValueLen, HTTP_SP_HT_STRING, &ulOutLen);
        if(NULL == pcLowWithoutSpace)
        {
            return BS_ERR;
        }

        
        if (0 == ulOutLen)
        {
            pstRangePart->uiKeyLow = HTTP_INVALID_VALUE;
        }
        else
        {
            if (TRUE != http_CheckStringIsDigital(pcLowWithoutSpace, ulOutLen))
            {
                return BS_ERR;
            }

            
            memcpy(szTmpBuf, pcLowWithoutSpace, ulOutLen);
            szTmpBuf[ulOutLen] = '\0';
			pstRangePart->uiKeyLow = 0;
			TXT_Atoui(pcLowWithoutSpace, &pstRangePart->uiKeyLow);
        }
    }

    pcStartHighValue = pcDash + 1;
    ulHighValueLen = ((ULONG)pcEnd - (ULONG)pcStartHighValue) + 1;

    
    if (pcDash == pcEnd)
    {
        pstRangePart->uiKeyHigh = HTTP_INVALID_VALUE;
    }
    else
    {
        pcHighWithoutSpace = HTTP_Strim(pcStartHighValue, ulHighValueLen, HTTP_SP_HT_STRING, &ulOutLen);
        if(NULL == pcHighWithoutSpace)
        {
            return BS_ERR;
        }
        if (0 == ulOutLen)
        {
            pstRangePart->uiKeyHigh = HTTP_INVALID_VALUE;
        }
        else
        {
            if (TRUE != http_CheckStringIsDigital(pcHighWithoutSpace, ulOutLen))
            {
                return BS_ERR;
            }

            
            memcpy(szTmpBuf, pcHighWithoutSpace, ulOutLen);
            szTmpBuf[ulOutLen] = '\0';
            pstRangePart->uiKeyHigh = 0;
            TXT_Atoui(pcHighWithoutSpace, &pstRangePart->uiKeyHigh);
        }
    }
    
    return BS_OK;
}


HTTP_RANGE_S * HTTP_GetRange(IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    HTTP_RANGE_S *pstRange;
    HTTP_RANGE_PART_S *pstRangePart;
    CHAR *pcRangeValue;
    CHAR *pcPos,*pcEnd;
    CHAR *pcTempComma, *pcTempDash;
    ULONG ulStrLen;
    ULONG ulRet = BS_OK;
    ULONG ulRangeNum = 1;
    ULONG ulTempLen;
    ULONG ulCount = 0;
    
    
    if (NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S*)hHttpInstance;
    
    if (NULL != pstHttpHead->pstRange)
    {
        return pstHttpHead->pstRange;
    }

    pcRangeValue = HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_RANGE);

    
    if (NULL == pcRangeValue)
    {
        return NULL; 
    }

    
    pcPos = pcRangeValue;
    ulStrLen = strlen(pcRangeValue);
    pcEnd = pcRangeValue + ulStrLen;

    
    if (8 > ulStrLen)
    {
        return NULL;
    }
    
    if (0 != strncmp("bytes", pcRangeValue, (size_t)5))
    {
        return NULL;
    }
    
    pcPos += 5;

    pcPos = HTTP_StrimHead(pcPos, strlen(pcPos), HTTP_SP_HT_STRING);
    if(NULL == pcPos)
    {
        return NULL;
    }

    
    if (HTTP_EQUAL_CHAR != *pcPos)
    {
        return NULL;
    }
    pcPos++;
    
    
    ulRangeNum = http_GetCharsNumInString(pcPos, ',') + 1;

    
    pstRange = (HTTP_RANGE_S*)MEMPOOL_ZAlloc(pstHttpHead->hMemPool, sizeof(HTTP_RANGE_S));
    if (NULL == pstRange)
    {
        return NULL;
    }
    pstRange->ulRangeNum = ulRangeNum;
    pstRangePart = (HTTP_RANGE_PART_S*)MEMPOOL_ZAlloc(pstHttpHead->hMemPool, ulRangeNum * sizeof(HTTP_RANGE_PART_S));
    if (NULL == pstRangePart)
    {
        MEMPOOL_Free(pstHttpHead->hMemPool, pstRange);
        return NULL;
    }

    
    pstRangePart->uiKeyLow  = HTTP_INVALID_VALUE;
    pstRangePart->uiKeyHigh = HTTP_INVALID_VALUE;
    
    pstRange->pstRangePart = pstRangePart;

    
    while((pcPos < pcEnd) && (ulCount < ulRangeNum))
    {
        pcTempComma = strchr(pcPos, ',');
        if (NULL == pcTempComma)
        {
            
            pcTempComma = pcEnd;
        }
        ulTempLen = (ULONG)pcTempComma - (ULONG)pcPos;
        pcTempDash = memchr(pcPos, '-', ulTempLen);
        if (NULL == pcTempDash)
        {
            ulRet = BS_ERR;
            break;
        }
        

        ulRet = http_range_DoParse(&pstRangePart[ulCount++], pcPos, pcTempDash, pcTempComma - 1);
        if (BS_OK != ulRet)
        {
            ulRet = BS_ERR;
            break;
        }
        pcPos += (ulTempLen + 1);
        
    }
    
    if (BS_OK != ulRet)
    {
        MEMPOOL_Free(pstHttpHead->hMemPool, pstRangePart);
        MEMPOOL_Free(pstHttpHead->hMemPool, pstRange);
        return NULL;
    }

    pstHttpHead->pstRange = pstRange;
    return pstRange;
    
}



static BS_STATUS http_BuildPortList
(
    IN USHORT ausPort[HTTP_MAX_COOKIE_PORT_NUM], 
    INOUT CHAR **ppcBuf, 
    INOUT ULONG *pulLeftLen
)
{
    ULONG ulLenTemp;
    ULONG i;
    CHAR szTemp[HTTP_COOKIE_ULTOA_LEN];
    BOOL_T bHasPort = FALSE;
    INT   iLen;
    ULONG ulLeftLen;
    CHAR *pcBuf = NULL;
    
    BS_DBGASSERT( NULL != pulLeftLen );
    BS_DBGASSERT( NULL != ppcBuf );

    ulLeftLen = *pulLeftLen;
    pcBuf = *ppcBuf;

    for( i = 0; i < HTTP_MAX_COOKIE_PORT_NUM; i++ )
    {
        if( 0 != ausPort[i] )
        {
            bHasPort = TRUE;
            break;
        }
    }
    if (FALSE == bHasPort)
    {
        return BS_OK;
    }

    ulLenTemp = (ULONG)strlen(g_apcHttpCookieAV[COOKIE_AV_PORT]);
    
    if (ulLeftLen < (ulLenTemp + 5) ) 
    {
        return BS_ERR;
    }
    
    iLen = scnprintf((CHAR*)pcBuf, ulLeftLen, "; %s=\"", g_apcHttpCookieAV[COOKIE_AV_PORT]);
    BS_DBGASSERT(iLen > 0);
    
    
    pcBuf += iLen;
    ulLeftLen -= (ULONG)(UINT)iLen;
    
    

    for( i = 0; i < HTTP_MAX_COOKIE_PORT_NUM; i++ )
    {
        
        if( 0 == ausPort[i] )
        {
            
            continue;
        }
        
        
        memset(szTemp, 0, (ULONG)HTTP_COOKIE_ULTOA_LEN);
        iLen = scnprintf(szTemp, (ULONG)HTTP_COOKIE_ULTOA_LEN, "%d", ausPort[i]);
        BS_DBGASSERT(iLen > 0);
        ulLenTemp = strlen(szTemp);
        
        
        if (ulLeftLen < (ulLenTemp + 1))
        {
            return BS_ERR;
        }
        
        scnprintf((CHAR*)pcBuf, ulLeftLen, "%s,", szTemp);
        ulLeftLen -= (ulLenTemp + 1);
        pcBuf += (ulLenTemp + 1);
    }
    
    *((pcBuf) - 1) = '"';

    *ppcBuf = pcBuf;
    *pulLeftLen = ulLeftLen;

    return BS_OK;
}


static BS_STATUS http_BuildCookieParam(IN CHAR *pcKey, 
                                     IN CHAR *pcValue, 
                                     IN BOOL_T bIsNeedQuot, 
                                     IN CHAR *pcBeforeSeparator, 
                                     INOUT CHAR **ppcBuf, 
                                     INOUT ULONG *pulLen)
{
    CHAR szTemp[HTTP_COOKIE_ULTOA_LEN];
    CHAR *pcDest;
    ULONG ulKeyLen;
    ULONG ulValueLen;
    ULONG ulSeparatorLen = 0;
    ULONG ulLen;
    ULONG ulTmpLen;

    
    BS_DBGASSERT(NULL != pcKey);
    BS_DBGASSERT(NULL != pcValue);
    BS_DBGASSERT(NULL != ppcBuf);
    BS_DBGASSERT(NULL != pulLen);    

    pcDest = (CHAR*)(*ppcBuf);

    ulKeyLen = strlen(pcKey);
    ulValueLen = strlen(pcValue);
    if (NULL != pcBeforeSeparator)
    {
        ulSeparatorLen = strlen(pcBeforeSeparator);
    }
    ulLen = ulKeyLen + ulValueLen + ulSeparatorLen + 1; 
    if (TRUE == bIsNeedQuot)
    {
        ulLen += 2; 
    }
     
    if (*pulLen > ulLen)
    {
        memset(szTemp, 0, (ULONG)HTTP_COOKIE_ULTOA_LEN);
        if (NULL != pcBeforeSeparator)
        {
            scnprintf(szTemp, (ULONG)HTTP_COOKIE_ULTOA_LEN, "%s", pcBeforeSeparator);
        }
        if (TRUE == bIsNeedQuot)
        {
            TXT_Strlcat(szTemp, "%s=\"%s\"", sizeof(szTemp));
        }
        else
        {
            TXT_Strlcat(szTemp, "%s=%s", sizeof(szTemp));
        }

        ulTmpLen = (ULONG)(UINT)scnprintf(pcDest, *pulLen, szTemp, pcKey, pcValue);
        BS_DBGASSERT(ulTmpLen == ulLen);
        *pulLen -= ulTmpLen;
        *ppcBuf += ulTmpLen;

        return BS_OK;
    }
    
    return BS_ERR;
    
}



static BS_STATUS http_DoBuildServerCookie
(
    IN HTTP_SERVER_COOKIE_S *pstCookie, 
    IN HTTP_COOKIE_TYPE_E enCookieType,
    INOUT CHAR *pcDest
)
{
    CHAR  szTemp[HTTP_COOKIE_ULTOA_LEN];
    ULONG ulLenTemp;
    ULONG ulLeftLen = HTTP_MAX_COOKIE_LEN;
    CHAR *pcCookieAvName;
    
    
    ulLenTemp = strlen(g_apcHttpCookieType[enCookieType]);
    memcpy(pcDest, g_apcHttpCookieType[enCookieType], ulLenTemp);
    pcDest += ulLenTemp;
    ulLeftLen -= ulLenTemp;

    
    memcpy(pcDest, ": ", (ULONG)2);
    pcDest += 2;
    ulLeftLen -= 2;
    

    
    if (BS_OK != http_BuildCookieParam(pstCookie->stCookieKey.pcName, 
                                                pstCookie->pcValue,
                                                FALSE, 
                                                NULL, 
                                                &pcDest, 
                                                &ulLeftLen))
    {
        return BS_ERR;
    }

                    
    if (NULL != pstCookie->pcComment)
    {
        if (BS_OK != http_BuildCookieParam(g_apcHttpCookieAV[COOKIE_AV_COMMENT], 
                                                    pstCookie->pcComment, 
                                                    FALSE, 
                                                    "; ", 
                                                    &pcDest, 
                                                    &ulLeftLen))
        {
            return BS_ERR;
        }
    }

    
    if (NULL != pstCookie->pcCommentUrl)
    {
        if (BS_OK != http_BuildCookieParam(g_apcHttpCookieAV[COOKIE_AV_COMMENTURL],
                                                    pstCookie->pcCommentUrl, 
                                                    TRUE, 
                                                    "; ", 
                                                    &pcDest, 
                                                    &ulLeftLen))
        {
            return BS_ERR;
        }                   
    }

    
    if (TRUE == pstCookie->bDiscard)
    {
        pcCookieAvName = g_apcHttpCookieAV[COOKIE_AV_DISCARD];
        ulLenTemp = strlen(pcCookieAvName);
        
        memcpy(pcDest, "; ", (ULONG)2);
        pcDest += 2;
        ulLeftLen -= 2;
        memcpy(pcDest, pcCookieAvName, ulLenTemp);
        pcDest += ulLenTemp;
        ulLeftLen -= ulLenTemp;
     }

    
    if (NULL != pstCookie->stCookieKey.pcDomain)
    {
        if (BS_OK != http_BuildCookieParam(g_apcHttpCookieAV[COOKIE_AV_DOMAIN],
                                                    pstCookie->stCookieKey.pcDomain, 
                                                    FALSE, 
                                                    "; ", 
                                                    &pcDest, 
                                                    &ulLeftLen))
        {
            return BS_ERR;
        }                   
    }

    
    if (pstCookie->ulMaxAge != 0xFFFFFFFF)
    {
        scnprintf(szTemp, (ULONG)HTTP_COOKIE_ULTOA_LEN, "%d", (UINT)(pstCookie->ulMaxAge));
        if ( BS_OK != http_BuildCookieParam(g_apcHttpCookieAV[COOKIE_AV_MAXAGE],
                                                     szTemp, 
                                                     FALSE, 
                                                     "; ", 
                                                     &pcDest, 
                                                     &ulLeftLen))
        {
            return BS_ERR;        
        } 
    }
                      
    
    if (NULL != pstCookie->stCookieKey.pcPath)
    {
        if (BS_OK != http_BuildCookieParam(g_apcHttpCookieAV[COOKIE_AV_PATH],
                                                    pstCookie->stCookieKey.pcPath, 
                                                    FALSE, 
                                                    "; ", 
                                                    &pcDest, 
                                                    &ulLeftLen))
        {
            return BS_ERR;
        }
    }

    
    if (BS_OK != http_BuildPortList(pstCookie->ausPort, &pcDest, &ulLeftLen))
    {
        return BS_ERR;
    }

    
    if (TRUE == pstCookie->bSecure)
    {
        pcCookieAvName = g_apcHttpCookieAV[COOKIE_AV_SECURE];
        ulLenTemp = strlen(pcCookieAvName); 
        
        memcpy(pcDest, "; ", (ULONG)2);
        pcDest += 2;
        ulLeftLen -= (ULONG)2;
        memcpy(pcDest, pcCookieAvName, ulLenTemp);
        pcDest += ulLenTemp;
        ulLeftLen -= ulLenTemp;
    }

           
    scnprintf(szTemp, (ULONG)HTTP_COOKIE_ULTOA_LEN, "%d", (UINT)(pstCookie->ulVersion));
    
    if (BS_OK != http_BuildCookieParam(g_apcHttpCookieAV[COOKIE_AV_VERSION],
                                               szTemp, 
                                               FALSE, 
                                               "; ", 
                                               &pcDest, 
                                               &ulLeftLen))
    {
        return BS_ERR;
    }          

    
    pcDest[HTTP_MAX_COOKIE_LEN - ulLeftLen] = '\0';

    return BS_OK;

}


CHAR * HTTP_BuildServerCookie 
(
    IN MEMPOOL_HANDLE hMemPool,
    IN HTTP_SERVER_COOKIE_S *pstCookie, 
    IN HTTP_COOKIE_TYPE_E enCookieType,
    OUT ULONG *pulCookieLen
)
{
    CHAR *pcCookieString;

    
    if( (NULL == pstCookie)||(NULL == pulCookieLen) )
    {
        return NULL;
    }

    if( (HTTP_COOKIE_SERVER != enCookieType) && (HTTP_COOKIE_SERVER2 != enCookieType) )
    {
        return NULL;
    }

    
    if( (NULL == pstCookie->stCookieKey.pcName) || (NULL == pstCookie->pcValue) )
    {
        return NULL;
    }

    pcCookieString = (CHAR*)MEMPOOL_Alloc(hMemPool, HTTP_MAX_COOKIE_LEN);
    if (NULL == pcCookieString)
    {
        return NULL;
    }
    pcCookieString[0] = '\0';
    if (BS_OK != http_DoBuildServerCookie(pstCookie, enCookieType, pcCookieString))
    {
        MEMPOOL_Free(hMemPool, pcCookieString);
        return NULL;
    }

    *pulCookieLen = strlen(pcCookieString);
    
    return pcCookieString;
    
}

static UINT g_http_head_chars_valid_map[8]; 

static void http_init_chars_valid_map()
{
    int i;
    for (i=32; i<127; i++) {
        ArrayBit_Set(g_http_head_chars_valid_map, i);
    }
    ArrayBit_Set(g_http_head_chars_valid_map, '\r');
    ArrayBit_Set(g_http_head_chars_valid_map, '\n');
}

BOOL_T HTTP_IsValidHeadChars(unsigned char *buf, int len)
{
    int i;

    for (i=0; i<len; i++) {
        if (0 == ArrayBit_Test(g_http_head_chars_valid_map, buf[i])) {
            
            return FALSE;
        }
    }
    
    return TRUE;
}


BOOL_T HTTP_IsHttpHead(char *buf, int len)
{
    if (len < HTTP_MIN_FIRST_LINE_LEN) {
        return FALSE;
    }

    if (TXT_Strnchr(buf, ' ', HTTP_MIN_FIRST_LINE_LEN) == NULL) {
        return FALSE;
    }

    if (! HTTP_IsValidHeadChars((void*)buf, HTTP_MIN_FIRST_LINE_LEN)) {
        return FALSE;
    }

    return TRUE;
}

CONSTRUCTOR(init) {
    http_init_chars_valid_map();
}

