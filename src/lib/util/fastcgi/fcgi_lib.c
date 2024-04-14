/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-9-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/fcgi_lib.h"
#include "utl/ssltcp_utl.h"
#include "utl/vbuf_utl.h"

#define FCGI_MAX_SEGMENT_SIZE (64 * 1024 - 1)   /* 最大的分段长度为65535 */

typedef enum
{
    FCGI_BEGIN_REQUEST = 1,
    FCGI_ABORT_REQUEST,
    FCGI_END_REQUEST,
    FCGI_PARAMS,
    FCGI_STDIN,
    FCGI_STDOUT,
    FCGI_STDERR,
    FCGI_DATA,
    FCGI_GET_VALUES,
    FCGI_GET_VALUES_RESULT,

    FCGI_UNKNOWN_TYPE
}FCGI_TYPE_E;

typedef enum
{
    FCGI_RECV_SEG_HEADER = 0,   /* 正在接收分段头 */
    FCGI_RECV_SEG_DATA,         /* 正在接收数据 */
    FCGI_RECV_TYPE_END,         /* 接收到了数据长度为0表示某参数结束的报文. 
                                   只有Param.stdin,stdout,stderr会走到这里 */
    FCGI_RECV_END               /* 收到了End Request */
}_FCGI_RECV_SEG_TYPE_E;

/*
 * Values for role component of FCGI_BeginRequestBody
 */
#define FCGI_RESPONDER  1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER     3

typedef struct {
    UCHAR ucVersion;    /* 01 */
    UCHAR ucType;       
    USHORT usRequestId;
    USHORT usContentLen;
    UCHAR ucPaddingLen;
    UCHAR ucReserved;
} FCGI_HEADER_S;

typedef struct {
    USHORT usRole;
    UCHAR ucFlags;    /* FCGI_KEEP_CONN表示保持长连接. 0表示服务完成后关闭连接 */
    UCHAR aucReserved[5];
} FCGI_BEGIN_REQUEST_BODY_S;

typedef struct {
    USHORT usRole;
    UCHAR ucFlags;
    UCHAR aucReserved[5];
} FCGI_END_REQUEST_BODY_S;

typedef struct {
    FCGI_HEADER_S stHeader;
    FCGI_BEGIN_REQUEST_BODY_S stBody;
} FCGI_BEGIN_REQUEST_RECORD_S;

typedef struct {
    FCGI_HEADER_S stHeader;
    FCGI_END_REQUEST_BODY_S stBody;
} FCGI_END_REQUEST_RECORD_S;

typedef struct
{
    UCHAR aucSegData[32];        /* 用于接收分段头/BeginRequest等协议数据的缓冲区 */
    UINT  uiDataLen;                 /* 缓冲区中的数据长度 */
}_FCGI_SEG_DATA_S;

typedef struct
{
    BOOL_T bIsServer;
    USHORT usRequestId;
    UINT  uiFlag;
    FCGI_STATUS_E eStatus;  /* FCGI状态 */
    _FCGI_RECV_SEG_TYPE_E eRecvSegType;/* 当处于正在接收数据状态时,具体的数据类型由eDataType指定 */
    FCGI_TYPE_E eDataType;  /* 正在接收的数据类型 */
    UINT uiSslTcpId;
    DLL_HEAD_S stRequestParams;    /* FCGI_PARAM_S */
    DLL_HEAD_S stResponseParams;   /* FCGI_PARAM_S */
    VBUF_S stVbuf;
    UINT uiRemainDataLen;         /* 剩余的数据 */
    UCHAR ucReaminPaddingLen;      /* 剩余的Padding */
    UCHAR ucFlag;                  /* 是否保持长连接 */

    _FCGI_SEG_DATA_S stSegHeadData;
}_FCGI_CTRL_S;




/* 创建FCGI通道, 要求SsltcpId的模式和eMode必须匹配 */
static _FCGI_CTRL_S * fcgi_Create(IN UINT uiSsltcpId, IN UINT uiFlag, IN BOOL_T bIsServer)
{
    _FCGI_CTRL_S *pstFcgi;

    pstFcgi = MEM_ZMalloc(sizeof(_FCGI_CTRL_S));
    if (NULL == pstFcgi)
    {
        return NULL;
    }

    VBUF_Init(&pstFcgi->stVbuf);

    DLL_INIT(&(pstFcgi->stRequestParams));
    DLL_INIT(&(pstFcgi->stResponseParams));
    pstFcgi->uiFlag = uiFlag;
    pstFcgi->bIsServer = bIsServer;
    pstFcgi->uiSslTcpId = uiSsltcpId;
    pstFcgi->usRequestId = 1;
    pstFcgi->eRecvSegType = FCGI_RECV_SEG_HEADER;

    return pstFcgi;
}

/* Description  : 释放一个Param Node */
static VOID fcgi_FreeParamNode(IN FCGI_PARAM_S *pstParamNode)
{
    if (NULL != pstParamNode->pcName)
    {
        MEM_Free(pstParamNode->pcName);
    }
    if (NULL != pstParamNode->pcValue)
    {
        MEM_Free(pstParamNode->pcValue);
    }
    MEM_Free(pstParamNode);
}

