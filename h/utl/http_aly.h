/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-5-19
* Description: 
* History:     
******************************************************************************/

#ifndef __HTTP_ALY_H_
#define __HTTP_ALY_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#include "utl/http_protocol.h"
#include "utl/mbuf_utl.h"
#include "utl/num_utl.h"


#define HTTP_ALY_MAX_URL_LEN 1024
#define HTTP_ALY_MAX_HEAD_LEN 2048
#define HTTP_ALY_MAX_RSP_BASE_HEAD_LEN 512
#define HTTP_ALY_MAX_SERVER_NAME_LEN 128


/* status code */
#define HTTP_ALY_STATUS_CODE_CONTINUE           100  /* Continue */
#define HTTP_ALY_STATUS_CODE_SWITCH_PROTOCOLS   101  /* Switching Protocols */
#define HTTP_ALY_STATUS_CODE_OK                 200  /* OK */
#define HTTP_ALY_STATUS_CODE_CREATED            201  /* Created */
#define HTTP_ALY_STATUS_CODE_ACCEPTED           202  /* Accepted */
#define HTTP_ALY_STATUS_CODE_NON_AUTH           203  /* Non-Authoritative Information */
#define HTTP_ALY_STATUS_CODE_NO_CONTENT         204  /* No Content */
#define HTTP_ALY_STATUS_CODE_RESET_CONTENT      205  /* Reset Content */
#define HTTP_ALY_STATUS_CODE_PARTIAL_CONTENT    206  /* Partial Content */
#define HTTP_ALY_STATUS_CODE_MULTIPLE_CHOICES   300  /* Multiple Choices */
#define HTTP_ALY_STATUS_CODE_MOVED_PMT          301  /* Moved Permanently */
#define HTTP_ALY_STATUS_CODE_FOUND              302  /* Found */
#define HTTP_ALY_STATUS_CODE_SEE_OTHER          303  /* See Other */
#define HTTP_ALY_STATUS_CODE_NOT_MODIFIED       304  /* Not Modified */
#define HTTP_ALY_STATUS_CODE_USE_PROXY          305  /* Use Proxy */
#define HTTP_ALY_STATUS_CODE_TMP_REDIRECT       307  /* Temporary Redirect */
#define HTTP_ALY_STATUS_CODE_BAD_REQUEST        400  /* Bad Request */
#define HTTP_ALY_STATUS_CODE_UNAUTHORIZED       401  /* Unauthorized */
#define HTTP_ALY_STATUS_CODE_PAYMENT_REQUIRED   402  /* Payment Required */
#define HTTP_ALY_STATUS_CODE_FORBIDDEN          403  /* Forbidden */
#define HTTP_ALY_STATUS_CODE_NOT_FOUND          404  /* Not Found */
#define HTTP_ALY_STATUS_CODE_METHOD_NOT_ALLOWED 405  /* Method Not Allowed */
#define HTTP_ALY_STATUS_CODE_NOT_ACCEPTABLE     406  /* Not Acceptable */
#define HTTP_ALY_STATUS_CODE_PXY_AUTH_REQUIRED  407  /* Proxy Authentication Required */
#define HTTP_ALY_STATUS_CODE_REQUEST_TIME_OUT   408  /* Request Time-out */
#define HTTP_ALY_STATUS_CODE_CONFLICT           409  /* Conflict */
#define HTTP_ALY_STATUS_CODE_GONE               410  /* Gone */
#define HTTP_ALY_STATUS_CODE_LEN_REQUIRED       411  /* Length Required */
#define HTTP_ALY_STATUS_CODE_PRECOND_FAILED     412  /* Precondition Failed */
#define HTTP_ALY_STATUS_CODE_ENTIRY_TOO_LARGE   413  /* Request Entity Too Large */
#define HTTP_ALY_STATUS_CODE_URI_TOO_LARGE      414  /* Request-URI Too Large */
#define HTTP_ALY_STATUS_CODE_UNSUPPORT_MEDIA    415  /* Unsupported Media Type */
#define HTTP_ALY_STATUS_CODE_RANGE_NOT_STAISF   416  /* Requested range not satisfiable */
#define HTTP_ALY_STATUS_CODE_EXPECTATION_FAILED 417  /* Expectation Failed */
#define HTTP_ALY_STATUS_CODE_INNER_SERVER_ERR   500  /* Internal Server Error */
#define HTTP_ALY_STATUS_CODE_NOT_IMP            501  /* Not Implemented */
#define HTTP_ALY_STATUS_CODE_BAD_GATEWAY        502  /* Bad Gateway */
#define HTTP_ALY_STATUS_CODE_SERVICE_UNAVAI     503  /* Service Unavailable */
#define HTTP_ALY_STATUS_CODE_GATEWAY_TIME_OUT   504  /* Gateway Time-out */
#define HTTP_ALY_STATUS_CODE_VER_NOT_SUPPORT    505  /* HTTP Version not supported */


