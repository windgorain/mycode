/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-6-13
* Description: 
* History:     
******************************************************************************/

#ifndef __FCGI_LIB_H_
#define __FCGI_LIB_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE FCGI_HANDLE;

/* flag */
#define FCGI_NO_KEEP_CONN 0
#define FCGI_KEEP_CONN 1

#define FCGI_FLAG_NOBLOCK 0x1  /* 非阻塞模式 */


/* FCGI通道状态 */
typedef enum
{
    FCGI_STATUS_PARSE_PARAM,        /* 正在解析Param */
    FCGI_STATUS_READ,               /* 正在读取数据 */
    FCGI_STATUS_SET_PARAM,          /* 正在设置Param */
    FCGI_STATUS_WRITE,              /* 正在发送数据 */
    FCGI_STATUS_FLUSH,              /* 发送数据完成，正在Flush */
    FCGI_STATUS_DONE                /* 完成 */
}FCGI_STATUS_E;

typedef struct
{
    DLL_NODE_S stLinkNode;
    CHAR *pcName;
    CHAR *pcValue;
}FCGI_PARAM_S;

FCGI_HANDLE FCGI_ServerCreate(IN UINT uiSsltcpId, IN UINT uiFlag);
FCGI_HANDLE FCGI_ClientCreate(IN UINT uiSsltcpId, IN UINT uiFlag);
VOID FCGI_Destory(IN FCGI_HANDLE hFcgiChannel);
UINT FCGI_GetSsltcpId(IN FCGI_HANDLE hFcgiChannel);
BS_STATUS FCGI_ParseHead(IN FCGI_HANDLE hFcgiChannel);
BOOL_T FCGI_IsParseHeadOk(IN FCGI_HANDLE hFcgiChannel);
BS_STATUS FCGI_SetParam(IN FCGI_HANDLE hFcgiChannel, IN CHAR *pcKey, IN CHAR *pcValue);
BS_STATUS FCGI_SetParamFinish(IN FCGI_HANDLE hFcgiChannel);
CHAR * FCGI_GetRequestParam(IN FCGI_HANDLE hFcgiChannel, IN CHAR *pcParamName);
FCGI_PARAM_S * FCGI_GetNextRequestParam(IN FCGI_HANDLE hFcgiChannel, IN FCGI_PARAM_S *pstParam);
BS_STATUS FCGI_Read(IN FCGI_HANDLE hFcgiChannel, OUT UCHAR *pucData, IN UINT uiDataLen, OUT UINT *puiReadLen);
BOOL_T FCGI_IsReadEOF(IN FCGI_HANDLE hFcgiChannel);
BS_STATUS FCGI_Write(IN FCGI_HANDLE hFcgiChannel, IN UCHAR *pucData, IN UINT uiDataLen, OUT UINT *puiWriteLen);
BS_STATUS FCGI_WriteFinish(IN FCGI_HANDLE hFcgiChannel);
ULONG FCGI_GetDataLenInSendBuf(IN FCGI_HANDLE hFcgiChannel);
BS_STATUS FCGI_Flush(IN FCGI_HANDLE hFcgiChannel);
FCGI_STATUS_E FCGI_GetStatus(IN FCGI_HANDLE hFcgiChannel);
BOOL_T FCGI_IsKeepAlive(IN FCGI_HANDLE hFcgiChannel);



#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__FCGI_LIB_H_*/