/* 设置头参数 */
static BS_STATUS fcgi_SetParam
(
    IN DLL_HEAD_S *pstListHead,
    IN CHAR *pcParamName,
    IN ULONG ulNameLen,
    IN CHAR *pcParamValue,
    IN ULONG ulValueLen
)
{
    FCGI_PARAM_S *pstParamNode;
    CHAR *pcValue = pcParamValue;

    BS_DBGASSERT(NULL != pcParamName);
    BS_DBGASSERT(ulNameLen > 0);

    pstParamNode = MEM_ZMalloc(sizeof(FCGI_PARAM_S));
    if (NULL == pstParamNode)
    {
        return BS_NO_MEMORY;
    }

    pstParamNode->pcName = MEM_Malloc(ulNameLen + 1);
    if (NULL == pstParamNode->pcName)
    {
        MEM_Free(pstParamNode);
        return BS_NO_MEMORY;
    }
    memcpy(pstParamNode->pcName, pcParamName, ulNameLen);
    pstParamNode->pcName[ulNameLen] = '\0';

    pstParamNode->pcValue = MEM_Malloc(ulValueLen + 1);
    if (NULL == pstParamNode->pcValue)
    {
        MEM_Free(pstParamNode->pcName);
        MEM_Free(pstParamNode);
        return BS_NO_MEMORY;
    }
    if (ulValueLen > 0)
    {
        memcpy(pstParamNode->pcValue, pcValue, ulValueLen);
    }
    pstParamNode->pcValue[ulValueLen] = '\0';

    DLL_ADD(pstListHead, pstParamNode);

    return BS_OK;    
}

static BS_STATUS fcgi_FillParamLen(IN VBUF_S *pstVbuf, IN ULONG ulLen)
{
    UCHAR aucBuf[4];

    if (ulLen > 255)
    {
        aucBuf[0] = (UCHAR)(0x80 | ((ulLen >> 24) & 0xff));
        aucBuf[1] = (UCHAR)((ulLen >> 16) & 0xff);
        aucBuf[2] = (UCHAR)((ulLen >> 8) & 0xff);
        aucBuf[3] = (UCHAR)(ulLen & 0xff);

        return VBUF_CatBuf(pstVbuf, aucBuf, 4);
    }

    aucBuf[0] = (UCHAR)(ulLen & 0xff);

    return VBUF_CatBuf(pstVbuf, aucBuf, 1);
}

static VOID fcgi_MakeHeader
(
    IN FCGI_HEADER_S *pstHeader,
    IN UCHAR ucType,
    IN USHORT usRequestId,
    IN USHORT usContentLen
)
{
    Mem_Zero(pstHeader, sizeof(FCGI_HEADER_S));
    pstHeader->ucVersion = 1;
    pstHeader->ucType = ucType;
    pstHeader->usRequestId = htons(usRequestId);
    pstHeader->usContentLen = htons(usContentLen);
    pstHeader->ucPaddingLen = NUM_UP_ALIGN(usContentLen, 8) - usContentLen;
}

static VOID fcgi_MakeBeginRequestBody(IN FCGI_BEGIN_REQUEST_BODY_S *pstBody, IN USHORT usRole, IN UCHAR ucFlag)
{
    Mem_Zero(pstBody, sizeof(FCGI_BEGIN_REQUEST_BODY_S));
    pstBody->ucFlags = ucFlag;
    pstBody->usRole = htons(usRole);
}

static VOID fcgi_MakeEndRequestBody(IN FCGI_END_REQUEST_BODY_S *pstBody)
{
    Mem_Zero(pstBody, sizeof(FCGI_END_REQUEST_BODY_S));
}



/* 将Param构建成数据 */
static BS_STATUS fcgi_BuildRequestParams(IN _FCGI_CTRL_S *pstFcgi)
{
    FCGI_PARAM_S *pstNode;
    ULONG ulNameLen;
    ULONG ulValueLen;
    FCGI_HEADER_S stHeader;
    ULONG ulDataLen;
    ULONG uiParamsLen;
    HANDLE hVbuf = &pstFcgi->stVbuf;
    FCGI_BEGIN_REQUEST_RECORD_S *pstRecod;
    FCGI_HEADER_S *pstHeader;
    UCHAR *pucPadding = (UCHAR*)"\0\0\0\0\0\0\0\0";
    DLL_HEAD_S *pstParamList = &pstFcgi->stRequestParams;

    /* 构造Param头 */
    fcgi_MakeHeader(&stHeader, FCGI_PARAMS, 1, 0);
    if (BS_OK != VBUF_CatBuf(hVbuf, (UCHAR*)&stHeader, sizeof(FCGI_HEADER_S)))
    {
        return BS_ERR;
    }

    ulDataLen = VBUF_GetDataLength(hVbuf);

    /* 构造Param */
    DLL_SCAN(pstParamList, pstNode)
    {
        ulNameLen = strlen(pstNode->pcName);
        ulValueLen = strlen(pstNode->pcValue);
        if ((BS_OK != fcgi_FillParamLen(hVbuf, ulNameLen)) ||
            (BS_OK != fcgi_FillParamLen(hVbuf, ulValueLen)) ||
            (BS_OK != VBUF_CatBuf(hVbuf, pstNode->pcName, ulNameLen)) ||
            (BS_OK != VBUF_CatBuf(hVbuf, pstNode->pcValue, ulValueLen)))
        {
            return BS_ERR;
        }
    }

    uiParamsLen = VBUF_GetDataLength(hVbuf) - ulDataLen;     /* 得到Param段中携带的载荷长度 */

    /* 暂不支持Params大于64K的情况 */
    if (uiParamsLen > FCGI_MAX_SEGMENT_SIZE)
    {
        return BS_ERR;
    }

    /* 将Param头中的长度字段填上 */
    pstRecod = VBUF_GetData(hVbuf);
    pstHeader = (FCGI_HEADER_S*)(pstRecod + 1);
    fcgi_MakeHeader(pstHeader, FCGI_PARAMS, pstFcgi->usRequestId, (USHORT)uiParamsLen);

    /* 填充Padding */
    if (BS_OK != VBUF_CatBuf(hVbuf, pucPadding, pstHeader->ucPaddingLen))
    {
        return BS_ERR;
    }

    /* 构造PARAMS结束标志 */
    fcgi_MakeHeader(&stHeader, FCGI_PARAMS, pstFcgi->usRequestId, 0);
    if (BS_OK != VBUF_CatBuf(hVbuf, (UCHAR*)&stHeader, sizeof(FCGI_HEADER_S)))
    {
        return BS_ERR;
    }

    return BS_OK;
}