/* response mode */
#define HTTP_ALY_RESPONSE_MODE_CLOSED  1
#define HTTP_ALY_RESPONSE_MODE_LENGTH  2
#define HTTP_ALY_RESPONSE_MODE_CHUNKED 3


/* ---typedef--- */

typedef enum
{
    HTTP_ALY_METHOD_GET = 0,
    HTTP_ALY_METHOD_POST,
    HTTP_ALY_METHOD_HEAD,
    HTTP_ALY_METHOD_OPTIONS,
    HTTP_ALY_METHOD_PUT,
    HTTP_ALY_METHOD_DELETE,
    HTTP_ALY_METHOD_TRACE,

    HTTP_ALY_METHOD_UNKNOWN
}HTTP_ALY_METHOD_E;

typedef enum{
    HTTP_ALY_BODY_LEN_KNOWN = 1,
    HTTP_ALY_BODY_LEN_TRUNKED,
    HTTP_ALY_BODY_LEN_CLOSED,
    HTTP_ALY_BODY_LEN_BOUNDRY,
    HTTP_ALY_BODY_LEN_UNKNOWN
}HTTP_ALY_BODY_LEN_TYPE_E;

typedef enum{
    HTTP_ALY_CONTINUE = 0,
    HTTP_ALY_STOP,
    HTTP_ALY_ERR
}HTTP_ALY_READ_RET_E;

typedef HTTP_ALY_READ_RET_E (*PF_HTTP_ALY_READ)(IN UINT ulFd, OUT UCHAR *pucBuf, IN UINT ulBufLen, OUT UINT *pulReadLen);

/* ---funcs--- */
extern HANDLE HTTP_ALY_Create();
extern BS_STATUS HTTP_ALY_Reset(IN HANDLE hHandle);
extern BS_STATUS HTTP_ALY_Delete(IN HANDLE hHandle);
extern BS_STATUS HTTP_ALY_AlyHead(IN HANDLE hHandle);
extern BS_STATUS HTTP_ALY_AddData(IN HANDLE hHandle, IN MBUF_S *pstMbuf);
extern BS_STATUS HTTP_ALY_SetReadFunc (IN HANDLE hHandle, IN PF_HTTP_ALY_READ pfFunc, IN UINT ulFd);
extern BS_STATUS HTTP_ALY_GetHead(IN HANDLE hHandle, OUT MBUF_S **ppstMbuf);
extern MBUF_S * HTTP_ALY_GetBody(IN HANDLE hHandle);
extern BS_STATUS HTTP_ALY_GetData(IN HANDLE hHandle, OUT MBUF_S **ppstMbuf);
extern BS_STATUS HTTP_ALY_TryHead (IN HANDLE hHandle);
extern BS_STATUS HTTP_ALY_ReadHead (IN HANDLE hHandle, OUT MBUF_S **ppMbuf);
BOOL_T HTTP_ALY_IsRecvBodyOK(IN HANDLE hHandle);
/*
return BS_OK; BS_ERR;BS_NOT_COMPLETE
*/
extern BS_STATUS HTTP_ALY_TryBody(IN HANDLE hHandle);
extern BS_STATUS HTTP_ALY_ParseKeyValue(IN HANDLE hHandle);
extern BS_STATUS HTTP_ALY_SetBaseRspHttpHeader(IN HANDLE hHandle, IN UCHAR *pucBuf);

/*
*成功: return BS_OK; BS_NOT_COMPLETE
*/
extern BS_STATUS HTTP_ALY_SendByChunk(IN HANDLE hHandle, IN UCHAR *pucBuf, IN UINT ulLen, IN BOOL_T bWaitForComplete, OUT UINT *pulSendLen);

