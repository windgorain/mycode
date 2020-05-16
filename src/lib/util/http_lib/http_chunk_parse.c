/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-21
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/ctype_utl.h"
#include "utl/http_lib.h"


#define HTTP_CHUNK_FLAG_MAX_SIZE                   1024UL

#define CHUNK_UPDATE_STATUS( _p, ulLen )    \
{                                           \
    (_p)->uiDataLen -= ulLen;               \
    (_p)->pucData   += ulLen;               \
}

typedef enum tagHTTP_ChunkStatus
{
    HTTP_CHUNK_BEGIN_FLAG,      /* 解析chunk开始标记 */
    HTTP_CHUNK_BODY,            /* 解析chunk体信息 */
    HTTP_CHUNK_END_FLAG,        /* 解析chunk结束标记 */
    HTTP_CHUNK_OVER_FLAG,       /* 解析chunk完成时最后的\r\n标记 */
    HTTP_CHUNK_BUTT             /* Butt */
}HTTP_CHUNK_STATUS_E;

typedef struct tagHTTP_ChunkParse
{
    PF_HTTP_CHUNK_FUNC pfFunc;            /* 用户的回调函数 */
    USER_HANDLE_S stUserHandle;           /* 用户回调函数参数 */
    UCHAR *pucData;                       /* 当前收到的数据指针 */
    UINT uiDataLen;                      /* 当前未解析的数据长度 */
    HTTP_CHUNK_STATUS_E enCurState;       /* 当前解析的状态 */
    UINT uiOverFlag;                     /* 解析的OverFlag状态 */
    UINT uiResLength;                    /* 处理BODY时,表示当前CHUNK中待解析CHUNK块长度 ，不包含\r\n。
                                            处理EndFlag时，表示剩余待解析的\r\n长度*/
    UINT uiLastDataLen;                  /* 上次未处理完的数据长度，存储到szChunkBegin中的begin标识长度 */
    UCHAR aucChunkBegin[HTTP_CHUNK_FLAG_MAX_SIZE]; /* BEGIN标志数据 */
}HTTP_CHUNK_PARSE_S;




/*******************************************************************************
  构造Chunk Begin标记                                                             
*******************************************************************************/
VOID HTTP_Chunk_BuildChunkFlag(IN UINT64 ui64DataLen, OUT CHAR szChunkBeginFlag[HTTP_CHUNK_FLAG_MAX_LEN + 1])
{
    (VOID)snprintf(szChunkBeginFlag, HTTP_CHUNK_FLAG_MAX_LEN + 1, "%llx\r\n", ui64DataLen);

    return;
}


