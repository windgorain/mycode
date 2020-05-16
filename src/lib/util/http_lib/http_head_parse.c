/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-21
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/http_lib.h"

#define HTTP_TIME_REMAIN_MAX_LEN  18
#define HTTP_TIME_ISOC_REMAIN_LEN 14
#define HTTP_COOKIE_ULTOA_LEN     32UL
#define HTTP_STATUS_CODE_LEN      3UL
#define HTTP_COPY_LEN             2UL


#define HTTP_SPACE_FLAG                   " "             /* HTTP协议请求/响应行空格标记 */
#define HTTP_HEAD_FIELD_SPLIT_FLAG        ": "            /* HTTP协议头域分隔标记(加上了一个空格) */

#define HTTP_WHITESPACE                   " \t\r\n"
#define HTTP_URI_HEAD_STR                 "://"
#define HTTP_URI_HEAD_STR_LEN             3UL

#define HTTP_VERSION_STR_LEN              8UL
#define HTTP_VERSION_0_9_STR              "HTTP/0.9"
#define HTTP_VERSION_1_0_STR              "HTTP/1.0"
#define HTTP_VERSION_1_1_STR              "HTTP/1.1"
#define HTTP_VERSION_UNKNOWN_STR          "HTTP/Unknown"

#define HTTP_CONN_KEEP_ALIVE              "Keep-Alive"          /* Connection值域 Keep-Alive */
#define HTTP_CONN_CLOSE                   "close"               /* Connection值域 close */
#define HTTP_TRANSFER_ENCODE_CHUNKED      "chunked"             /* Transfer-Encoding值域 chunked */
#define HTTP_MULTIPART_STR                "multipart"           /* Content-Type值域 multipart */    

#define HTTP_SET_STR(pcDest, pcSrc)  ((pcDest) = (pcSrc))

/* 对于描述时间的字符串，不同的rfc标准，格式也不一样 */
typedef enum tagHTTP_TimeStandard
{
    NORULE = 0,
    RFC822,   /* Tue, 10 Nov 2002 23:50:13   */
    RFC850,   /* Tuesday, 10-Dec-02 23:50:13 */
    ISOC,     /* Tue Dec 10 23:50:13 2002    */
} HTTP_TIME_STANDARD_E;


/* 举例说明: http://ip/1.htm;a=b;c=d?e=f&g=x */

typedef struct tagHTTP_UriInfo
{
    CHAR * pcFullUri;      /* http://ip/1.htm;a=b;c=d?e=f&g=x#xyz */
    CHAR * pcUriPath;      /* http://ip/1.htm */
    CHAR * pcUriQuery;     /* e=f&g=x */
    CHAR * pcUriParam;     /* a=b;c=d */
    CHAR * pcUriFragment;  /* xyz     */
    CHAR * pcUriAbsPath;   /* /1.htm */
    CHAR * pcSimpleAbsPath;/* 增加接口HTTP_GetSimpleAbsPath,先decode解决%之类，然后再得到这个 */
}HTTP_URI_INFO_S;

typedef struct tagHTTP_Head{
    HTTP_VERSION_E enHttpVersion;           /* HTTP的版本号 */
    HTTP_METHOD_E enMethod;                 /* 请求报文头的方法类型 */
    CHAR *pcMethodData;                     /* 报文中的原始Method数据 */
    HTTP_URI_INFO_S stUriInfo;              /* 报文头的URI信息 */
    HTTP_STATUS_CODE_E  enStatusCode;       /* 响应状态码 */
    HTTP_RANGE_S *pstRange;                 /* range指针 */
    CHAR *pcReason;                         /* HTTP应答Reason字段 */
    DLL_HEAD_S  stHeadFieldList;            /* 头域信息列表 */
    MEMPOOL_HANDLE hMemPool;
}HTTP_HEAD_S;

typedef struct tagHTTP_TokData
{
    CHAR *pcData;       /* 数据指针 */
    ULONG ulDataLen;    /* 数据长度 */
}HTTP_TOK_DATA_S;

typedef struct tagHTTPSmartBuf
{
    CHAR   *pcBuf;             /* 内存指针 */
    ULONG  ulBufLen;           /* buf长度 */
    ULONG  ulBufOffset;        /* buf偏移 */
    BOOL_T bErrorFlag;         /* 错误标记，拼装过程出现错误时记录 */
}HTTP_SMART_BUF_S;    

typedef BS_STATUS (*HTTP_SCAN_LINE_PF)(IN CHAR *pcLineStart, IN ULONG ulLineLen, IN VOID *pUserContext);