/* 构造请求Header */
static BS_STATUS fcgi_BuildRequestHeader(IN _FCGI_CTRL_S *pstFcgi)
{
    FCGI_BEGIN_REQUEST_RECORD_S stBeginRecord;

    fcgi_MakeHeader(&stBeginRecord.stHeader, FCGI_BEGIN_REQUEST,
        1, sizeof(FCGI_BEGIN_REQUEST_BODY_S));
    fcgi_MakeBeginRequestBody(&stBeginRecord.stBody, FCGI_RESPONDER, FCGI_NO_KEEP_CONN);
    if (BS_OK != VBUF_CatBuf(&pstFcgi->stVbuf,
                    (UCHAR*)&stBeginRecord, sizeof(FCGI_BEGIN_REQUEST_RECORD_S)))
    {
        return BS_ERR;
    }

    if (BS_OK != fcgi_BuildRequestParams(pstFcgi))
    {
        return BS_ERR;
    }

    return BS_OK;
}

/* 构造应答Header */
static BS_STATUS fcgi_BuildResponseHeader(IN _FCGI_CTRL_S *pstFcgi)
{
    FCGI_HEADER_S stHeader;
    FCGI_HEADER_S *pstHeader;
    FCGI_PARAM_S *pstNode;
    DLL_HEAD_S *pstParamList = &pstFcgi->stResponseParams;
    ULONG ulParamLen;
    HANDLE hVbuf = &pstFcgi->stVbuf;
    UCHAR *pucPadding = (UCHAR*)"\0\0\0\0\0\0\0\0";
    
    fcgi_MakeHeader(&stHeader, FCGI_STDOUT, pstFcgi->usRequestId, 0);
    if (BS_OK != VBUF_CatBuf(hVbuf, (UCHAR*)&stHeader, sizeof(FCGI_HEADER_S)))
    {
        return BS_ERR;
    }

    /* 添加Params */
    if (DLL_COUNT(pstParamList) > 0)
    {
        DLL_SCAN(pstParamList, pstNode)
        {
            if (BS_OK != VBUF_CatBuf(hVbuf, pstNode->pcName, strlen(pstNode->pcName)))
            {
                return BS_ERR;
            }

            if (BS_OK != VBUF_CatBuf(hVbuf, ": ", 2))
            {
                return BS_ERR;
            }

            if (BS_OK != VBUF_CatBuf(hVbuf, pstNode->pcValue, strlen(pstNode->pcValue)))
            {
                return BS_ERR;
            }

            if (BS_OK != VBUF_CatBuf(hVbuf, "\r\n", 2))
            {
                return BS_ERR;
            }
        }

        
        if (BS_OK != VBUF_CatBuf(hVbuf, "\r\n", 2))
        {
            return BS_ERR;
        }
    }
    else
    {
        if (BS_OK != VBUF_CatBuf(hVbuf, "\r\n\r\n", 4))
        {
            return BS_ERR;
        }
    }

    ulParamLen = VBUF_GetDataLength(hVbuf) - sizeof(FCGI_HEADER_S);
    pstHeader = VBUF_GetData(hVbuf);
    fcgi_MakeHeader(&stHeader, FCGI_STDOUT, pstFcgi->usRequestId, (USHORT)ulParamLen);

    /* 填充Padding */
    if (BS_OK != VBUF_CatBuf(hVbuf, pucPadding, pstHeader->ucPaddingLen))
    {
        return BS_ERR;
    }

    /* 构造PARAMS结束标志 */
    fcgi_MakeHeader(&stHeader, FCGI_STDOUT, pstFcgi->usRequestId, 0);
    if (BS_OK != VBUF_CatBuf(hVbuf, (UCHAR*)&stHeader, sizeof(FCGI_HEADER_S)))
    {
        return BS_ERR;
    }

    return BS_OK;
}


/* 获取Param */
static CHAR * fcgi_GetParam(IN DLL_HEAD_S *pstDllHead, IN CHAR *pcParamName)
{
    FCGI_PARAM_S *pstNode;

    DLL_SCAN(pstDllHead, pstNode)
    {
        if (0 == strcmp(pstNode->pcName, pcParamName))
        {
            return pstNode->pcValue;
        }
    }

    return NULL;
}

/* 获取Next Param, pstParam: 当前Param. 如果是NULL, 则表示取第一个Param */ 
static FCGI_PARAM_S * fcgi_GetNextParam(IN DLL_HEAD_S *pstDllHead, IN FCGI_PARAM_S *pstParam)
{
    return DLL_NEXT(pstDllHead, pstParam);
}

static BS_STATUS fcgi_WriteDataInBuf(IN _FCGI_CTRL_S *pstFcgi)
{
    HANDLE hVbuf = &pstFcgi->stVbuf;
    ULONG ulVbufLen = VBUF_GetDataLength(hVbuf);
    UINT ulWriteSize;

    if (ulVbufLen > 0)
    {
        if (BS_OK != SSLTCP_Write(pstFcgi->uiSslTcpId, VBUF_GetData(hVbuf), ulVbufLen, &ulWriteSize))
        {
            return BS_ERR;
        }

        VBUF_CutHead(hVbuf, ulWriteSize);
    }

    return BS_OK;
}