/*******************************************************************************
  判断CHUNK开始标记中的16进制数据                                                             
*******************************************************************************/
STATIC BOOL_T http_chunk_CheckHex(IN UCHAR *pucHex, IN ULONG ulLen)
{
    ULONG i;

    BS_DBGASSERT(NULL != pucHex);
    BS_DBGASSERT(0 != ulLen);

    for (i = 0; i < ulLen; i++)
    {
        if (! CTYPE_IsXDigit(pucHex[i]))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*******************************************************************************
  得到Chunk数据块长度                                                             
*******************************************************************************/
BS_STATUS HTTP_Chunk_GetChunkDataLen
(
    IN UCHAR *pucData,
    IN ULONG ulDataLen,
    OUT ULONG *pulChunkDataLen,
    OUT ULONG *pulChunkFlagLen
)
{
    UCHAR *pucCRLF = NULL;
    ULONG ulTemp = 0;
    UCHAR *pucSem = NULL;
    CHAR szSize[HTTP_MAX_UINT_HEX_LEN + 1];
    ULONG ulCRLFLen = 0;                    /* 从起始位置到CRLF的长度 */
    ULONG ulLen = 0;    

    /* 入参合法性判断 */
    if((NULL == pucData) || (0 == ulDataLen) || (NULL == pulChunkDataLen) || (NULL == pulChunkFlagLen))
    {
        return BS_ERR;
    }

    /* 搜索"\r\n"，找到起始字段的结束位置。 */
    pucCRLF = (UCHAR*)MEM_Find((VOID*)pucData, ulDataLen, HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);
    if (NULL == pucCRLF)
    {
        if(ulDataLen < HTTP_CHUNK_FLAG_MAX_SIZE)
        {
            return BS_NOT_COMPLETE;
        }
        /* 不支持超过1024字节的起始字段 */
        return BS_ERR;
    }
    ulCRLFLen = (ULONG)pucCRLF - (ULONG)pucData;

    /* 搜索';'，如果存在，则将';'后的chunk-extension去掉 */
    pucSem = (UCHAR*)memchr((VOID*)pucData, HTTP_SEMICOLON_CHAR, ulCRLFLen);
    /* 设置chunk-size字段长度 */
    if (NULL == pucSem)
    {
        ulLen = ulCRLFLen;
    }
    else
    {
        ulLen = (ULONG)pucSem - (ULONG)pucData;
    }

    /* 暂不支持超过4G, 以后再扩展 */
    if (ulLen > HTTP_MAX_UINT_HEX_LEN)
    {
        return BS_ERR;  
    }

    /* 判断chunk-size字段是否为合法十六进制数 */
    if (TRUE != http_chunk_CheckHex(pucData, ulLen))
    {
        return BS_ERR;
    }    

    /* 将chunk-size字段转换为十进制数字 */
    memcpy(szSize, pucData, ulLen);
    szSize[ulLen] = '\0';
    ulTemp = (ULONG)strtol(szSize, (CHAR **)NULL, 16);
    *pulChunkDataLen = (ULONG)ulTemp;
    *pulChunkFlagLen = ulCRLFLen + HTTP_LINE_END_FLAG_LEN;

    return BS_OK;
}

/*******************************************************************************
  创建Chunk解析器                                                             
*******************************************************************************/
HTTP_CHUNK_HANDLE HTTP_Chunk_CreateParser(IN PF_HTTP_CHUNK_FUNC pfFunc,IN USER_HANDLE_S *pstUserPointer)
{ 
    HTTP_CHUNK_PARSE_S *pstChunk = NULL;

    /* 入参合法性检查 */
    if(NULL == pfFunc)
    {
        return NULL;
    }
    
    pstChunk = (HTTP_CHUNK_PARSE_S *)malloc(sizeof(HTTP_CHUNK_PARSE_S));
    if(pstChunk == NULL)
    {
        return NULL;
    }
    /* 初始化 */
    memset(pstChunk, 0, sizeof(HTTP_CHUNK_PARSE_S));
    
    if (NULL != pstUserPointer)
    {
        pstChunk->stUserHandle = *pstUserPointer;
    }
    pstChunk->pfFunc = pfFunc;
    pstChunk->enCurState = HTTP_CHUNK_BEGIN_FLAG;

    return pstChunk;
}

/*******************************************************************************
  Chunk销毁函数
*******************************************************************************/
VOID HTTP_Chunk_DestoryParser(IN HTTP_CHUNK_HANDLE hParser)
{
    if(NULL != hParser)
    {
        free(hParser);            
    }
    return ;
}

/*******************************************************************************
  解析Chunk起始标记，包括\r\n                                                            
*******************************************************************************/
STATIC BS_STATUS http_chunk_ParseBegin(IN HTTP_CHUNK_PARSE_S *pstChunk)
{
    ULONG ulChunkDataLen = 0;        /* 当前Chunk的长度 */
    UINT uiCopyLen = 0;             /* 表示需要Copy到szChunkBegin中的值，当前数据中ChunkBegin标记的长度 */
    UINT uiOldDataLen;              /* 上次末处理完的，存在数组中的ChunkBegin标记的长度 */
    ULONG ulChunkFlagLen;
    BS_STATUS eRet;
    
    /* 入参判断 */
    BS_DBGASSERT(NULL != pstChunk);
    BS_DBGASSERT(0 != pstChunk->uiDataLen);

    /* 例如: pstChunk->ulLastDataLen为3，表示aucChunkBegin数组中仍然存有上次放入到3个字节数据，
             pstChunk->ulDataLen为5，表示新需要解析的数据有5个字节，
             由于 HTTP_CHUNK_FLAG_MAX_SIZE - 3 > 5 因此需要将新的5个字节数据拷入aucChunkBegin.
    */
    uiOldDataLen = pstChunk->uiLastDataLen;
    uiCopyLen = HTTP_CHUNK_FLAG_MAX_SIZE - uiOldDataLen;
    uiCopyLen = MIN(uiCopyLen, pstChunk->uiDataLen);

    /* 将可能的ChunkBegin标记存入szChunkBegin中 */
    memcpy((CHAR *)pstChunk->aucChunkBegin + uiOldDataLen, (CHAR *)pstChunk->pucData, (size_t)uiCopyLen);

    /* 更新数组中的ChunkBegin标记的长度 */
    pstChunk->uiLastDataLen += uiCopyLen;  

    /* 得到当前Chunk的长度 */
    eRet = HTTP_Chunk_GetChunkDataLen(pstChunk->aucChunkBegin, pstChunk->uiLastDataLen, &ulChunkDataLen, &ulChunkFlagLen);
    if (BS_OK != eRet)
    {
        pstChunk->uiDataLen -= uiCopyLen;
        return eRet;
    }

    /* 数据指针移动到Chunk数据所在的位置,更新输入数据的剩余数据长度 */
    CHUNK_UPDATE_STATUS(pstChunk, (ulChunkFlagLen - uiOldDataLen));

    /* 存储chunk体数据长度 */
    pstChunk->uiResLength = ulChunkDataLen;

    /* 清零，为下一次begin标记做准备 */
    memset(pstChunk->aucChunkBegin, 0, HTTP_CHUNK_FLAG_MAX_SIZE);
    pstChunk->uiLastDataLen = 0;

    /* 如果体数据长度为0，表示当前解析的为最后一个chunk块 */
    if (0 == ulChunkDataLen)
    {
        pstChunk->enCurState = HTTP_CHUNK_OVER_FLAG;
        pstChunk->uiOverFlag = HTTP_LINE_END_FLAG_LEN;
    }
    else
    {
        /* 如果体数据长度不为0，则需要进行chunk体的解析 */
        pstChunk->enCurState = HTTP_CHUNK_BODY;
    }

    return BS_NOT_COMPLETE;
}

/*******************************************************************************
  解析Chunk体部分
*******************************************************************************/
STATIC BS_STATUS http_chunk_ParseBody(IN HTTP_CHUNK_PARSE_S *pstChunk)
{
    UINT uiBodyLen = 0;  /* 输入数据中包含当前CHUNK块的数据长度 */
    UCHAR *pucDataBody = NULL;
    
    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstChunk);

    if(0 == pstChunk->uiDataLen)
    {
        return BS_NOT_COMPLETE;
    }      
    /* 当前体数据起始位置指针 */
    pucDataBody = pstChunk->pucData;

    /* 当前可解析的体数据长度，取当前输入数据长度(ulDataLen)和chunk块长度(ulResLength)的较小值 */
    uiBodyLen = MIN(pstChunk->uiDataLen, pstChunk->uiResLength);
    if (uiBodyLen > 0)
    {
        /* 调用回调函数进行体数据发送 */
        if (BS_OK != pstChunk->pfFunc(pucDataBody, uiBodyLen, &pstChunk->stUserHandle))
        {
            return BS_ERR;
        }
        /* 更新结构体中存储的数据信息 */
        CHUNK_UPDATE_STATUS(pstChunk, uiBodyLen);
        pstChunk->uiResLength -= uiBodyLen;
    }

    if (0 == pstChunk->uiResLength)
    {
        /* 为chunk结束标记做准备 */
        pstChunk->uiResLength = HTTP_LINE_END_FLAG_LEN;
        pstChunk->enCurState = HTTP_CHUNK_END_FLAG;
    }

    return BS_NOT_COMPLETE;
}

/*******************************************************************************
  解析Chunk块结束标记
*******************************************************************************/
STATIC BS_STATUS http_chunk_ParseEnd(IN HTTP_CHUNK_PARSE_S *pstChunk)
{
    UINT uiLen;
    
    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstChunk);

    if(0 == pstChunk->uiDataLen)
    {
        return BS_NOT_COMPLETE;
    }

    /* 取chunk的结束标记'\r\n' */
    uiLen = MIN(pstChunk->uiDataLen, pstChunk->uiResLength);
    pstChunk->uiResLength -= uiLen;
    pstChunk->uiDataLen -= uiLen;
    pstChunk->pucData += uiLen;

    if (0 == pstChunk->uiResLength)
    {
        pstChunk->enCurState = HTTP_CHUNK_BEGIN_FLAG;
    }

    return BS_NOT_COMPLETE;
}