/* 每个月的天数 */
STATIC UINT  g_auiHttpDay[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

STATIC const CHAR *g_apcHttpParseMethodStrTable[]= 
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

STATIC const CHAR *g_apcHttpStatueTable[] =
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

STATIC UCHAR g_aucHttpStausCodeMap[] = 
{
    /* 0 - 49 */
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

    /* 50 - 99 */
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

    /* 100 - 149 */
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

    /* 150 - 199 */
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

    /* 200 - 249 */
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

    /* 250 - 299 */
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

    /* 300 - 349 */
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

    /* 350 - 399 */
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

    /* 400 - 449 */
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

    /* 450 - 499 */
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

    /* 500 - 505 */
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
    /* Name = Value; */
    "Comment",
    "CommentURL",   
    "Domain",    
    "Path",
    "Port",

    /* name = ulong; */
    "Max-Age",
    "Version",

    /* name; */
    "Discard",
    "Secure"
};

STATIC CHAR *g_pcEncode = "=`~!#$%^&()+{}|:\"<>?[]\\;\',/\t";

STATIC const CHAR *g_apcWkday[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
STATIC const CHAR *g_apcMonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

                                                       
/* 初始化拼接字符串buf结构 */
STATIC BS_STATUS http_InitBufData(IN HTTP_HEAD_S *pstHttpHead, IN ULONG ulBufLen, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    CHAR *pcBuf;

    /* 入参合法性检查 */
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

/*******************************************************************************
  提取拼接buf数据
*******************************************************************************/
STATIC CHAR *http_MoveBufData(INOUT HTTP_SMART_BUF_S *pstBuf, OUT ULONG *pulDataLen)
{
    CHAR  *pcBuf = NULL;

    /* 入参合法性检查 */
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

/*******************************************************************************
 向buf拼接字符串
*******************************************************************************/
STATIC BS_STATUS http_AppendBufStr(IN CHAR *pcStr, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    ULONG ulLen;
    
    /* 入参合法性检查 */    
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


/*******************************************************************************
  得到某个字符在指定字符串中的出现次数
*******************************************************************************/
STATIC ULONG http_GetCharsNumInString(IN CHAR *pcString, IN CHAR cChar)
{
    CHAR *pcPos = NULL;
    CHAR *pcEnd = NULL;
    ULONG ulCount = 0;
    /* 入参合法性检查 */
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

/*******************************************************************************
  检查一个字符串是否为数字          
*******************************************************************************/
STATIC BOOL_T http_CheckStringIsDigital(IN CHAR *pcString, IN ULONG ulLen)
{
    CHAR *pcPos, *pcEnd;

    /* 入参合法性检查 */
    BS_DBGASSERT(pcString != NULL);
    BS_DBGASSERT(ulLen != 0);

    pcPos = pcString;
    pcEnd = pcString + ulLen;

    /* 超过UINT64限度,视为非法情况 */
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


/*******************************************************************************
  创建HTTP头解析器实例
*******************************************************************************/
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

/*******************************************************************************
    销毁HTTP头解析器实例
*******************************************************************************/
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

/*******************************************************************************
  得到HTTP头的长度，包括"\r\n\r\n"
  成功返回长度, 不全返回0
*******************************************************************************/
UINT HTTP_GetHeadLen(IN CHAR *pcHttpData, IN ULONG ulDataLen)
{
    CHAR *pcFound = NULL;
    UINT tmp= 0;
    CHAR *pcTemp = NULL;

    /* 入参检查 */
    if (NULL == pcHttpData) {
        return 0;
    }

    if (ulDataLen <= HTTP_CRLFCRLF_LEN) {
        return 0;
    }

    /* 过滤掉请求头起始部分的的" \t\r\n" */
    pcTemp = HTTP_StrimHead(pcHttpData, ulDataLen, HTTP_WHITESPACE);
    if(NULL == pcTemp) {
        return 0;
    }

    tmp = ulDataLen - ((ULONG)pcTemp - (ULONG)pcHttpData);
    if (tmp <= HTTP_CRLFCRLF_LEN) {
        return 0;
    }
    
    /* 搜索"\r\n" */
    pcFound = (CHAR*)MEM_Find(pcTemp, tmp, HTTP_CRLFCRLF, HTTP_CRLFCRLF_LEN);
    if (NULL == pcFound) {
        return 0;
    }

    tmp= (UINT)(pcFound - pcHttpData);

    return tmp + HTTP_CRLFCRLF_LEN;
}

/*******************************************************************************
  扫描HTTP头域数据，判断是否存在续行
*******************************************************************************/
STATIC BOOL_T http_IsHaveContinueLine(IN CHAR *pcHttpHead, IN ULONG ulHeadLen)
{
    /* 入参合法性检查 */
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
/*******************************************************************************
 扫描一定长度的字符串，跳过数据中指定的字符
*******************************************************************************/
CHAR * HTTP_StrimHead(IN CHAR *pcData, IN ULONG ulDataLen, IN CHAR *pcSkipChars)
{
    CHAR *pcTemp = pcData;
    CHAR *pcEnd = pcData + ulDataLen;

    /* 入参合法性检查 */
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

/*******************************************************************************
 扫描一定长度的字符串，计算除去后面的跳过字符之后的长度
*******************************************************************************/
ULONG HTTP_StrimTail(IN CHAR *pcData, IN ULONG ulDataLen, IN CHAR *pcSkipChars)
{
    CHAR *pcTemp;

    /* 入参合法性检查 */
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

/*******************************************************************************
  扫描一定长度的字符串，跳过数据中指定的字符，并且计算除去后面的跳过字符的长度
*******************************************************************************/
CHAR * HTTP_Strim(IN CHAR *pcData, IN ULONG ulDataLen, IN CHAR *pcSkipChars, OUT ULONG *pulNewLen)
{
    CHAR *pcTemp;
    ULONG ulDataLenTemp;

    /* 入参合法性检查 */
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

/*******************************************************************************
  将所有的头域数据分行扫描，对每行都使用回调函数进行处理
*******************************************************************************/
STATIC BS_STATUS http_ScanLine(IN CHAR *pcData, IN ULONG ulDataLen, IN HTTP_SCAN_LINE_PF pfFunc, IN VOID *pUserContext)
{
    CHAR *pcLineBegin = pcData;
    CHAR *pcLineEnd = NULL;
    CHAR *pcEnd = NULL;
    ULONG ulOffset = 0;
    ULONG ulError = BS_ERR;

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pcData);
    BS_DBGASSERT(0 != ulDataLen);
    BS_DBGASSERT(NULL != pfFunc);
    BS_DBGASSERT(NULL != pUserContext);


    /* 扫描一行，对每行都调用pcFunc */
    pcEnd = pcData + ulDataLen;    
    while (pcLineBegin < pcEnd)
    {
        /* 搜索一行中的CRLF */
        pcLineEnd = (CHAR*)MEM_Find(pcLineBegin, ((ULONG)pcEnd - (ULONG)pcLineBegin), 
                                    HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);

        /* 没有找到，表示到了HTTP头的结尾 */
        if (NULL == pcLineEnd)
        {
            ulOffset = (ULONG)pcEnd - (ULONG)pcLineBegin;
        }
        else
        {
            ulOffset = (ULONG)pcLineEnd - (ULONG)pcLineBegin;
        }
        /* 找到了CRLF，使用回调函数将这一行进行处理 */
        ulError = pfFunc(pcLineBegin, ulOffset, pUserContext);
        if (BS_OK != ulError)
        {
            break;
        }
        /* 更新下一行的起始指针 */
        pcLineBegin += ulOffset + HTTP_LINE_END_FLAG_LEN;
    }
    
    return ulError;
}

/*******************************************************************************
  压缩一行数据，去掉续行中的\t\r\n，拷贝其余数据到pUserContext传出
*******************************************************************************/
STATIC BS_STATUS http_CompressHeadLine(IN CHAR *pcLineStart, IN ULONG ulLineLen, IN VOID *pUserContext)
{
    CHAR *pcHead = NULL;
    ULONG ulHeadLen = 0;


    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pcLineStart);
    BS_DBGASSERT(0 != ulLineLen);
    BS_DBGASSERT(NULL != pUserContext);

    pcHead = pUserContext;
    ulHeadLen = strlen(pcHead);

    /* 非续行时(首字符非TAB和SPACE)，拷贝"\r\n"; 续行或者第一行时，不拷贝"\r\n" */
    if ((!TXT_IS_BLANK(*pcLineStart)) && (0 != ulHeadLen))
    {
        memcpy(pcHead + ulHeadLen, HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);
        ulHeadLen += HTTP_LINE_END_FLAG_LEN;       
    }
    
    memcpy(pcHead + ulHeadLen, pcLineStart, ulLineLen);
    ulHeadLen += ulLineLen;
    
    /* 保证以'\0'结尾 */
    pcHead[ulHeadLen] = '\0';
    
    return BS_OK;
}

/*******************************************************************************
  新生成一个压缩的HTTP头, 新生成的头以'\0'结束.
*******************************************************************************/
STATIC CHAR * http_CompressHead(IN HTTP_HEAD_S *pstHttpHead, IN CHAR *pcHttpHead, IN ULONG ulHeadLen)
{
    CHAR *pcHead = NULL;

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pcHttpHead);  
    BS_DBGASSERT(0 != ulHeadLen);    

    pcHead = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulHeadLen + 1); /* 压缩后的头一定不会大于ulHeadLen + 1. */
    if (NULL == pcHead)
    {
        return NULL;
    }
    pcHead[0] = '\0';
    
    /* http_CompressHeadLine函数内部保证了以'\0'结尾 */
    (VOID) http_ScanLine(pcHttpHead, ulHeadLen, http_CompressHeadLine, pcHead);

    return pcHead;
}

/*******************************************************************************
  对指定长度字符串进行起始空格和结束空格过滤，然后申请一块内存，
  将过滤后的字符串拷贝后返回。
*******************************************************************************/
STATIC CHAR * http_GetWord(IN HTTP_HEAD_S *pstHttpHead, IN CHAR *pcSrc, IN ULONG ulHeadLen)
{
    CHAR *pcWord = NULL;
    CHAR *pcRealBegin = NULL;
    ULONG ulRealLen = 0;

    /* 入参合法性检查,uiHeadLen允许为0 */
    BS_DBGASSERT(NULL != pcSrc);    
    

    /* 滤掉头尾的空格和TAB */
    pcRealBegin = HTTP_Strim(pcSrc, ulHeadLen, HTTP_SP_HT_STRING, &ulRealLen);
    if(NULL == pcRealBegin)
    {
        return NULL;
    }
    
    /* 分配一块内存，拷贝找到的字符串 */
    pcWord = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulRealLen+1);
    if(NULL == pcWord)
    {
        return NULL;
    }
    pcWord[0] = '\0';

    if (ulRealLen > 0)
    {
        memcpy(pcWord, pcRealBegin, ulRealLen);
        pcWord[ulRealLen] = '\0';
    }

    return pcWord;
}

/*******************************************************************************
  解析HTTP头域信息
*******************************************************************************/
STATIC BS_STATUS http_ParseField(IN CHAR *pcLineStart, IN ULONG ulLineLen, IN VOID *pUserContext)
{
    CHAR *pcFieldName = NULL;
    CHAR *pcValue = NULL;
    ULONG ulFieldNameLen;
    ULONG ulFieldValueLen;
    HTTP_HEAD_S *pstHttpHead = pUserContext;
    HTTP_HEAD_FIELD_S *pstHeadField = NULL;
    CHAR *pcSplit;

    BS_DBGASSERT(NULL != pUserContext);
    BS_DBGASSERT(0 != pcLineStart);
    BS_DBGASSERT(0 != ulLineLen);

    pcFieldName = pcLineStart;
    pcSplit = memchr(pcLineStart, HTTP_HEAD_FIELD_SPLIT_CHAR, ulLineLen);
    if(NULL == pcSplit)
    {
        /* 没有":",直接跳过这一行,认为是合法情况。因为要提供给类似portal的解析使用，会出现数据不全的情况 */
        return BS_OK;
    }
    ulFieldNameLen = (ULONG)pcSplit - (ULONG)pcLineStart;

    pcValue = pcSplit + 1;
    ulFieldValueLen = ulLineLen - (ulFieldNameLen + 1);


    pstHeadField = (HTTP_HEAD_FIELD_S *)MEMPOOL_ZAlloc(pstHttpHead->hMemPool, sizeof(HTTP_HEAD_FIELD_S));
    if(NULL == pstHeadField)
    {
        return BS_ERR;
    }
    pcFieldName = http_GetWord(pstHttpHead, pcFieldName, ulFieldNameLen);
    pcValue     = http_GetWord(pstHttpHead, pcValue, ulFieldValueLen);    
    pstHeadField->pcFieldName  = pcFieldName;     
    pstHeadField->pcFieldValue = pcValue;
    if((NULL == pcValue) || (NULL == pcFieldName) || ('\0' == pcFieldName[0]))
    {
        /* 头域是空字符串返回错误 */        
        return BS_ERR;
    }
    else
    {
        DLL_ADD(&(pstHttpHead->stHeadFieldList), (DLL_NODE_S *)pstHeadField);        
        return BS_OK;
    }
 
}
/*******************************************************************************
 解析HTTP头域信息
*******************************************************************************/
STATIC HTTP_METHOD_E http_MethodParse(IN CHAR *pcMethodData)
{
    ULONG i = 0;
    
    /* 入参合法性检查 */
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

/*******************************************************************************
    设置经解码的简单的绝对路径
*******************************************************************************/
STATIC BS_STATUS http_HeadSetSimpleUri(IN HTTP_HEAD_S *pstHttpHead)
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


    /* 入参合法性检查 */
    BS_DBGASSERT(pstHttpHead != NULL);

    pstUriInfo = &pstHttpHead->stUriInfo;

    /* 绝对路径不存在则直接返回 */
    if( NULL == pstUriInfo->pcUriAbsPath )
    {
        pstUriInfo->pcSimpleAbsPath = NULL;
        return BS_OK;
    }

    ulSrcUriLen = strlen(pstUriInfo->pcUriAbsPath);

    /* 进行Uri解码,得到解码后的AbsUriPath字符串 */
    pcAbsUriDecoded = HTTP_UriDecode(pstHttpHead->hMemPool, pstUriInfo->pcUriAbsPath, ulSrcUriLen);
    if(NULL == pcAbsUriDecoded)
    {
        return BS_ERR;
    }
    /* 得到解码后的AbsUriPath字符串长度 */
    ulSrcUriLen = strlen(pcAbsUriDecoded);

    
    /* 申请内存资源 */
    pcSimpleAbsUri = (CHAR *)MEMPOOL_Alloc(pstHttpHead->hMemPool, ulSrcUriLen+1);
    if( NULL == pcSimpleAbsUri)
    {
        return BS_ERR;
    }
    pcSimpleAbsUri[0] = '\0';

    pcWalk = pcAbsUriDecoded;
    pcStart = pcSimpleAbsUri;
    pcOut   = pcSimpleAbsUri;
    pcSlash = pcSimpleAbsUri;

    /* 把输入字符串中的'\'转换为'/' */
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
        /* 发现'/'或者'\0'进入处理 */
        if (('/' == cTemp) || ('\0' == cTemp)) 
        {
            /* 与上次的出现'/'的距离 */
            ulToklen = (ULONG)pcOut - (ULONG)pcSlash;

            /* "/../"或者"/..结束"时，回退到上一级目录 */
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
            /* "//"或者"/./"或者"/.结束"时，替换为"/" */
            if ((1 == ulToklen) || ((('/' << 8) | '.') == usPre))
            {
                pcOut = pcSlash;
                if ('\0' == cTemp)
                {
                    pcOut++;
                }
            }
            /* 记录出现'/'的位置 */
            pcSlash = pcOut;
        }

        /* 输入字符串结束 */
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
     
    /* 更新简单绝对URI */   
    HTTP_SET_STR(pstUriInfo->pcSimpleAbsPath, pcSimpleAbsUri);

    return BS_OK;       
}
/*******************************************************************************
  设置URI中的AbsUri                                           
*******************************************************************************/
STATIC BS_STATUS http_HeadSetAbsUri(IN HTTP_HEAD_S *pstHttpHead)
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
        /* 没有找到"://"，则整个Path就为AbsPath */
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
    /* 找到了"://" */
    ulMatchLen = (ULONG)pcGetChr - (ULONG)pcFullPath;
    ulSrcUriLen -= (ulMatchLen + HTTP_URI_HEAD_STR_LEN);
    pcTempPath = (pcFullPath + ulMatchLen) + HTTP_URI_HEAD_STR_LEN;           

    pcGetChrChr = memchr(pcTempPath, HTTP_BACKSLASH_CHAR, ulSrcUriLen);
    if(NULL != pcGetChrChr)
    {   
        /* 找到了'/' */
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
        pstUriInfo->pcUriAbsPath = NULL;    /* NULL表示没有携带绝对路径 */
    }
    
    return BS_OK;     
}

/*******************************************************************************
  解析请求行URI
*******************************************************************************/
STATIC BS_STATUS http_ParseSubUri (IN HTTP_HEAD_S *pstHttpHead)
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

    /* 搜索'#'，得到fragment */    
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
    
    /* 搜索'?'，得到query */
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

    /* 搜索';'，得到param */
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

    /* 得到UriPath */
    pcTemp = http_GetWord(pstHttpHead, pcFullUri, ulSrc);
    if(NULL == pcTemp)
    {
        return BS_ERR;
    }
    HTTP_SET_STR(pstUriInfo->pcUriPath, pcTemp);  
    
    return BS_OK;
}


/*******************************************************************************
  设置HTTP请求的URI                                             
*******************************************************************************/
STATIC BS_STATUS http_HeadSetUri (IN HTTP_HEAD_S *pstHttpHead)
{
    ULONG ulRet = BS_ERR;

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstHttpHead);     

    if(BS_OK != http_ParseSubUri(pstHttpHead))
    {
        return BS_ERR;
    }
   
    /* 解析得到 AbsPath */
    ulRet = http_HeadSetAbsUri(pstHttpHead);
    if(BS_OK != ulRet)
    {
        return BS_ERR;
    }
    /* 设置经解码的简单的绝对路径 */
    ulRet = http_HeadSetSimpleUri(pstHttpHead);

    return ulRet;
}

/*******************************************************************************
  删除HTTP头域                                                          
*******************************************************************************/
STATIC BS_STATUS http_DelHeadField(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFieldName)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    DLL_NODE_S *pstCurrentNode = NULL;
    HTTP_HEAD_FIELD_S *pstHeadField = NULL;

    /* 入参合法性检查 */
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    /* 遍历链表找到匹配的头域 */
    DLL_SCAN(&(pstHttpHead->stHeadFieldList), pstCurrentNode)
    {
        pstHeadField = (HTTP_HEAD_FIELD_S *)pstCurrentNode;
        if(0 == stricmp(pstHeadField->pcFieldName, pcFieldName))
        {
            DLL_DEL(&(pstHttpHead->stHeadFieldList), pstCurrentNode);
            break;
        }
    }
    
    return BS_OK;
}

/*******************************************************************************
 根据分隔符查找数据                                                         
*******************************************************************************/
STATIC BS_STATUS http_StrTok(IN CHAR *pcDelim, INOUT HTTP_TOK_DATA_S *pstData, OUT HTTP_TOK_DATA_S *pstFound)
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


/*******************************************************************************
  解析HTTP头域
*******************************************************************************/
STATIC BS_STATUS http_ParseHeadFileds
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

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pcHeadFields);
    BS_DBGASSERT(0 != ulHeadFieldsLen);

    /* 判断是否存在续行 */
    bIsContinue = http_IsHaveContinueLine(pcHeadFields, ulHeadFieldsLen);
    if(TRUE == bIsContinue)
    {
        /* 将数据进行压缩，除去续行之间的\r\nSPHT */
        pcRealHeadField = http_CompressHead(pstHttpHead, pcHeadFields, ulHeadFieldsLen);
        if(NULL == pcRealHeadField)
        {
            return BS_ERR;
        }
        ulRealHeadFieldLen = strlen(pcRealHeadField);
    }
    else
    {
        /* 不存在续行 */
        pcRealHeadField = pcHeadFields;
        ulRealHeadFieldLen = ulHeadFieldsLen;
    }
    
    /* 解析头域，将其设置到协议解析器结构体链表中 */
    ulRet = http_ScanLine(pcRealHeadField, ulRealHeadFieldLen, http_ParseField, pstHttpHead);

    return ulRet;
}