extern CHAR * HTTP_ALY_GetField(IN HANDLE hHandle, IN CHAR *pcFieldName);
/* 得到Value在Head中的offset, 返回0表示没有找到 */
extern UINT HTTP_ALY_GetFieldValueOffset(IN HANDLE hHandle, IN CHAR *pcFieldName);
/* 返回0表示没有找到 */
extern UINT HTTP_ALY_GetFieldKeyOffset(IN HANDLE hHandle, IN CHAR *pcFieldName);
extern CHAR * HTTP_ALY_GetKeyValue(IN HANDLE hHandle, IN CHAR *pcKey);
extern UINT HTTP_ALY_GetResponseStatusCode(IN HANDLE hHandle);
extern BOOL_T HTTP_ALY_IsRequestKeepAlive(IN HANDLE hHandle);
extern BOOL_T HTTP_ALY_IsResponseKeepAlive(IN HANDLE hHandle);
extern BOOL_T HTTP_ALY_IsProxyConnection(IN HANDLE hHandle);
extern HTTP_PROTOCOL_VER_E HTTP_ALY_GetRequestVer(IN HANDLE hHandle);
extern CHAR * HTTP_ALY_GetProtocol(IN HANDLE hHandle);
extern USHORT HTTP_ALY_GetPort(IN HANDLE hHandle);
extern UINT HTTP_ALY_GetHeadLen(IN HANDLE hHandle);
extern UINT HTTP_ALY_GetBodyLen(IN HANDLE hHandle);
extern CHAR * HTTP_ALY_GetHost(IN HANDLE hHandle);
extern CHAR * HTTP_ALY_GetUrl(IN HANDLE hHandle);
extern CHAR * HTTP_ALY_GetQueryString(IN HANDLE hHandle);
extern UINT HTTP_ALY_GetFileId(IN HANDLE hHandle);
extern CHAR * HTTP_ALY_GetCookie(IN HANDLE hHandle, IN CHAR *pszCookieKey);
extern CHAR * HTTP_ALY_GetReferer(IN HANDLE hHandle);
extern HTTP_ALY_METHOD_E HTTP_ALY_GetMethod(IN HANDLE hHandle);
extern CHAR * HTTP_ALY_GetMethodString(IN HANDLE hHandle);
extern HTTP_ALY_METHOD_E HTTP_ALY_GetMethodByString(IN CHAR *pcMethod);
extern UINT HTTP_ALY_GetAvailableDataSize(IN HANDLE hHandle);
extern HTTP_ALY_BODY_LEN_TYPE_E HTTP_ALY_GetBodyLenType(IN HANDLE hHandle);
extern VOID HTTP_ALY_SetRemoveChunkFlag(IN HANDLE hHandle);

#if 1/* 重新构造接收到的HTTP头类接口 */
extern BS_STATUS HTTP_ALY_AddRequestHeadField(IN HANDLE hHandle, IN CHAR *pszFieldName, IN CHAR *pszFieldValue);
extern MBUF_S * HTTP_ALY_BuildRequestHead(IN HANDLE hHandle);
#endif

#if 1/* 应答Key Value 设置类接口 */
extern BS_STATUS HTTP_ALY_SetResponseHeadField(IN HANDLE hHttpHandle, IN CHAR *pszKey, IN CHAR *pszValue);
extern BS_STATUS HTTP_ALY_SetResponseHeadFieldByBuf(IN HANDLE hHttpHandle, IN CHAR *pszString);
extern CHAR * HTTP_ALY_GetResponseKeyValue(IN HANDLE hHttpHandle, IN CHAR *pszKey);
extern BS_STATUS HTTP_ALY_SetResponseStatusCode(IN HANDLE hHttpHandle, IN UINT ulStatusCode, IN CHAR *pszDesc);
extern BS_STATUS HTTP_ALY_SetResponseKeepAlive(IN HANDLE hHandle, IN BOOL_T bIsKeepAlive);
extern BS_STATUS HTTP_ALY_SetResponseContentLen(IN HANDLE hHandle, IN ULONG ulContentLen);
extern BS_STATUS HTTP_ALY_SetCookie(IN HANDLE hHttpHandle, IN CHAR *pszCookieKey, IN CHAR *pszCookieValue, IN CHAR *pszPath);
extern BS_STATUS HTTP_ALY_SetNotModify(IN HANDLE hHttpHandle);
extern BS_STATUS HTTP_ALY_SetNotFound(IN HANDLE hHttpHandle);
extern BS_STATUS HTTP_ALY_SetMethodNotSupport(IN HANDLE hHttpHandle);
extern BS_STATUS HTTP_ALY_SetRedirectTo(IN HANDLE hHttpHandle, IN CHAR *pszRedirectTo);
extern BS_STATUS HTTP_ALY_SetNoCache(IN HANDLE hHttpHandle);
extern BS_STATUS HTTP_ALY_NotBuildHeadField(IN HANDLE hHttpHandle, IN CHAR *pszFieldName);
extern BS_STATUS HTTP_ALY_BuildHeadField(IN HANDLE hHttpHandle, IN CHAR *pszFieldName);
#endif

/* 得到应答模式 */
UCHAR HTTP_ALY_GetResponseMode(IN HANDLE hHttpHandle);
/* pucData可以为NULL，这时uiDataLen必须为0. 这种情况下只发送缓冲区中的数据 */
BS_STATUS HTTP_ALY_Send(IN HANDLE hHttpHandle, IN UCHAR *pucData, IN UINT uiDataLen, OUT UINT *pulSendLen);
/* 数据发送完成.但是这种情况下缓冲区中可能还有数据 */
BS_STATUS HTTP_ALY_Finish(IN HANDLE hHttpHandle);
BS_STATUS HTTP_ALY_BuildResponseHead(IN HANDLE hHttpHandle);
extern UINT HTTP_ALY_GetSendDataSize(IN HANDLE hHttpHandle);
BS_STATUS HTTP_ALY_Flush(IN HANDLE hHttpHandle);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__HTTP_ALY_H_*/