/*******************************************************************************
  解析Chunk文件传输结束标记
*******************************************************************************/
STATIC BS_STATUS http_chunk_ParseOver(IN HTTP_CHUNK_PARSE_S *pstChunk)
{
    UINT i = 0;
    UCHAR ucChar;
    CHAR *pcOverFlag = HTTP_CRLFCRLF;
    BS_STATUS eRet = BS_NOT_COMPLETE;
    
    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstChunk);

    while (i < pstChunk->uiDataLen)
    {
        ucChar = pstChunk->pucData[i];

        if (pcOverFlag[pstChunk->uiOverFlag] == ucChar)
        {
            pstChunk->uiOverFlag ++;
        }
        else
        {
            if (0 != pstChunk->uiOverFlag)
            {
                pstChunk->uiOverFlag = 0;
                continue;
            }
        }

        if (HTTP_CRLFCRLF_LEN == pstChunk->uiOverFlag)
        {
            /* 告诉用户chunk文件传输完毕 */
            (VOID)pstChunk->pfFunc(NULL, (ULONG)0, &pstChunk->stUserHandle);
            eRet = BS_OK;
            break;
        }
        i++;
    }

    CHUNK_UPDATE_STATUS(pstChunk, pstChunk->uiDataLen);
    
    return eRet;
}