/*******************************************************************************
 获取HTTP状态码
*******************************************************************************/
STATIC BS_STATUS http_GetStatusCode(IN HTTP_TOK_DATA_S *pstTok, OUT HTTP_STATUS_CODE_E *penStatusCode)
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

/*******************************************************************************
  解析HTTP请求/响应行中的Version字段
*******************************************************************************/
STATIC HTTP_VERSION_E http_ParseVersion(IN CHAR *pcData, IN ULONG ulDataLen)
{
    CHAR szVersion[HTTP_VERSION_STR_LEN + 1];
    HTTP_VERSION_E enVersion;

    /* 入参合法性检查 */
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


/*******************************************************************************
  解析HTTP请求行
*******************************************************************************/
STATIC BS_STATUS http_ParseRequestFirstLine(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFirstLine, IN ULONG ulFirstLineLen)
{
    CHAR *pcTemp = NULL;
    HTTP_VERSION_E enVer = HTTP_VERSION_BUTT;
    HTTP_HEAD_S *pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    HTTP_TOK_DATA_S stData;
    HTTP_TOK_DATA_S stTok;

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pcFirstLine);
    BS_DBGASSERT(0 != ulFirstLineLen);

    memset(&stData, 0, sizeof(stData));
    memset(&stTok, 0, sizeof(stTok));

    stData.pcData = pcFirstLine;
    stData.ulDataLen = ulFirstLineLen;

    /* 提取Method,必须存在 */
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

    /* 提取URI,必须存在 */
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

    /* 提取HTTP Version. 可能不存在Version */
    if (BS_OK == http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        enVer = http_ParseVersion(stTok.pcData, stTok.ulDataLen);   
    }
    
    (VOID)HTTP_SetVersion (hHttpInstance, enVer); 

    return BS_OK;
}


/*******************************************************************************
 解析HTTP响应行
*******************************************************************************/
STATIC BS_STATUS http_ParseResponseFirstLine
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

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pcFirstLine);
    BS_DBGASSERT(0 != ulFirstLineLen);

    memset(&stData, 0, sizeof(stData));
    memset(&stTok, 0, sizeof(stTok));

    stData.pcData = pcFirstLine;
    stData.ulDataLen = ulFirstLineLen;

    /* 提取Version,必须存在 */
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        return BS_ERR;
    }
    
    enVer = http_ParseVersion(stTok.pcData, stTok.ulDataLen);   

    (VOID)HTTP_SetVersion (hHttpInstance, enVer);

    /* 提取StatueCode,必须存在 */
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        return BS_ERR;
    }
    
    if (BS_OK != http_GetStatusCode(&stTok, &enStatusCode))
    {
        return BS_ERR;
    }
    pstHttpHead->enStatusCode = enStatusCode; 

    /* 提取Reason. 可能不存在Reason ,放入 NULL */
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        HTTP_SET_STR(pstHttpHead->pcReason, NULL);
        return BS_OK;
    }
    
    /* 存在Reason字段 */
    pcTemp = http_GetWord(pstHttpHead, stTok.pcData, stTok.ulDataLen);
    if(NULL == pcTemp)
    {
        return BS_ERR;
    }
    HTTP_SET_STR(pstHttpHead->pcReason, pcTemp);
    
    return BS_OK;
}


/*******************************************************************************
  解析FCGI格式响应行. 
*******************************************************************************/
STATIC BS_STATUS http_ParseFCGI
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

    /* 入参合法性检查 */
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

    /* 如果Status头域不存在，则设置一个值进行如下操作 */
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

    /* StatusValue的格式为: 200 OK */
    memset(&stData, 0, sizeof(stData));
    memset(&stTok, 0, sizeof(stTok));
    stData.pcData = pcStatusValue;
    stData.ulDataLen = strlen(pcStatusValue);

    /* 提取状态码 */
    if (BS_OK != http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        return BS_ERR;
    }

    if (BS_OK != http_GetStatusCode(&stTok, &enStatusCode))
    {
        return BS_ERR;
    }
    pstHttpHead->enStatusCode = enStatusCode;

    /* 提取Reason: 例如 OK */
    if (BS_OK == http_StrTok(HTTP_SP_HT_STRING, &stData, &stTok))
    {
        pcTemp = http_GetWord(pstHttpHead, stTok.pcData, stTok.ulDataLen);
        if(NULL == pcTemp)
        {
            return BS_ERR;
        }
        HTTP_SET_STR(pstHttpHead->pcReason, pcTemp);        
    }

    /* 例如FCGI响应头第一行为Status : 200 OK ,最后生成的头域中不应该再有这一项，因此删除 */
    (VOID) http_DelHeadField(hHttpInstance, HTTP_HEAD_FIELD_STATUS);
    
    return BS_OK;
}