static BS_STATUS fcgi_WriteDataOnce
(
    IN _FCGI_CTRL_S *pstFcgi,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiWriteLen
)
{
    UINT uiTotleSendLen = 0;
    UINT uiSegLen;
    FCGI_HEADER_S stHeader;
    UINT uiWillSendLen;
    UINT uiSendLen;
    UCHAR *pucPadding = (UCHAR*)"\0\0\0\0\0\0\0\0";
    
    for(;;)
    {
        /* 发送缓冲区中的数据 */
        if (BS_OK != fcgi_WriteDataInBuf(pstFcgi))
        {
            return BS_ERR;
        }

        /* 如果缓冲区数据发送未完成,则返回等待下次发送 */
        if (VBUF_GetDataLength(&pstFcgi->stVbuf) > 0)
        {
            break;
        }

        if (pstFcgi->uiRemainDataLen > 0)
        {
            uiWillSendLen = MIN(pstFcgi->uiRemainDataLen, uiDataLen - uiTotleSendLen);
            if (uiWillSendLen > 0)
            {
                if (BS_OK != SSLTCP_Write(pstFcgi->uiSslTcpId,
                    pucData + uiTotleSendLen,
                    uiWillSendLen, &uiSendLen))
                {
                    return BS_ERR;
                }
                uiTotleSendLen += uiSendLen;
                pstFcgi->uiRemainDataLen -= uiSendLen;
            }

            if (pstFcgi->uiRemainDataLen > 0)
            {
                break;
            }
        }

        if (pstFcgi->ucReaminPaddingLen > 0)
        {
            if (BS_OK != VBUF_CatBuf(&pstFcgi->stVbuf, pucPadding,
                pstFcgi->ucReaminPaddingLen))
            {
                return BS_ERR;
            }
            pstFcgi->ucReaminPaddingLen = 0;
        }

        if (uiDataLen > uiTotleSendLen)
        {
            /* 构造分段头 */
            uiSegLen = MIN(FCGI_MAX_SEGMENT_SIZE, uiDataLen - uiTotleSendLen);
            fcgi_MakeHeader(&stHeader, FCGI_STDIN, pstFcgi->usRequestId, (USHORT)uiSegLen);
            if (BS_OK != VBUF_CatBuf(&pstFcgi->stVbuf, (UCHAR*)&stHeader, sizeof(FCGI_HEADER_S)))
            {
                return BS_ERR;
            }
            pstFcgi->uiRemainDataLen = uiSegLen;
            pstFcgi->ucReaminPaddingLen = stHeader.ucPaddingLen;
        }

        /* 如果没有Padding和分段头需要发送, 则退出循环 */
        if (VBUF_GetDataLength(&pstFcgi->stVbuf) == 0)
        {
            break;
        }
    }

    *puiWriteLen = uiTotleSendLen;

    return BS_OK;        
}

/* 发送用户数据. 阻塞式循环发送,非阻塞式发送一次 */
static BS_STATUS fcgi_WriteData
(
    IN _FCGI_CTRL_S *pstFcgi,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiWriteLen
)
{
    BS_STATUS eRet;
    UINT uiWriteLen;
    UINT uiTotleWriteLen = 0;
    
    if (pstFcgi->uiFlag & FCGI_FLAG_NOBLOCK)
    {
        return fcgi_WriteDataOnce(pstFcgi, pucData, uiDataLen, puiWriteLen);
    }

    do {
        eRet = fcgi_WriteDataOnce(pstFcgi, pucData + uiTotleWriteLen,
            uiDataLen - uiTotleWriteLen, &uiWriteLen);
        if (BS_OK != eRet)
        {
            return eRet;
        }
        uiTotleWriteLen += uiWriteLen;
    }while(uiTotleWriteLen < uiDataLen);

    return BS_OK;
}

/* Description  : 创建Server端通道 */
FCGI_HANDLE FCGI_ServerCreate(IN UINT uiSsltcpId, IN UINT uiFlag)
{
    _FCGI_CTRL_S *pstFcgi;
    
    pstFcgi = fcgi_Create(uiSsltcpId, uiFlag, TRUE);
    if (NULL != pstFcgi)
    {
        pstFcgi->eStatus = FCGI_STATUS_PARSE_PARAM;
    }

    return pstFcgi;
}

/* 创建Client端通道 */
FCGI_HANDLE FCGI_ClientCreate(IN UINT uiSsltcpId, IN UINT uiFlag)
{
    _FCGI_CTRL_S *pstFcgi;

    pstFcgi = fcgi_Create(uiSsltcpId, uiFlag, FALSE);
    if (NULL != pstFcgi)
    {
        pstFcgi->eStatus = FCGI_STATUS_SET_PARAM;
    }

    return pstFcgi;
}


/* 销毁FCGI通道 */
VOID FCGI_Destory(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;
    FCGI_PARAM_S *pstNode, *pstNodeNext;

    VBUF_Finit(&pstFcgi->stVbuf);

    DLL_SAFE_SCAN(&(pstFcgi->stRequestParams), pstNode, pstNodeNext)
    {
        fcgi_FreeParamNode(pstNode);
    }

    DLL_SAFE_SCAN(&(pstFcgi->stResponseParams), pstNode, pstNodeNext)
    {
        fcgi_FreeParamNode(pstNode);
    }

    MEM_Free(pstFcgi);
}