/*******************************************************************************
  解析Chunk
    OK: 接收完成
    NOT_COMPLETE: 需要继续接收数据
    其他: 错误原因
*******************************************************************************/
BS_STATUS HTTP_Chunk_Parse
(
    IN HTTP_CHUNK_HANDLE hParser,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *uiParsedLen
)
{
    HTTP_CHUNK_PARSE_S *pstChunk = NULL;
    BS_STATUS eResult = BS_OK;
    HTTP_CHUNK_STATUS_E enCurState = HTTP_CHUNK_BUTT;

    /* 入参判断 */
    if((NULL == hParser) || (NULL == pucData) || (NULL == uiParsedLen))
    {
        return BS_ERR;
    }

    *uiParsedLen = 0;

    /* 数据不全，需要进一步输入数据 */
    if(0 == uiDataLen)
    {
        return BS_NOT_COMPLETE;
    }    
  
    pstChunk = (HTTP_CHUNK_PARSE_S*)hParser;    

    pstChunk->uiDataLen = uiDataLen;
    pstChunk->pucData = pucData;

    do
    {
        enCurState = pstChunk->enCurState;
        switch(enCurState)
        {
            case HTTP_CHUNK_BEGIN_FLAG:
            {
                /* 解析一个chunk的开始标记 */
                eResult = http_chunk_ParseBegin(pstChunk);
                break;
            }
            case HTTP_CHUNK_BODY:
            {
                /* 解析一个chunk的体数据 */
                eResult = http_chunk_ParseBody(pstChunk);
                break;
            }
            case HTTP_CHUNK_END_FLAG:
            {
                /* 解析一个chunk的结束标记 */
                eResult = http_chunk_ParseEnd(pstChunk);
                break;
            }
            case HTTP_CHUNK_OVER_FLAG:
            {
                /* 解析所有chunk数据结束时的结尾标记 */
                eResult = http_chunk_ParseOver(pstChunk);
                break;
            }            
            default:
            {
                return BS_ERR;
            }
        }
    }while ((pstChunk->uiDataLen > 0) && (BS_NOT_COMPLETE == eResult));

    *uiParsedLen = uiDataLen - pstChunk->uiDataLen;

    return eResult;
}