/*******************************************************************************
  解析HTTP第一行，根据头类型参数分别解析"请求行"或"响应行"
*******************************************************************************/
STATIC BS_STATUS http_ParseFirstLine
(
    IN HTTP_HEAD_PARSER hHttpInstance, 
    IN CHAR *pcFirstLine, 
    IN ULONG ulFirstLineLen, 
    IN HTTP_SOURCE_E enSourceType
)
{
    ULONG ulRet = 0;
    HTTP_HEAD_S *pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pcFirstLine);
    BS_DBGASSERT(0 != ulFirstLineLen);
    
    switch(enSourceType)
    {
        case HTTP_REQUEST:  
        {
            /* 解析HTTP请求行 */
            ulRet = http_ParseRequestFirstLine(pstHttpHead, pcFirstLine, ulFirstLineLen);
            break;
        }
        case HTTP_RESPONSE:
        {
            /* 解析HTTP响应行 */
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

/*******************************************************************************
  解析HTTP头
*******************************************************************************/
BS_STATUS HTTP_ParseHead(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcHead, IN ULONG ulHeadLen, IN HTTP_SOURCE_E enSourceType)
{
    HTTP_HEAD_S *pstHttpHead = NULL;    /* 协议解析器头结构体指针 */                    
    ULONG ulUsefulHeadLen = 0;                               
    CHAR *pcRealHead;
    CHAR *pcLineEnd;
    ULONG ulFirstLineLen;
    CHAR *pcHeadFields;
    ULONG ulHeadFieldsLen;

    /* 入参合法性检查 */
    if((NULL == hHttpInstance) || (NULL == pcHead) || (ulHeadLen < HTTP_CRLFCRLF_LEN))
    {
        return BS_ERR;
    }

    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    /* 忽略待解析头前面和后面的空格\t\r\n,保证在头不全的情况下可以正常解析 */
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
        /* 全部是 空格\t\r\n */
        return BS_ERR;
    }

    if (HTTP_PART_HEADER == enSourceType)
    {
        return http_ParseHeadFileds(pstHttpHead, pcHead, ulUsefulHeadLen);
    }
    

    /* 找第一行 */
    pcLineEnd = (CHAR*)MEM_Find(pcRealHead, ulUsefulHeadLen, HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);
    if (NULL == pcLineEnd)
    {
        ulFirstLineLen = ulUsefulHeadLen;
    }
    else
    {
        ulFirstLineLen = (ULONG)pcLineEnd - (ULONG)pcRealHead;
    }
    
    /* 处理第一行的内容 */
    if (BS_OK != http_ParseFirstLine(hHttpInstance, pcRealHead, ulFirstLineLen, enSourceType))
    {
        return BS_ERR;
    }

    /* 判断是否存在头域 */
    if (NULL == pcLineEnd)
    {
        return BS_OK;
    }

    /* 解析头域 */
    pcHeadFields = pcLineEnd + HTTP_LINE_END_FLAG_LEN;
    ulHeadFieldsLen = ulUsefulHeadLen - (ulFirstLineLen + HTTP_LINE_END_FLAG_LEN);
    if (BS_OK != http_ParseHeadFileds(pstHttpHead, pcHeadFields, ulHeadFieldsLen))
    {
        return BS_ERR;
    }
    return BS_OK;
}
/*******************************************************************************
  将字符串时间解析成ULONG                                                          
*******************************************************************************/
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

    /* 入参合法性检查 */
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

    /* 看先找到','还是' ',如果先找到空格则是按ISOC处理 */
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

    /* 
        找到空格
        对Tue, 10 Nov 2002 23:50:13来说，是找到10开始的地方
        对Tue Dec 10 23:50:13 2002来说，是找到10开始的地方
    */
    for (pucPos++; pucPos < pucEnd; pucPos++)
    {
        if (' ' != *pucPos) 
        {
            break;
        }
    }

    /*  上面提到的三种rfc规则，最短的是ISOC，找到两个空格后面的数据长度至少是18 */
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
        /* 得到天 Tue, 10 Nov 2002 23:50:13中的10 */
        uiDay = (((*pucPos - '0') * 10) + (*(pucPos + 1))) - '0';
        /*  
            此时p要么指向Tue, 10 Nov 2002 23:50:13中的 Nov...
            要么指向Tuesday, 10-Dec-02 23:50:13中的-Dec-02 23:50:13
        */
        pucPos += 2; 
        if (' ' == *pucPos) 
        {
            if (HTTP_TIME_REMAIN_MAX_LEN > (pucEnd - pucPos)) 
            {
                return 0;
            }
            /* Tue, 10 Nov 2002 23:50:13情况 */
            enFmt = RFC822;

        } 
        else if ('-' == *pucPos) 
        {
            /* Tuesday, 10-Dec-02 23:50:13情况 */
            enFmt = RFC850;
        } 
        else 
        {
            return 0;
        }

        pucPos++;
    }

    /* 开始处理月份 :Nov 2002 23:50:13或者Dec-02 23:50:13或者Dec 10 23:50:13 2002*/
    switch (*pucPos) 
    {

        case 'J':
        {
            iMonth = (*(pucPos + 1) == 'a' )? 0 : ((*(pucPos + 2) == 'n' )? 5 : 6);   /* JAN JUNE JULY */
            break;
        }

        case 'F':
        {
            iMonth = 1;
            break;
        }

        case 'M':
        {
            iMonth = (*(pucPos + 2) == 'r') ? 2 : 4;  /* March, May */
            break;
        }

        case 'A':
        {
            iMonth = (*(pucPos + 1) == 'p') ? 3 : 7; /* Apirl, August */
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

    /* 前进三位，月份占三个字节 */
    pucPos += 3;

    /* 此时RFC822: 2002 23:50:13
           RFC850: -02 23:50:13
           ISOC: 10 23:50:13 2002 */
    if (((RFC822 == enFmt) && (' ' != *pucPos)) || 
        ((RFC850 == enFmt) && ('-' != *pucPos))) 
    {
        return 0;
    }

    pucPos++;
    /* 此时RFC822:2002 23:50:13
           RFC850:02 23:50:13
           ISOC:10 23:50:13 2002 */
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
                  (*(pucPos + 3) - '0'); /* 得到2002 */
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
        uiYear += (uiYear < 70) ? 2000 : 1900; /* why 70? */
        pucPos += 2;
    }
    else
    {
        /* do nothing */
    }

    if (ISOC == enFmt) 
    {
        /* 当前p指向10 23:50:13 2002 */
        if (' ' == *pucPos) 
        {
            pucPos++;
        }

        if (('0' > *pucPos) || 
            ('9' < *pucPos)) 
        {
            return 0;
        }

        uiDay = *pucPos++ - '0'; /* 得到1 */

        if (' ' != *pucPos) 
        {
            if (('0' > *pucPos) || 
                ('9' < *pucPos)) 
            {
                return 0;
            }

            uiDay = ((uiDay * 10) + *pucPos++) - '0'; /* 得到10 */
        }
        /* 还剩 23:50:13 2002，长度为14 */
        if (HTTP_TIME_ISOC_REMAIN_LEN > (pucEnd - pucPos)) 
        {
            return 0;
        }
    }

    if (' ' != *pucPos++) 
    {
        return 0;
    }

    /* 
        此时RFC822:  23:50:13 
            RFC850:  23:50:13
            ISOC:    23:50:13 2002
            
    */
    if (('0' > *pucPos) || 
        ('9' < *pucPos) || 
        ('0' > *(pucPos + 1)) || 
        ('9' < *(pucPos + 1))) 
    {
        return 0;
    }

    /* 得到小时数 23 */
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
    /* 得到分钟数 50 */
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
    /* 得到秒数 13 */
    uiSec = (((*pucPos - '0') * 10) + *(pucPos + 1)) - '0';

    /* 如果是ISOC, 最后还有四个字节表示年份 */
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

    /* 如果二月份有29天 且不是闰年*/
    if ((29 == uiDay) && 
        (1 == iMonth)) 
    {
        if ((0 < (uiYear & 3)) || 
            ((0 == (uiYear % 100)) && (0 != (uiYear % 400)))) 
        {
            return 0;
        }

    }
    /* g_auiHttpDay[1]为28，二月份只有28和29天两种情况 */
    else if (uiDay > g_auiHttpDay[iMonth]) 
    {
        return 0;
    }
    else
    {
        /* do nothing */
    }

    /*
        这是一个计算日期的高斯公式算法，以下注释出自nginx源码
        shift new year to March 1 and start months from 1 (not 0),
        it is needed for Gauss' formula
        
    */

    if (0 >= --iMonth) 
    {
        /* 
            新的一年从三月开始
            否则一月和二月退回到上一年(OMG..)
        */
        iMonth += 12;
        uiYear -= 1;
    }

    /* 
        高斯公式，标准算法
        出自nginx:
        Gauss' formula for Grigorian days since March 1, 1 BC 
        以下为nginx开源代码，为了避免pclint报警，修改代码格式
    
    ulTime =(
             //days in years including leap years since March 1, 1 BC 

            365 * uiYear + uiYear / 4 - uiYear / 100 + uiYear / 400

             //days before the month 

            + 367 * iMonth / 12 - 30

             //days before the day 

            + uiDay - 1

            
             // 719527 days were between March 1, 1 BC and March 1, 1970,
             // 31 and 28 days were in January and February 1970
             

            - 719527 + 31 + 28) * 86400 + uiHour * 3600 + uiMin * 60 + uiSec;
    */

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

/*******************************************************************************
  将ULONG转换成符合RFC1123规定的时间字符串
*******************************************************************************/
BS_STATUS HTTP_DateToStr(IN time_t stTimeSrc, OUT CHAR szDataStr[HTTP_RFC1123_DATESTR_LEN + 1])
{    
    LONG   lYday;
    ULONG  ulTime, ulSec, ulMin, ulHour, ulMday, ulMon, ulYear, ulWday, ulDays, ulLeap;
    ULONG  ulTemp;
    

    /* the calculation is valid for positive time_t only */

    ulTime = (unsigned int) stTimeSrc;

    ulDays = ulTime / 86400;

    /* Jaunary 1, 1970 was Thursday */

    ulWday = (4 + ulDays) % 7;

    ulTime %= 86400;
    ulHour = ulTime / 3600;
    ulTime %= 3600;
    ulMin = ulTime / 60;
    ulSec = ulTime % 60;

    /*
     * the algorithm based on Gauss' formula,
     * see src/http/ngx_http_parse_time.c
     */

    /* days since March 1, 1 BC */
    ulDays = (ulDays - (31 + 28)) + 719527;

    /*
     * The "days" should be adjusted to 1 only, however, some March 1st's go
     * to previous year, so we adjust them to 2.  This causes also shift of the
     * last Feburary days to next year, but we catch the case when "yday"
     * becomes negative.
     */

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

    /*
     * The empirical formula that maps "yday" to month.
     * There are at least 10 variants, some of them are:
     *     mon = (yday + 31) * 15 / 459
     *     mon = (yday + 31) * 17 / 520
     *     mon = (yday + 31) * 20 / 612
     */

    ulMon = (((ULONG)lYday + 31) * 10) / 306;

    /* the Gauss' formula that evaluates days before the month */

    ulMday = ((ULONG)lYday - (((367 * ulMon) / 12) - 30)) + 1;

    if (lYday >= 306) {

        ulYear++;
        ulMon -= 10;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday -= 306;
         */

    } else {

        ulMon += 2;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday += 31 + 28 + leap;
         */
    }    

    if( ( ulWday >= 7 ) || ( ulMon > 12 ) || ( ulMon < 1 ) )
    {
        return BS_ERR;
    }

    /* 将时间转换成协议要求的字符串格式 */
    (VOID)snprintf( szDataStr, HTTP_RFC1123_DATESTR_LEN + 1, "%3s, %02lu %3s %lu %02lu:%02lu:%02lu GMT",
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

    /* 入参合法性检查 */
    if((NULL == hHttpInstance) || (NULL == pcFieldName))
    {
        BS_DBGASSERT(0);
        return;
    }

    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    /* 遍历链表找到匹配的头域 */
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

/*******************************************************************************
  设置HTTP头域                                                          
*******************************************************************************/
BS_STATUS HTTP_SetHeadField(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFieldName, IN CHAR *pcFieldValue)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    HTTP_HEAD_FIELD_S *pstHeadField = NULL;
    CHAR *pcTempName = NULL;
    CHAR *pcTempValue = NULL;    

    /* 入参合法性检查 */    
    if((NULL == hHttpInstance) || (NULL == pcFieldName) || (NULL == pcFieldValue))
    {
        return BS_ERR;
    }
    
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    /* 分配节点内存 */
    pstHeadField = (HTTP_HEAD_FIELD_S *)MEMPOOL_ZAlloc(pstHttpHead->hMemPool, sizeof(HTTP_HEAD_FIELD_S));
    if(NULL == pstHeadField)
    {
        return BS_ERR;
    }

    /* 生成内存并拷贝 */
    pcTempName  = MEMPOOL_Strdup(pstHttpHead->hMemPool, pcFieldName);;
    pcTempValue = MEMPOOL_Strdup(pstHttpHead->hMemPool, pcFieldValue);
    pstHeadField->pcFieldName  = pcTempName;
    pstHeadField->pcFieldValue = pcTempValue;
    if((NULL == pcTempName)||(NULL == pcTempValue))
    {
        return BS_ERR;
    }

    /* 加入链表 */
    DLL_ADD(&(pstHttpHead->stHeadFieldList), (DLL_NODE_S *)pstHeadField);
    return BS_OK;    
}

VOID HTTP_SetNoCache(IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_PRAGMA, "no-cache");
    HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_CATCH_CONTROL, "no-cache");
}