/* 得到和FCGI通道绑定在一起的SSLTCP ID */
UINT FCGI_GetSsltcpId(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    return pstFcgi->uiSslTcpId;
}

/* 接收Seg Header */
static BS_STATUS fcgi_RecvSegHeader(IN _FCGI_CTRL_S *pstFcgi)
{
    UINT uiReadLen;
    FCGI_HEADER_S *pstHeader;
    
    /* 如果待接收的数据是0，说明是第一次调用这个函数,还没有接收到任何数据 */
    if (pstFcgi->uiRemainDataLen == 0)
    {
        pstFcgi->uiRemainDataLen = sizeof(FCGI_HEADER_S);
    }

    if (BS_OK != SSLTCP_Read(pstFcgi->uiSslTcpId,
                pstFcgi->stSegHeadData.aucSegData + pstFcgi->stSegHeadData.uiDataLen,
                pstFcgi->uiRemainDataLen, &uiReadLen))
    {
        return BS_ERR;
    }
    pstFcgi->uiRemainDataLen -= uiReadLen;
    pstFcgi->stSegHeadData.uiDataLen += uiReadLen;

    /* 如果接收完成,则进行处理并切换状态 */
    if (pstFcgi->uiRemainDataLen == 0)
    {
        pstHeader = (FCGI_HEADER_S*)pstFcgi->stSegHeadData.aucSegData;

        pstFcgi->eDataType = pstHeader->ucType;
        pstFcgi->uiRemainDataLen = ntohs(pstHeader->usContentLen);
        pstFcgi->ucReaminPaddingLen = pstHeader->ucPaddingLen;
        if (pstFcgi->uiRemainDataLen == 0)
        {
            pstFcgi->eRecvSegType = FCGI_RECV_TYPE_END;
        }
        else
        {
            pstFcgi->eRecvSegType = FCGI_RECV_SEG_DATA;
        }
    }

    return BS_OK;    
}

/* 接收Seg Data */
static BS_STATUS fcgi_RecvBegineRequestData(IN _FCGI_CTRL_S *pstFcgi)
{
    UINT uiReadLen;
    FCGI_BEGIN_REQUEST_BODY_S *pstRequestBody;

    if (BS_OK != SSLTCP_Read(pstFcgi->uiSslTcpId,
                pstFcgi->stSegHeadData.aucSegData + pstFcgi->stSegHeadData.uiDataLen,
                pstFcgi->uiRemainDataLen, &uiReadLen))
    {
        return BS_ERR;
    }
    pstFcgi->uiRemainDataLen -= uiReadLen;
    pstFcgi->stSegHeadData.uiDataLen += uiReadLen;

    /* 如果接收完成,则进行处理并切换状态 */
    if (pstFcgi->uiRemainDataLen == 0)
    {
        pstRequestBody = (FCGI_BEGIN_REQUEST_BODY_S*)(pstFcgi->stSegHeadData.aucSegData);
        pstFcgi->ucFlag = pstRequestBody->ucFlags;
        pstFcgi->eRecvSegType = FCGI_RECV_SEG_HEADER;
    }

    return BS_OK;
}

/* 接收Seg Data */
static BS_STATUS fcgi_RecvEndRequestData(IN _FCGI_CTRL_S *pstFcgi)
{
    UINT uiReadLen;

    if (BS_OK != SSLTCP_Read(pstFcgi->uiSslTcpId,
                pstFcgi->stSegHeadData.aucSegData + pstFcgi->stSegHeadData.uiDataLen,
                pstFcgi->uiRemainDataLen, &uiReadLen))
    {
        return BS_ERR;
    }
    pstFcgi->uiRemainDataLen -= uiReadLen;
    pstFcgi->stSegHeadData.uiDataLen += uiReadLen;

    /* 如果接收完成,则进行处理并切换状态 */
    if (pstFcgi->uiRemainDataLen == 0)
    {
        pstFcgi->eStatus = FCGI_STATUS_DONE;
        pstFcgi->eRecvSegType = FCGI_RECV_END;
    }

    return BS_OK;
}

/* 接收Padding */
static BS_STATUS fcgi_RecvPadding(IN _FCGI_CTRL_S *pstFcgi)
{
    CHAR acPadding[8];
    UINT ulReadLen;

    return SSLTCP_Read(pstFcgi->uiSslTcpId, acPadding, pstFcgi->ucReaminPaddingLen, &ulReadLen);
}

/* 接收Seg Data */
static BS_STATUS fcgi_RecvParamData(IN _FCGI_CTRL_S *pstFcgi)
{
    CHAR szParams[1024];
    UINT uiReadLen;

    while (pstFcgi->uiRemainDataLen > 0)
    {
        if (BS_OK != SSLTCP_Read(pstFcgi->uiSslTcpId, szParams, sizeof(szParams), &uiReadLen))
        {
            return BS_ERR;
        }

        if (uiReadLen == 0)
        {
            return BS_OK;
        }

        pstFcgi->uiRemainDataLen -= uiReadLen;

        VBUF_CatBuf(&pstFcgi->stVbuf, szParams, uiReadLen);
    }

    if (pstFcgi->ucReaminPaddingLen > 0)
    {
        if (BS_OK != fcgi_RecvPadding(pstFcgi))
        {
            return BS_ERR;
        }
    }

    if (pstFcgi->ucReaminPaddingLen == 0)
    {
        pstFcgi->eRecvSegType = FCGI_RECV_SEG_HEADER;
    }

    return BS_OK;
}

/* 接收用户数据 */
static BS_STATUS fcgi_RecvUserData
(
    IN _FCGI_CTRL_S *pstFcgi,
    OUT UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiReadLen
)
{
    UINT uiEmptyBufLen = 0;
    UINT uiReadLen = 0;
    UINT uiReadTotleLen = 0;

    uiEmptyBufLen = uiDataLen;

    while ((pstFcgi->uiRemainDataLen > 0) && (uiEmptyBufLen > 0))
    {
        if (BS_OK != SSLTCP_Read(pstFcgi->uiSslTcpId,
                     pucData + uiReadTotleLen,
                     MIN(pstFcgi->uiRemainDataLen, uiEmptyBufLen),
                     &uiReadLen))
        {
            return BS_ERR;
        }

        if (uiReadLen == 0)
        {
            *puiReadLen = uiReadTotleLen;
            return BS_OK;
        }
        pstFcgi->uiRemainDataLen -= uiReadLen;
        uiEmptyBufLen -= uiReadLen;
        uiReadTotleLen += uiReadLen;
    }

    *puiReadLen = uiReadTotleLen;

    if (pstFcgi->uiRemainDataLen > 0)
    {
        return BS_OK;
    }

    if (pstFcgi->ucReaminPaddingLen > 0)
    {
        if (BS_OK != fcgi_RecvPadding(pstFcgi))
        {
            return BS_ERR;
        }
    }

    if (pstFcgi->ucReaminPaddingLen == 0)
    {
        pstFcgi->eRecvSegType = FCGI_RECV_SEG_HEADER;
    }

    return BS_OK;
}

/* 接收Err数据 */
static BS_STATUS fcgi_RecvErrData
(
    IN _FCGI_CTRL_S *pstFcgi,
    OUT UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiReadLen
)
{
    UCHAR aucData[1024];
    UINT uiReadLen;

    while (pstFcgi->uiRemainDataLen > 0)
    {
        if (BS_OK != SSLTCP_Read(pstFcgi->uiSslTcpId, aucData, sizeof(aucData), &uiReadLen))
        {
            return BS_ERR;
        }

        if (uiReadLen == 0)
        {
            return BS_OK;
        }

        pstFcgi->uiRemainDataLen -= uiReadLen;
    }

    if (pstFcgi->ucReaminPaddingLen > 0)
    {
        if (BS_OK != fcgi_RecvPadding(pstFcgi))
        {
            return BS_ERR;
        }
    }

    if (pstFcgi->ucReaminPaddingLen == 0)
    {
        pstFcgi->eRecvSegType = FCGI_RECV_SEG_HEADER;
    }

    return BS_OK;

}


/* 接收Seg Data */
static BS_STATUS fcgi_RecvSegData
(
    IN _FCGI_CTRL_S *pstFcgi,
    OUT UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiReadLen
)
{
    BS_STATUS eRet = BS_ERR;
    switch (pstFcgi->eDataType)
    {
        case FCGI_BEGIN_REQUEST:
        {
            eRet = fcgi_RecvBegineRequestData(pstFcgi);
            break;
        }
        case FCGI_ABORT_REQUEST:
        {
            return BS_ERR;
        }
        case FCGI_END_REQUEST:
        {
            eRet = fcgi_RecvEndRequestData(pstFcgi);
            break;
        }
        case FCGI_PARAMS:
        {
            eRet = fcgi_RecvParamData(pstFcgi);
            break;
        }
        case FCGI_STDIN:
        case FCGI_STDOUT:
        {
            eRet = fcgi_RecvUserData(pstFcgi, pucData, uiDataLen, puiReadLen);
            break;
        }
        case FCGI_STDERR:
        {
            eRet = fcgi_RecvErrData(pstFcgi, NULL, 0, NULL);
            break;
        }
        default:
        {
            BS_DBGASSERT(0);
            break;
        }
    }

    return eRet;
}

/* 接收Seg Data */
static BS_STATUS fcgi_RecvedParamEnd(IN _FCGI_CTRL_S *pstFcgi)
{
    HANDLE hVbuf = &pstFcgi->stVbuf;
    UCHAR *pucParam;
    ULONG ulParamLen;
    UINT uiParamNamelen;
    UINT uiParamValueLen;
    
    /* 开始解析Param */
    pucParam = VBUF_GetData(hVbuf);
    ulParamLen = VBUF_GetDataLength(hVbuf);

    if ((NULL == pucParam) || (0 == ulParamLen))
    {
        return BS_OK;   /* 不存在Param是正常的 */
    }

    while (ulParamLen > 0)
    {
        if (pucParam[0] & 0x80)
        {
            if (ulParamLen < 4)
            {
                return BS_ERR;
            }
            uiParamNamelen = (UINT)(pucParam[0] & 0x7f) << 24;
            uiParamNamelen |= (UINT)pucParam[1] << 16;
            uiParamNamelen |= (UINT)pucParam[2] << 8;
            uiParamNamelen |= (UINT)pucParam[3];
            ulParamLen -= 4;
            pucParam += 4;
        }
        else
        {
            uiParamNamelen = pucParam[0];
            ulParamLen -= 1;
            pucParam += 1;
        }

        if (ulParamLen == 0)
        {
            return BS_ERR;
        }
        
        if (pucParam[0] & 0x80)
        {
            if (ulParamLen < 4)
            {
                return BS_ERR;
            }
            uiParamValueLen = (UINT)(pucParam[0] & 0x7f) << 24;
            uiParamValueLen |= (UINT)pucParam[1] << 16;
            uiParamValueLen |= (UINT)pucParam[2] << 8;
            uiParamValueLen |= (UINT)pucParam[3];
            ulParamLen -= 4;
            pucParam += 4;
        }
        else
        {
            uiParamValueLen = pucParam[0];
            ulParamLen -= 1;
            pucParam += 1;
        }
        
        if (ulParamLen < uiParamNamelen + uiParamValueLen)
        {
            return BS_ERR;
        }
        
        if (BS_OK != fcgi_SetParam(&pstFcgi->stRequestParams,
                    (CHAR*)pucParam, uiParamNamelen,
                    (CHAR*)pucParam + uiParamNamelen, uiParamValueLen))
        {
            return BS_ERR;
        }
        ulParamLen -= (uiParamNamelen + uiParamValueLen);
        pucParam += (uiParamNamelen + uiParamValueLen);
    }

    pstFcgi->eStatus = FCGI_STATUS_READ;

    return BS_OK;
}