/* 要求客户端每次确认页面是否有更新 */
VOID HTTP_SetRevalidate(IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_CATCH_CONTROL, "max-age=0");
}

/*******************************************************************************
  获取HTTP头域                                                           
*******************************************************************************/
CHAR * HTTP_GetHeadField(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFieldName)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    DLL_NODE_S *pstCurrentNode = NULL;
    HTTP_HEAD_FIELD_S *pstHeadField = NULL;

    /* 入参合法性检查 */
    if((NULL == hHttpInstance) || (NULL == pcFieldName))
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    /* 遍历链表找到匹配的头域 */
    DLL_SCAN(&(pstHttpHead->stHeadFieldList), pstCurrentNode)
    {
        pstHeadField = (HTTP_HEAD_FIELD_S *)pstCurrentNode;
        if(0 == stricmp(pstHeadField->pcFieldName, pcFieldName))
        {
            return pstHeadField->pcFieldValue;
        }
    }
    return NULL;    
}


/*******************************************************************************
  获取HTTP头域下一个节点
  如果pstHeadFieldNode为NULL则表示获取第一个
*******************************************************************************/
HTTP_HEAD_FIELD_S * HTTP_GetNextHeadField(IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_HEAD_FIELD_S *pstHeadFieldNode)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    HTTP_HEAD_FIELD_S *pstResultNode = NULL;

    /* 入参合法性检查 */
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


/*******************************************************************************
  获取HTTP请求Content-Length域值                                                        
*******************************************************************************/
BS_STATUS HTTP_GetContentLen(IN HTTP_HEAD_PARSER hHttpInstance, OUT UINT64 *puiContentLen)
{
    /* 局部变量定义 */
    CHAR *pcContentLen;
    UINT uiLen = 0;
    
    /* 入参合发性检查 */
    if(NULL == hHttpInstance)
    {
        return BS_NULL_PARA;
    }
    /* 得到头域Value */
    pcContentLen = HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_CONTENT_LENGTH);
    
    if (NULL == pcContentLen)
    {
        return BS_NO_SUCH;
    }

    TXT_Atoui(pcContentLen, &uiLen);
    *puiContentLen = uiLen;

    return BS_OK;
}

/*******************************************************************************
  设置HTTP请求Content-Length域值                                                        
*******************************************************************************/
BS_STATUS HTTP_SetContentLen(IN HTTP_HEAD_PARSER hHttpInstance, IN UINT64 uiValue)
{
    /* 局部变量定义 */
    CHAR szContentLen[HTTP_MAX_UINT64_LEN+1];

    /* 入参合发性检查 */
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    
    /* 设置Content域值 */
    snprintf(szContentLen, sizeof(szContentLen), "%llu", uiValue);
    return HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_CONTENT_LENGTH, szContentLen);
}

/*******************************************************************************
  获取HTTP请求的URI Path                                                        
*******************************************************************************/
CHAR * HTTP_GetUriPath (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;

    /* 入参合发性检查 */
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcUriPath;
}

/*******************************************************************************
  设置HTTP请求的URI
*******************************************************************************/
BS_STATUS HTTP_SetUriPath(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcUri, IN UINT uiUriLen)
{ 
    HTTP_HEAD_S *pstHttpHead = NULL;
    CHAR *pcTemp = NULL;

    /* 入参合法性检查 */
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


/*******************************************************************************
  获取HTTP请求的不包含Host的路径                                                     
*******************************************************************************/
CHAR * HTTP_GetUriAbsPath (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合发性检查 */
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcUriAbsPath;
}

/*******************************************************************************
  获取经过简化计算后的绝对路径                                                    
*******************************************************************************/
CHAR * HTTP_GetSimpleUriAbsPath (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合发性检查 */
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcSimpleAbsPath;
}

/*******************************************************************************
  获取HTTP请求的完整URI，包括Host、Path、Param、Query                                                
*******************************************************************************/
CHAR * HTTP_GetFullUri (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合发性检查 */
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcFullUri;
}

/*******************************************************************************
  获取HTTP请求的Query                                             
*******************************************************************************/
CHAR * HTTP_GetUriQuery (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合发性检查 */
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
    /* 入参合发性检查 */
    if(NULL == hHttpInstance)
    {
        return;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    pstHttpHead->stUriInfo.pcUriQuery = NULL;
}

/*******************************************************************************
  设置HTTP请求的Query , 可以重复set                                            
*******************************************************************************/
BS_STATUS HTTP_SetUriQuery (IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcQuery)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    size_t ulSize = 0;
    CHAR *pcTemp = NULL;
    
    /* 入参合法性检查 */
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

/*******************************************************************************
  获取HTTP请求的Fragment                                             
*******************************************************************************/
CHAR * HTTP_GetUriFragment (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合发性检查 */
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcUriFragment;
}

/*******************************************************************************
  设置HTTP请求的Fragment , 可以重复set                                            
*******************************************************************************/
BS_STATUS HTTP_SetUriFragment(IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcFragment)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    size_t ulSize = 0;
    CHAR *pcTemp = NULL;
    
    /* 入参合法性判断 */
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

/*******************************************************************************
  获取HTTP头的协议版本号                                          
*******************************************************************************/
HTTP_VERSION_E HTTP_GetVersion(IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合法性判断 */
    if(NULL == hHttpInstance)
    {
        return HTTP_VERSION_BUTT;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->enHttpVersion;
}

/*******************************************************************************
  转换HTTP请求的协议版本号到字符串                                          
*******************************************************************************/
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

/*******************************************************************************
  设置协议版本号                                        
*******************************************************************************/
BS_STATUS HTTP_SetVersion (IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_VERSION_E enVer)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合法性判断 */
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    pstHttpHead->enHttpVersion = enVer;
    return BS_OK;
}

/*******************************************************************************
  获取HTTP请求的Method                                      
*******************************************************************************/
HTTP_METHOD_E HTTP_GetMethod (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合法性判断 */
    if(NULL == hHttpInstance)
    {
        return HTTP_METHOD_BUTT;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->enMethod;
}

/*******************************************************************************
  设置HTTP请求的Method                                  
*******************************************************************************/
BS_STATUS  HTTP_SetMethod (IN HTTP_HEAD_PARSER hHttpInstance,  IN HTTP_METHOD_E enMethod)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合法性判断 */
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    pstHttpHead->enMethod = enMethod;
    return BS_OK;
}

/*******************************************************************************
  根据Method获取Method String                                 
*******************************************************************************/
CHAR* HTTP_GetMethodStr(IN HTTP_METHOD_E enMethod )
{
    /* 入参合法性检查 */
    if(enMethod > HTTP_METHOD_CONNECT)
    {
        return NULL;
    }
    return (CHAR *)g_apcHttpParseMethodStrTable[enMethod];
}

/*******************************************************************************
  获取Method原始数据                               
*******************************************************************************/
CHAR* HTTP_GetMethodData (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合法性判断 */
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->pcMethodData;
}

/*******************************************************************************
  获取应答状态码                              
*******************************************************************************/
HTTP_STATUS_CODE_E HTTP_GetStatusCode( IN HTTP_HEAD_PARSER hHttpInstance )
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合法性判断 */
    if(NULL == hHttpInstance)
    {
        return HTTP_STATUS_CODE_BUTT;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->enStatusCode;
}

/*******************************************************************************
  设置应答状态码                              
*******************************************************************************/
BS_STATUS HTTP_SetStatusCode( IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_STATUS_CODE_E enStatusCode)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合法性判断 */
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    pstHttpHead->enStatusCode = enStatusCode;
    return BS_OK;
}

/*******************************************************************************
  获取HTTP应答的Reason字段                            
*******************************************************************************/
CHAR * HTTP_GetReason ( IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合法性判断 */
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->pcReason;
}

/*******************************************************************************
  设置HTTP应答的Reason字段                           
*******************************************************************************/
BS_STATUS HTTP_SetReason (IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcReason)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    size_t ulSize = 0;
    CHAR *pcTemp = NULL;

    /* 入参合法性判断 */
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

/*******************************************************************************
  UriDecode内部封装函数，判断%后的两个字符是否为16进制数据，并转换为字符输出
*******************************************************************************/
STATIC BS_STATUS http_DecodeHex(IN CHAR *pcStrBegin, IN CHAR *pcStrEnd, OUT CHAR *pcValue)
{
    CHAR szTmp[HTTP_COPY_LEN+1] = {0};
    INT iValue = 0;
    
    BS_DBGASSERT(NULL != pcStrBegin);
    BS_DBGASSERT(NULL != pcStrEnd);
    BS_DBGASSERT(NULL != pcValue); 
    
    /* 长度不足返回错误 */
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

    /* 将转换后的ASCII值保存在目标字符中 */
    *pcValue = (CHAR)iValue;
    return BS_OK;
}

/*****************************************************************************
  对收到的URI串进行解码, 参考RFC2616, section 3.2.3
******************************************************************************/
CHAR * HTTP_UriDecode(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcUri, IN ULONG ulUriLen)
{
    /* 局部变量定义 */
    CHAR  *pcDesTmp = NULL;
    CHAR  *pcSrcTmp = NULL;
    CHAR  *pcSrcEnd = NULL;
    CHAR  *pcBuf = NULL;
    ULONG ulErrCode = BS_OK;
    CHAR cValue;

    /* 参数检测 */
    if((NULL == pcUri) || (0 == ulUriLen))
    {
        return NULL;
    }
    
    /* 获取原始URL字符串长度 */
    if ( ulUriLen > HTTP_MAX_HEAD_LENGTH )
    {
        return NULL;
    }

    pcSrcTmp = pcUri;
    pcSrcEnd = pcUri + ulUriLen;
    
    /* 分配内存用来临时保存解码后的URL */
    pcBuf = (CHAR *)MEMPOOL_Alloc(hMemPool, ulUriLen + 1 );
    if( NULL == pcBuf )
    {
        return NULL;
    }
    pcBuf[0] = '\0';

    /* 遍历原始URL,解码 */
    pcDesTmp = pcBuf;
    while( pcSrcTmp < pcSrcEnd )
    {
        cValue = *pcSrcTmp;
        /* 如果有'%'字符 */        
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
        pcBuf = NULL;
    }

    return pcBuf;
}

/*******************************************************************************
  URI编码                          
*******************************************************************************/
CHAR * HTTP_UriEncode(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcUri, IN ULONG ulUriLen)
{
    /* 局部变量定义 */
    CHAR *pcSrcTmp = NULL;
    CHAR *pcSrcEnd = NULL;    
    CHAR *pcDstTmp = NULL;
    CHAR *pcBuf = NULL;
    ULONG ulLen = 0;

    /* 参数合法性判断 */
    if((NULL == pcUri) || (0 == ulUriLen))
    {
        return NULL;
    }

    pcSrcTmp = pcUri;
    pcSrcEnd = pcUri + ulUriLen;

    /* 分配临时内存 */
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
            /* 将这些值转换成%ASCII码的形式 */
            (VOID)snprintf( pcDstTmp, ulLen, "%%" );
            pcDstTmp++;
            ulLen--;

            (VOID)snprintf( pcDstTmp, ulLen, "%02X", *pcSrcTmp );
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

/*******************************************************************************
  Body/Query的解码
*******************************************************************************/
CHAR * HTTP_DataDecode(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcData, IN ULONG ulDataLen)
{
    /* 局部变量定义 */
    CHAR  *pcDesTmp = NULL;
    CHAR  *pcSrcTmp = NULL;
    CHAR  *pcSrcEnd = NULL;    
    CHAR  *pcBuf = NULL;
    ULONG ulErrCode = BS_OK;    
    CHAR  cValue;
    
    /* 参数检测 */
    if((NULL == pcData) || (0 == ulDataLen))
    {
        return NULL;
    }

    pcSrcTmp = pcData;
    pcSrcEnd = pcData + ulDataLen;
    
    /* 分配内存用来临时保存解码后的URL */
    pcBuf = (CHAR *)MEMPOOL_Alloc(hMemPool, ulDataLen + 1 );
    if( NULL == pcBuf )
    {
        return NULL;
    }
    pcBuf[0] = '\0';


    /* 遍历原始URL,解码 */
    pcDesTmp = pcBuf;
    while( pcSrcTmp < pcSrcEnd )
    {
        cValue = *pcSrcTmp;
        
        /* 如果有'%'字符 */        
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

        /* 指向目标字符串的指针向后移动1个字节 */
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

/*******************************************************************************
  Body/Query的编码                       
*******************************************************************************/
CHAR * HTTP_DataEncode(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcData, IN ULONG ulDataLen)
{    
    /* 局部变量定义 */    
    CHAR *pcSrcTmp = NULL;
    CHAR *pcSrcEnd = NULL;
    CHAR *pcDstTmp = NULL;
    CHAR *pcBuf = NULL;
    ULONG ulLen = 0;

    /* 参数合法性判断 */
    if((NULL == pcData) || (0 == ulDataLen))
    {
        return NULL;
    }

    pcSrcTmp = pcData;
    pcSrcEnd = pcData + ulDataLen;
    
    /* 分配临时内存 */
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
            /* 将空格转换成'+' */
            (VOID)snprintf( pcDstTmp, ulLen, "+" );
            pcDstTmp++;
            pcSrcTmp++;
            ulLen--;
            continue;
        }

        if(NULL != strchr(g_pcEncode, *pcSrcTmp))
        {      
            (VOID)snprintf( pcDstTmp, ulLen, "%%" );
            pcDstTmp++;
            ulLen--;
            /* 将匹配到的字符转换成%ASCII码的形式 */
            (VOID)snprintf( pcDstTmp, ulLen, "%02X", *pcSrcTmp );
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

/*******************************************************************************
  获取Param                          
*******************************************************************************/
CHAR * HTTP_GetUriParam (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    /* 入参合法性检查 */
    if(NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    return pstHttpHead->stUriInfo.pcUriParam;
}

/*******************************************************************************
  设置Param                          
*******************************************************************************/
BS_STATUS HTTP_SetUriParam (IN HTTP_HEAD_PARSER hHttpInstance, IN CHAR *pcParam)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    size_t ulSize = 0;
    CHAR *pcTemp = NULL;

    /* 入参合法性检查 */
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

/*******************************************************************************
  构造并拼装http 请求方法
*******************************************************************************/
STATIC BS_STATUS http_BuildRequestMethod(IN HTTP_METHOD_E enMethod, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    CHAR *pcGetWord = NULL;
    ULONG ulRet;

    /* 入参合法性判断 */
    BS_DBGASSERT(NULL != pstBuf);

    /* 获取 Method */
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

/*******************************************************************************
  根据已经设置好的Path、Param、Query来设置FullURI
*******************************************************************************/
STATIC BS_STATUS http_BuildRequestFullURI (IN HTTP_URI_INFO_S *pstURIInfo, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    CHAR *pcUriPathTmp;
    ULONG ulRet;
    
    /* 入参合法性检查 */    
    BS_DBGASSERT (NULL != pstURIInfo);
    BS_DBGASSERT (NULL != pstBuf);

    /* 获取path长度 */
    if(NULL != pstURIInfo->pcUriPath)
    {
        pcUriPathTmp = pstURIInfo->pcUriPath;
    }
    else
    {
        /* 如果当前UriPath为NULL，则默认为根目录'/' */    
        pcUriPathTmp = HTTP_BACKSLASH_STRING;
    }

    /* 按顺序放入UriPath、UriParam、URIQuery */
    ulRet  = http_AppendBufStr(pcUriPathTmp, pstBuf);

    /* 按顺序放入UriParam */
    if(NULL != pstURIInfo->pcUriParam)
    {
        /* 加入';' */
        ulRet |= http_AppendBufStr(HTTP_SEMICOLON_STRING, pstBuf);
        ulRet |= http_AppendBufStr(pstURIInfo->pcUriParam, pstBuf);
    }    

    /* 按顺序放入ulQuery */
    if(NULL != pstURIInfo->pcUriQuery)
    {
        /* 加入'?' */
        ulRet |= http_AppendBufStr(HTTP_HEAD_URI_QUERY_STRING, pstBuf);
        ulRet |= http_AppendBufStr(pstURIInfo->pcUriQuery, pstBuf);
    }

    /* 按顺序放入ulFragment */
    if(NULL != pstURIInfo->pcUriFragment)
    {
        /* 加入'#' */
        ulRet |= http_AppendBufStr(HTTP_HEAD_URI_FRAGMENT_STRING, pstBuf);
        ulRet |= http_AppendBufStr(pstURIInfo->pcUriFragment, pstBuf);
    }

    /* 补充空格作为结束符 */
    ulRet |= http_AppendBufStr(HTTP_SPACE_FLAG, pstBuf);

    if(BS_OK != ulRet)
    {
        ulRet = BS_ERR;
    }        
    return ulRet;     
}

/*******************************************************************************
  构造并拼装http 头域
*******************************************************************************/
STATIC BS_STATUS http_BuildHeadField(IN HTTP_HEAD_FIELD_S *pstHeadField, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    ULONG ulRet;

    /* 入参合法性判断 */
    BS_DBGASSERT(NULL != pstHeadField);
    BS_DBGASSERT(NULL != pstBuf);

    /* 加入头域名称 */
    ulRet  = http_AppendBufStr(pstHeadField->pcFieldName, pstBuf);
    /* 加入冒号 */    
    ulRet |= http_AppendBufStr(HTTP_HEAD_FIELD_SPLIT_FLAG, pstBuf);

    

    /* 加入头域值 */
    ulRet |= http_AppendBufStr(pstHeadField->pcFieldValue, pstBuf);
    /* 加入头域行结束标志"\r\n" */
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, pstBuf);
    
    if(BS_OK != ulRet)
    {
        ulRet = BS_ERR;
    }        
    return ulRet;    
}


/*******************************************************************************
  构造并拼装http version
*******************************************************************************/
STATIC BS_STATUS http_BuildVersion(IN HTTP_VERSION_E enVersion, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    ULONG ulRet = BS_OK;
    CHAR *pcStr = NULL;

    /* 入参合法性判断 */
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

/*******************************************************************************
  获取状态码和reason字符串                        
*******************************************************************************/
STATIC CHAR* http_GetStatusStr(IN HTTP_STATUS_CODE_E enStatusCode)
{
    if(enStatusCode >= HTTP_STATUS_CODE_BUTT)
    {
        return NULL;
    }
    return (CHAR *)g_apcHttpStatueTable[enStatusCode];
}


/*******************************************************************************
  构造并拼装http reason
*******************************************************************************/
STATIC BS_STATUS http_BuildReason(IN HTTP_HEAD_S *pstHttpHead, INOUT HTTP_SMART_BUF_S *pstBuf)
{
    ULONG ulRet = BS_OK;
    CHAR *pcGetWord = NULL;
    CHAR szStatusCode[HTTP_STATUS_CODE_LEN+1];

    /* 入参合法性判断 */
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pstBuf);

    /* 获取应答码和Reason */
    pcGetWord = http_GetStatusStr(pstHttpHead->enStatusCode);
    if(NULL == pcGetWord)
    {
        return BS_ERR;
    }
    
    if(NULL != pstHttpHead->pcReason)
    {
        /* 只取表的前三个字节为应答码，后面使用用户Set的Reason */
        memcpy(szStatusCode, pcGetWord, (size_t)HTTP_STATUS_CODE_LEN);
        szStatusCode[HTTP_STATUS_CODE_LEN] = '\0';

        ulRet |= http_AppendBufStr(szStatusCode, pstBuf);
        ulRet |= http_AppendBufStr(HTTP_SPACE_FLAG, pstBuf);
        pcGetWord = pstHttpHead->pcReason;        
    }

    ulRet |= http_AppendBufStr(pcGetWord, pstBuf);
    /* 加上\r\n */    
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, pstBuf);

    if(BS_OK != ulRet)
    {
        ulRet = BS_ERR;
    }    
    return ulRet;
}


/*******************************************************************************
  构造HTTP请求头                         
*******************************************************************************/
STATIC CHAR * http_BuildHttpRequestHead(IN HTTP_HEAD_S *pstHttpHead, OUT ULONG *pulHeadLen)
{
    CHAR *pcHead = NULL;
    DLL_NODE_S *pstNode = NULL;
    HTTP_SMART_BUF_S stBufData;
    ULONG ulRet;

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pulHeadLen);


    /* 初始化buf */    
    ulRet = http_InitBufData(pstHttpHead, HTTP_MAX_HEAD_LENGTH + 1, &stBufData);    
    if(BS_OK != ulRet)
    {
        return NULL;
    }

    /* 获取 Method */
    ulRet  = http_BuildRequestMethod(pstHttpHead->enMethod, &stBufData);
    /* 获取请求行URI */     
    ulRet |= http_BuildRequestFullURI(&(pstHttpHead->stUriInfo), &stBufData);
    /* 获取请求行HTTP Version */
    ulRet |= http_BuildVersion(pstHttpHead->enHttpVersion, &stBufData);
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, &stBufData);
    
    /* 填充HTTP头域 */
    DLL_SCAN(&(pstHttpHead->stHeadFieldList), pstNode)
    {
        ulRet |= http_BuildHeadField((HTTP_HEAD_FIELD_S *)pstNode, &stBufData);
    }

    /* 填充头结束 */
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, &stBufData);

    if(BS_OK == ulRet)
    {
        pcHead = http_MoveBufData(&stBufData, pulHeadLen);
    }

    return pcHead; 
}

/*******************************************************************************
  构造HTTP响应头                         
*******************************************************************************/
STATIC CHAR * http_BuildHttpResponseHead(IN HTTP_HEAD_S *pstHttpHead, OUT ULONG *pulHeadLen)
{
    CHAR *pcHead = NULL;
    DLL_NODE_S *pstNode = NULL;
    ULONG ulRet;
    HTTP_SMART_BUF_S stBufData;

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstHttpHead);
    BS_DBGASSERT(NULL != pulHeadLen);


    /* 初始化buf */    
    ulRet = http_InitBufData(pstHttpHead, HTTP_MAX_HEAD_LENGTH + 1, &stBufData);    
    if(BS_OK != ulRet)
    {
        return NULL;
    }

    /* 获取HTTP版本号 */
    ulRet |= http_BuildVersion(pstHttpHead->enHttpVersion, &stBufData);
    ulRet |= http_AppendBufStr(HTTP_SPACE_FLAG, &stBufData);

    /* 获取应答码和Reason */
    ulRet |= http_BuildReason(pstHttpHead, &stBufData);

    /* 填充HTTP头域 */
    DLL_SCAN(&(pstHttpHead->stHeadFieldList), pstNode)
    {
        ulRet |= http_BuildHeadField((HTTP_HEAD_FIELD_S *)pstNode, &stBufData);
    }

    /* 填充头结束 */
    ulRet |= http_AppendBufStr(HTTP_LINE_END_FLAG, &stBufData);

    if(BS_OK == ulRet)
    {
        pcHead = http_MoveBufData(&stBufData, pulHeadLen);
    }

    return pcHead;
}

/*******************************************************************************
  构造HTTP请求头或者响应头, 返回值不需要调用者释放
*******************************************************************************/
CHAR * HTTP_BuildHttpHead(IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_SOURCE_E enHeadType, OUT ULONG *pulHeadLen)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    CHAR *pcBuild = NULL;

    /* 入参合法性检查 */    
    if((NULL == hHttpInstance) || (HTTP_RESPONSE < enHeadType) || (NULL == pulHeadLen))
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;

    if(HTTP_REQUEST == enHeadType )
    {
        /* 创建请求头 */
        pcBuild = http_BuildHttpRequestHead(pstHttpHead, pulHeadLen);
    }
    else if(HTTP_RESPONSE == enHeadType )
    {
        /* 创建响应头 */
        pcBuild = http_BuildHttpResponseHead(pstHttpHead, pulHeadLen);
    }
    else
    {
        pcBuild = NULL;
    } 

    return pcBuild; 
}