/* 接收Type End消息 */
static BS_STATUS fcgi_RecvedTypeEnd(IN _FCGI_CTRL_S *pstFcgi)
{
    BS_STATUS eRet = BS_ERR;
    switch (pstFcgi->eDataType)
    {
        case FCGI_PARAMS:
        {
            eRet = fcgi_RecvedParamEnd(pstFcgi);
            break;
        }
        default:
        {
            BS_DBGASSERT(0);
            break;
        }
    }

    return eRet;
}

/* 解析FCGI请求头 */
static BS_STATUS fcgi_ParseRequestHead(IN _FCGI_CTRL_S *pstFcgi)
{
    _FCGI_RECV_SEG_TYPE_E eOldRecvSegType;
    BS_STATUS eRet = BS_OK;

    if (pstFcgi->eStatus != FCGI_STATUS_PARSE_PARAM)
    {
        return BS_OK;
    }
    
    do {
        eOldRecvSegType = pstFcgi->eRecvSegType;

        switch (pstFcgi->eRecvSegType)
        {
            case FCGI_RECV_SEG_HEADER:
            {
                eRet = fcgi_RecvSegHeader(pstFcgi);
                break;
            }
            case FCGI_RECV_SEG_DATA:
            {
                eRet = fcgi_RecvSegData(pstFcgi, NULL, 0, NULL);
                break;
            }
            case FCGI_RECV_TYPE_END:
            {
                eRet = fcgi_RecvedTypeEnd(pstFcgi);
                break;
            }
            default:
            {
                eRet = BS_OK;
                break;
            }
        }
    }while ((BS_OK == eRet)
                && (eOldRecvSegType != pstFcgi->eRecvSegType)
                && (pstFcgi->eStatus == FCGI_STATUS_PARSE_PARAM));

    return eRet;
}

/* 解析FCGI应答头 */
static BS_STATUS fcgi_ParseResponseHead(IN _FCGI_CTRL_S *pstFcgi)
{
    _FCGI_RECV_SEG_TYPE_E eOldRecvSegType;
    BS_STATUS eRet = BS_OK;

    if (pstFcgi->eStatus != FCGI_STATUS_PARSE_PARAM)
    {
        return BS_OK;
    }
    
    do {
        eOldRecvSegType = pstFcgi->eRecvSegType;

        switch (pstFcgi->eRecvSegType)
        {
            case FCGI_RECV_SEG_HEADER:
            {
                eRet = fcgi_RecvSegHeader(pstFcgi);
                break;
            }
            case FCGI_RECV_SEG_DATA:
            {
                eRet = fcgi_RecvSegData(pstFcgi, NULL, 0, NULL);
                break;
            }
            case FCGI_RECV_TYPE_END:
            {
                eRet = fcgi_RecvedTypeEnd(pstFcgi);
                break;
            }
            default:
            {
                eRet = BS_OK;
                break;
            }
        }
    }while ((BS_OK == eRet)
                && (eOldRecvSegType != pstFcgi->eRecvSegType)
                && (pstFcgi->eStatus == FCGI_STATUS_PARSE_PARAM));

    return eRet;
}

/* 解析FCGI头 */
BS_STATUS FCGI_ParseHead(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    if (pstFcgi->bIsServer == TRUE)
    {
        return fcgi_ParseRequestHead(pstFcgi);
    }
    else
    {
        return fcgi_ParseResponseHead(pstFcgi);
    }
}

/* 解析FCGI头是否完成 */
BOOL_T FCGI_IsParseHeadOk(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    if (pstFcgi->eStatus == FCGI_STATUS_PARSE_PARAM)
    {
        return FALSE;
    }

    return TRUE;
}

/* 设置头参数 */
BS_STATUS FCGI_SetParam(IN FCGI_HANDLE hFcgiChannel, IN CHAR *pcParamName, IN CHAR *pcParamValue)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;
    DLL_HEAD_S *pstParamList;
    CHAR *pcValue = pcParamValue;

    if (NULL == pcValue)
    {
        pcValue = "";
    }    

    if (pstFcgi->bIsServer == TRUE)
    {
        pstParamList = &pstFcgi->stResponseParams;
    }
    else
    {
        pstParamList = &pstFcgi->stRequestParams;
    }

    return fcgi_SetParam(pstParamList,
                pcParamName, strlen(pcParamName),
                pcValue, strlen(pcValue));
}

/* 设置头参数完成 */
BS_STATUS FCGI_SetParamFinish(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;


    if (pstFcgi->bIsServer == TRUE)
    {
        if (BS_OK != fcgi_BuildResponseHeader(pstFcgi))
        {
            return BS_ERR;
        }
    }
    else
    {
        if (BS_OK != fcgi_BuildRequestHeader(pstFcgi))
        {
            return BS_ERR;
        }
    }

    return BS_OK;
}