/*******************************************************************************
  解析传输编码格式                        
*******************************************************************************/
HTTP_TRANSFER_ENCODING_E HTTP_GetTransferEncoding (IN HTTP_HEAD_PARSER hHttpInstance)
{
    CHAR *pcTransEncode = NULL;

    /* 入参合法性检查 */
    if(NULL == hHttpInstance)
    {
        return HTTP_TRANSFER_ENCODING_BUTT;
    }
    pcTransEncode = HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_TRANSFER_ENCODING);
    if(NULL != pcTransEncode)
    {
        if(0 == strcmp(pcTransEncode, HTTP_TRANSFER_ENCODE_CHUNKED))
        {
            return HTTP_TRANSFER_ENCODING_CHUNK;
        }
    }
    return HTTP_TRANSFER_ENCODING_NOCHUNK;
}
/*******************************************************************************
  解析是否长连接                      
*******************************************************************************/
HTTP_CONNECTION_E HTTP_GetConnection (IN HTTP_HEAD_PARSER hHttpInstance)
{
    HTTP_HEAD_S *pstHttpHead = NULL;
    CHAR *pcTransEncode = NULL;

    /* 入参合法性检查 */
    if(NULL == hHttpInstance)
    {
        return HTTP_CONNECTION_BUTT;
    }
    pstHttpHead = (HTTP_HEAD_S *)hHttpInstance;
    pcTransEncode = HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_CONNECTION);
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

    /* HTTP/1.1 默认为 keep-alive */
    if(HTTP_VERSION_1_1 == pstHttpHead->enHttpVersion)
    {
        return HTTP_CONNECTION_KEEPALIVE;
    }
    return HTTP_CONNECTION_CLOSE;
}

/*******************************************************************************
  设置是否长连接                      
*******************************************************************************/
BS_STATUS HTTP_SetConnection (IN HTTP_HEAD_PARSER hHttpInstance, IN HTTP_CONNECTION_E enConnType)
{
    ULONG ulResult = 0;

    /* 入参合法性检查 */
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }

    switch(enConnType)
    {
        case HTTP_CONNECTION_KEEPALIVE:
        {
            /* 设置Connection Keep-Alive */
            ulResult = HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_CONNECTION, HTTP_CONN_KEEP_ALIVE);
            if(BS_OK != ulResult)
            {
                return BS_ERR;
            }
            break;  
        }
        case HTTP_CONNECTION_CLOSE:
        {
            /* 设置Connection close */
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


/*******************************************************************************
  获取报文体的传输类型                         
*******************************************************************************/
HTTP_BODY_TRAN_TYPE_E HTTP_GetBodyTranType (IN HTTP_HEAD_PARSER hHttpInstance )
{
    /* 入参合法性检查 */
    if(NULL == hHttpInstance)
    {
        return HTTP_BODY_TRAN_TYPE_NONE;
    }

    if (HTTP_TRANSFER_ENCODING_CHUNK == HTTP_GetTransferEncoding(hHttpInstance))
    {
        return HTTP_BODY_TRAN_TYPE_CHUNKED;
    }

    if (NULL != HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_CONTENT_LENGTH))
    {
        return HTTP_BODY_TRAN_TYPE_CONTENT_LENGTH;
    }

    if (HTTP_CONNECTION_CLOSE == HTTP_GetConnection(hHttpInstance))
    {
        return HTTP_BODY_TRAN_TYPE_CLOSED;
    }

    return HTTP_BODY_TRAN_TYPE_NONE;
}



/*******************************************************************************
  获取实体正文的媒体类型                         
*******************************************************************************/
HTTP_BODY_CONTENT_TYPE_E HTTP_GetContentType (IN HTTP_HEAD_PARSER hHttpInstance )
{
    CHAR *pcContentType = NULL;

    /* 入参合法性判断 */
    if(NULL == hHttpInstance)
    {
        return HTTP_BODY_CONTENT_TYPE_MAX;
    }

    /* 获取Content-Type信息 */
    pcContentType = HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_CONTENT_TYPE);
    if(NULL != pcContentType)
    {
        if(0 == strnicmp(pcContentType, HTTP_MULTIPART_STR, strlen(HTTP_MULTIPART_STR)))
        {
            return HTTP_BODY_CONTENT_TYPE_MULTIPART;
        }
    }

    return HTTP_BODY_CONTENT_TYPE_NORMAL;
}

/*******************************************************************************
  设置Chunked标记
*******************************************************************************/
BS_STATUS HTTP_SetChunk (IN HTTP_HEAD_PARSER hHttpInstance)
{
    /* 入参合法性检查 */
    if(NULL == hHttpInstance)
    {
        return BS_ERR;
    }
    return HTTP_SetHeadField(hHttpInstance, HTTP_FIELD_TRANSFER_ENCODING, HTTP_TRANSFER_ENCODE_CHUNKED);    
}


/*******************************************************************************
  将一个Range段中的字符串解析为数据存到RangePart结构中                     
*******************************************************************************/
STATIC BS_STATUS http_range_DoParse
(
    IN HTTP_RANGE_PART_S *pstRangePart, 
    IN CHAR *pcStart, 
    IN CHAR *pcDash, 
    IN CHAR *pcEnd
)
{                                   /* 以"..3...-.4....."字符串描述下面变量定义    '.'表示空格 */
    ULONG ulLowValueLen;            /* "..3..."的长度，即6 */
    ULONG ulHighValueLen;           /* ".4....."的长度，即7 */
    CHAR *pcStartLowValue;          /* "..3..."的的头指针 */
    CHAR *pcStartHighValue;         /* ".4....."的头指针 */
    CHAR *pcLowWithoutSpace;        /* "3..." */
    CHAR *pcHighWithoutSpace;       /* "4....." */
    CHAR szTmpBuf[HTTP_MAX_UINT64_LEN+1];
    ULONG ulOutLen;
    

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstRangePart);
    BS_DBGASSERT(NULL != pcStart);
    BS_DBGASSERT(NULL != pcDash);
    BS_DBGASSERT(NULL != pcEnd);
    
    
    ulLowValueLen = (ULONG)pcDash - (ULONG)pcStart;
    pcStartLowValue = pcStart;
    /* 没有lowvalue 比如"-3" */
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

        /* 全是空格，比如"   -3" */
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

            /* 构造放入字符串 */
            memcpy(szTmpBuf, pcLowWithoutSpace, ulOutLen);
            szTmpBuf[ulOutLen] = '\0';
			pstRangePart->uiKeyLow = 0;
			TXT_Atoui(pcLowWithoutSpace, &pstRangePart->uiKeyLow);
        }
    }

    pcStartHighValue = pcDash + 1;
    ulHighValueLen = ((ULONG)pcEnd - (ULONG)pcStartHighValue) + 1;

    /* 没有highvalue, 比如"  3 -" */
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

            /* 构造放入字符串 */
            memcpy(szTmpBuf, pcHighWithoutSpace, ulOutLen);
            szTmpBuf[ulOutLen] = '\0';
            pstRangePart->uiKeyHigh = 0;
            TXT_Atoui(pcHighWithoutSpace, &pstRangePart->uiKeyHigh);
        }
    }
    
    return BS_OK;
}

/*******************************************************************************
  解析Range字段                    
*******************************************************************************/
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
    
    /* 入参检查 */
    if (NULL == hHttpInstance)
    {
        return NULL;
    }
    pstHttpHead = (HTTP_HEAD_S*)hHttpInstance;
    /* 如果已经解析过range，就直接返回 */
    if (NULL != pstHttpHead->pstRange)
    {
        return pstHttpHead->pstRange;
    }

    pcRangeValue = HTTP_GetHeadField(hHttpInstance, HTTP_FIELD_RANGE);

    /* 如果没有Range域，就返回空 */
    if (NULL == pcRangeValue)
    {
        return NULL; 
    }

    /* 
        开始解析range字段 
        bytes=2-3,5-  6,5-,-9
    */
    pcPos = pcRangeValue;
    ulStrLen = strlen(pcRangeValue);
    pcEnd = pcRangeValue + ulStrLen;

    /* 最少长度是"bytes=2-" 占8个字节 */
    if (8 > ulStrLen)
    {
        return NULL;
    }
    /* 判断前5个字节是否为bytes,如果不是则返回空 */
    if (0 != strncmp("bytes", pcRangeValue, (size_t)5))
    {
        return NULL;
    }
    /* 跳过前面的"bytes" */
    pcPos += 5;

    pcPos = HTTP_StrimHead(pcPos, strlen(pcPos), HTTP_SP_HT_STRING);
    if(NULL == pcPos)
    {
        return NULL;
    }

    /* 如果bytes后面的第一个非空格字符不是'=', 则认为错误 */
    if (HTTP_EQUAL_CHAR != *pcPos)
    {
        return NULL;
    }
    pcPos++;
    
    /* 得到有几个range段落，range以逗号分开 */
    ulRangeNum = http_GetCharsNumInString(pcPos, ',') + 1;

    /* 根据range段落的个数，申请内存生成HTTP_RANGE_S结构 */
    pstRange = (HTTP_RANGE_S*)MEMPOOL_ZAlloc(pstHttpHead->hMemPool, sizeof(HTTP_RANGE_S));
    if (NULL == pstRange)
    {
        return NULL;
    }
    pstRange->ulRangeNum = ulRangeNum;
    pstRangePart = (HTTP_RANGE_PART_S*)MEMPOOL_ZAlloc(pstHttpHead->hMemPool, ulRangeNum * sizeof(HTTP_RANGE_PART_S));
    if (NULL == pstRangePart)
    {
        return NULL;
    }

    /* 初始化结构 */
    pstRangePart->uiKeyLow  = HTTP_INVALID_VALUE;
    pstRangePart->uiKeyHigh = HTTP_INVALID_VALUE;
    
    pstRange->pstRangePart = pstRangePart;

    /* 此时，pcPos指向真正的range数据了 */
    while((pcPos < pcEnd) && (ulCount < ulRangeNum))
    {
        pcTempComma = strchr(pcPos, ',');
        if (NULL == pcTempComma)
        {
            /* 最后一个range字段 */
            pcTempComma = pcEnd;
        }
        ulTempLen = (ULONG)pcTempComma - (ULONG)pcPos;
        pcTempDash = memchr(pcPos, '-', ulTempLen);
        if (NULL == pcTempDash)
        {
            ulRet = BS_ERR;
            break;
        }
        /* 找到'-',将'-'左右两边的字符串解析成数据 */

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
        return NULL;
    }

    pstHttpHead->pstRange = pstRange;
    return pstRange;
    
}


/*******************************************************************************
  根据Cookie的port字段生成对应的字符串              
*******************************************************************************/
STATIC BS_STATUS http_BuildPortList
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
    /* 加5是因为加上了一个'; '，一个'='，两个'"'的长度 */
    if (ulLeftLen < (ulLenTemp + 5) ) 
    {
        return BS_ERR;
    }
    
    iLen = snprintf((CHAR*)pcBuf, ulLeftLen, "; %s=\"", g_apcHttpCookieAV[COOKIE_AV_PORT]);
    BS_DBGASSERT(iLen > 0);
    
    /* 偏移指针，计算内存剩余长度 */
    pcBuf += iLen;
    ulLeftLen -= (ULONG)(UINT)iLen;
    
    /* 遍历portlist数组, 如果有port则加入列表中 */

    for( i = 0; i < HTTP_MAX_COOKIE_PORT_NUM; i++ )
    {
        /* 如果某个port 值为0则任务无效，继续查看下一个, 再遍历过程中统计有效port数 */
        if( 0 == ausPort[i] )
        {
            /* 查看下一个 */
            continue;
        }
        
        /* 将整数转化成字符串 */
        memset(szTemp, 0, (ULONG)HTTP_COOKIE_ULTOA_LEN);
        iLen = snprintf(szTemp, (ULONG)HTTP_COOKIE_ULTOA_LEN, "%d", ausPort[i]);
        BS_DBGASSERT(iLen > 0);
        ulLenTemp = strlen(szTemp);
        
        /* 加1是因为','的长度 */
        if (ulLeftLen < (ulLenTemp + 1))
        {
            return BS_ERR;
        }
        /* 将PortNum值放入，并在其后加一个',' */
        snprintf((CHAR*)pcBuf, ulLeftLen, "%s,", szTemp);
        ulLeftLen -= (ulLenTemp + 1);
        pcBuf += (ulLenTemp + 1);
    }
    
    *((pcBuf) - 1) = '"';

    *ppcBuf = pcBuf;
    *pulLeftLen = ulLeftLen;

    return BS_OK;
}

/*******************************************************************************
  生成指定的Param字符串                
*******************************************************************************/
STATIC BS_STATUS http_BuildCookieParam(IN CHAR *pcKey, 
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

    /* 入参合法性检查 */
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
    ulLen = ulKeyLen + ulValueLen + ulSeparatorLen + 1; /* 1表示key和value之间的'=' */
    if (TRUE == bIsNeedQuot)
    {
        ulLen += 2; /* 表示Value要用引号括起来， 增加两个引号的长度 */
    }
    /* 这里缓冲区的剩余长度要大于而不能等于要拷贝的数据长度 因为最后要预留一个空间存放字符串结束字符 */ 
    if (*pulLen > ulLen)
    {
        memset(szTemp, 0, (ULONG)HTTP_COOKIE_ULTOA_LEN);
        if (NULL != pcBeforeSeparator)
        {
            snprintf(szTemp, (ULONG)HTTP_COOKIE_ULTOA_LEN, "%s", pcBeforeSeparator);
        }
        if (TRUE == bIsNeedQuot)
        {
            TXT_Strlcat(szTemp, "%s=\"%s\"", sizeof(szTemp));
        }
        else
        {
            TXT_Strlcat(szTemp, "%s=%s", sizeof(szTemp));
        }

        ulTmpLen = (ULONG)(UINT)snprintf(pcDest, *pulLen, szTemp, pcKey, pcValue);
        BS_DBGASSERT(ulTmpLen == ulLen);
        *pulLen -= ulTmpLen;
        *ppcBuf += ulTmpLen;

        return BS_OK;
    }
    
    return BS_ERR;
    
}


/*******************************************************************************
  组装Cookie字符串                      
*******************************************************************************/
STATIC BS_STATUS http_DoBuildServerCookie
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
    
    /* 填充Server Cookie的名字，只能是Set-Cookie或Set-Cookie2 */
    ulLenTemp = strlen(g_apcHttpCookieType[enCookieType]);
    memcpy(pcDest, g_apcHttpCookieType[enCookieType], ulLenTemp);
    pcDest += ulLenTemp;
    ulLeftLen -= ulLenTemp;

    /* 在Cookie Name后面加上': ', 长度至少要有3 */
    memcpy(pcDest, ": ", (ULONG)2);
    pcDest += 2;
    ulLeftLen -= 2;
    

    /* 组装Name和Value, 必选 */
    if (BS_OK != http_BuildCookieParam(pstCookie->stCookieKey.pcName, 
                                                pstCookie->pcValue,
                                                FALSE, 
                                                NULL, 
                                                &pcDest, 
                                                &ulLeftLen))
    {
        return BS_ERR;
    }

    /* 组装Comment */                
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

    /* 组装CommentUrl */
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

    /* 组装Discard */
    if (TRUE == pstCookie->bDiscard)
    {
        pcCookieAvName = g_apcHttpCookieAV[COOKIE_AV_DISCARD];
        ulLenTemp = strlen(pcCookieAvName);
        /* 加1是因为Discard前面的分号 */
        memcpy(pcDest, "; ", (ULONG)2);
        pcDest += 2;
        ulLeftLen -= 2;
        memcpy(pcDest, pcCookieAvName, ulLenTemp);
        pcDest += ulLenTemp;
        ulLeftLen -= ulLenTemp;
     }

    /* 组装domain */
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

    /* 组装Max-Age:其默认值是0xFFFFFFFF，如果其值是默认值，则认为用户未设置，故不组装Max-Age */
    if (pstCookie->ulMaxAge != 0xFFFFFFFF)
    {
        snprintf(szTemp, (ULONG)HTTP_COOKIE_ULTOA_LEN, "%d", (UINT)(pstCookie->ulMaxAge));
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
                      
    /* 组装path */
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

    /* 组装portlist */
    if (BS_OK != http_BuildPortList(pstCookie->ausPort, &pcDest, &ulLeftLen))
    {
        return BS_ERR;
    }

    /* 组装Secure */
    if (TRUE == pstCookie->bSecure)
    {
        pcCookieAvName = g_apcHttpCookieAV[COOKIE_AV_SECURE];
        ulLenTemp = strlen(pcCookieAvName); 
        /* 先组装分号, 再组装 secure */
        memcpy(pcDest, "; ", (ULONG)2);
        pcDest += 2;
        ulLeftLen -= (ULONG)2;
        memcpy(pcDest, pcCookieAvName, ulLenTemp);
        pcDest += ulLenTemp;
        ulLeftLen -= ulLenTemp;
    }

    /* 组装Version, 必选 */       
    snprintf(szTemp, (ULONG)HTTP_COOKIE_ULTOA_LEN, "%d", (UINT)(pstCookie->ulVersion));
    
    if (BS_OK != http_BuildCookieParam(g_apcHttpCookieAV[COOKIE_AV_VERSION],
                                               szTemp, 
                                               FALSE, 
                                               "; ", 
                                               &pcDest, 
                                               &ulLeftLen))
    {
        return BS_ERR;
    }          

    /* 尾字符清零 */
    pcDest[HTTP_MAX_COOKIE_LEN - ulLeftLen] = '\0';

    return BS_OK;

}

/*******************************************************************************
  创建服务器cookie字段                        
*******************************************************************************/
CHAR * HTTP_BuildServerCookie 
(
    IN MEMPOOL_HANDLE hMemPool,
    IN HTTP_SERVER_COOKIE_S *pstCookie, 
    IN HTTP_COOKIE_TYPE_E enCookieType,
    OUT ULONG *pulCookieLen
)
{
    CHAR *pcCookieString;

    /* 入参合法性检查 */
    if( (NULL == pstCookie)||(NULL == pulCookieLen) )
    {
        return NULL;
    }

    if( (HTTP_COOKIE_SERVER != enCookieType) && (HTTP_COOKIE_SERVER2 != enCookieType) )
    {
        return NULL;
    }

    /* 如果某cookie 结点的name 或者 value 为空，则返回 NULL 跳过该结点 */
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
        return NULL;
    }

    *pulCookieLen = strlen(pcCookieString);
    
    return pcCookieString;
    
}