/* 获取Request Param */
CHAR * FCGI_GetRequestParam(IN FCGI_HANDLE hFcgiChannel, IN CHAR *pcParamName)
{
    
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    return fcgi_GetParam(&pstFcgi->stRequestParams, pcParamName);
}

/* 获取Response Param */
CHAR * FCGI_GetResponseParam(IN FCGI_HANDLE hFcgiChannel, IN CHAR *pcParamName)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    return fcgi_GetParam(&pstFcgi->stResponseParams, pcParamName);
}

/* 获取下一个Request Param, pstParam: 当前Param. 如果是NULL, 则表示取第一个Param */
FCGI_PARAM_S * FCGI_GetNextRequestParam(IN FCGI_HANDLE hFcgiChannel, IN FCGI_PARAM_S *pstParam)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;
    
    return fcgi_GetNextParam(&pstFcgi->stRequestParams, pstParam);
}

/* 获取下一个Response Param, pstParam: 当前Param. 如果是NULL, 则表示取第一个Param */
FCGI_PARAM_S * FCGI_GetNextResponseParam(IN FCGI_HANDLE hFcgiChannel, IN FCGI_PARAM_S *pstParam)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;
    
    return fcgi_GetNextParam(&pstFcgi->stResponseParams, pstParam);
}

/* 读取FCGI数据 */
static BS_STATUS fcgi_Read
(
    IN _FCGI_CTRL_S *pstFcgi,
    OUT UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiReadLen
)
{
    _FCGI_RECV_SEG_TYPE_E eOldRecvSegType;
    BS_STATUS eRet = BS_OK;

    do {
        eOldRecvSegType = pstFcgi->eRecvSegType;

        switch (pstFcgi->eRecvSegType)
        {
            case FCGI_RECV_SEG_HEADER:
            {
                eRet = fcgi_RecvSegHeader(pstFcgi);
                break;
            }
            case FCGI_RECV_SEG_DATA:
            {
                eRet = fcgi_RecvSegData(pstFcgi, pucData, uiDataLen, puiReadLen);
                break;
            }
            case FCGI_RECV_TYPE_END:
            {
                eRet = fcgi_RecvedTypeEnd(pstFcgi);
                break;
            }
            default:
            {
                eRet = BS_OK;
                break;
            }
        }
    }while ((BS_OK == eRet)
                && (eOldRecvSegType != pstFcgi->eRecvSegType)
                && (pstFcgi->eStatus == FCGI_STATUS_PARSE_PARAM));

    return eRet;
}


/* 读取FCGI数据 */
BS_STATUS FCGI_Read
(
    IN FCGI_HANDLE hFcgiChannel,
    OUT UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiReadLen
)
{
    return fcgi_Read(hFcgiChannel, pucData, uiDataLen, puiReadLen);
}

/* 是否读取FCGI数据结束 */
BOOL_T FCGI_IsReadEOF(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    if (pstFcgi->bIsServer)
    {
        if (pstFcgi->eStatus >=FCGI_STATUS_SET_PARAM)
        {
            return TRUE;
        }
    }
    else
    {
        if (pstFcgi->eStatus >= FCGI_STATUS_DONE)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/* 发送数据 */
BS_STATUS FCGI_Write
(
    IN FCGI_HANDLE hFcgiChannel,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiWriteLen
)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    return fcgi_WriteData(pstFcgi, pucData, uiDataLen, puiWriteLen);
}

/* 发送数据结束 */
BS_STATUS FCGI_WriteFinish(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;
    FCGI_HEADER_S stHeader;
    FCGI_END_REQUEST_RECORD_S stEndRequest;
    UINT uiWriteLen;

    if (pstFcgi->bIsServer == TRUE)
    {
        fcgi_MakeHeader(&stEndRequest.stHeader, FCGI_END_REQUEST,
            pstFcgi->usRequestId, sizeof(FCGI_END_REQUEST_BODY_S));
        fcgi_MakeEndRequestBody(&stEndRequest.stBody);

        if (BS_OK != VBUF_CatBuf(&pstFcgi->stVbuf, (UCHAR*)&stEndRequest, sizeof(FCGI_END_REQUEST_RECORD_S)))
        {
            return BS_ERR;
        }
    }
    else
    {
        fcgi_MakeHeader(&stHeader, FCGI_STDIN, pstFcgi->usRequestId, 0);
        if (BS_OK != VBUF_CatBuf(&pstFcgi->stVbuf, (UCHAR*)&stHeader, sizeof(FCGI_HEADER_S)))
        {
            return BS_ERR;
        }
    }

    return fcgi_WriteData(hFcgiChannel, NULL, 0, &uiWriteLen);
}

/* 得到发送缓冲区中数据长度 */
ULONG FCGI_GetDataLenInSendBuf(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    return VBUF_GetDataLength(&pstFcgi->stVbuf);
}

/* 发送FCGI缓冲区中的数据 */
BS_STATUS FCGI_Flush(IN FCGI_HANDLE hFcgiChannel)
{
    UINT uiWriteLen;

    return fcgi_WriteData(hFcgiChannel, NULL, 0, &uiWriteLen);
}

/* 得到FCGI通道状态 */
FCGI_STATUS_E FCGI_GetStatus(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    return pstFcgi->eStatus;
}

/* 是否保持长连接 */
BOOL_T FCGI_IsKeepAlive(IN FCGI_HANDLE hFcgiChannel)
{
    _FCGI_CTRL_S *pstFcgi = hFcgiChannel;

    return pstFcgi->ucFlag == FCGI_KEEP_CONN ? FALSE : TRUE;
}


