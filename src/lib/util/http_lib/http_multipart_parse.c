/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-21
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/num_utl.h"
#include "utl/txt_utl.h"
#include "utl/http_lib.h"

#define HTTP_MULTIPART_MAX_BOUNDARY_LEN           64UL
#define HTTP_MULTIPART_MAX_BUF_LEN               512UL


#define HTTP_MULTIPART_BOUNDARY_HEADER "--"
#define HTTP_MULTIPART_BOUNDARY_HEADER_LEN 2


typedef struct tagHTTP_MultipartVbuf
{
    UCHAR aucRecBuf[HTTP_MULTIPART_MAX_BUF_LEN];
    ULONG ulDataLen;               /* aucRecBuf中的数据长度 */
}HTTP_MULTIPART_VBUF_S;

typedef enum tagHTTP_StatusMachine
{
    HTTP_PART_STATUS_DATA = 0,     /* 解析体数据 */
    HTTP_PART_STATUS_BOUNDARY,     /* 解析BOUNDARY */
    HTTP_PART_STATUS_HEAD,         /* 解析请求头 */
    HTTP_PART_STATUS_END,          /* 解析结束BOUNDARY */
}HTTP_STATUS_MACHINE_E;

typedef struct tagHTTP_MultipartData
{
    UCHAR *pucData;
    ULONG ulDataLen;
}HTTP_MULTIPART_DATA_S;

typedef struct tagHTTP_Multipart
{
    /* Information from Invoker */
    UCHAR aucBoundaryBuf[HTTP_MULTIPART_MAX_BOUNDARY_LEN]; /* 分隔符字符串 */
    ULONG  ulBoundaryLen;                                  /* 分隔字符串长度 */
    PF_HTTP_MULTIPART_FUNC pfCallback;                     /* 回调函数 */
    VOID *pUserPointer;                                    /* 用户透传数据 */
    
    HTTP_STATUS_MACHINE_E enStatus;                        /* 解析过程的中间状态 */
    HTTP_MULTIPART_VBUF_S stVBuf;

    HTTP_HEAD_PARSER hHttpHandle;
    
}HTTP_MULTIPART_S;



/*******************************************************************************
  创建MultiPart句柄
*******************************************************************************/
HTTP_MULTIPART_HANDLE HTTP_Multipart_CreateParser
(
    IN CHAR *pcBoundary, 
    IN PF_HTTP_MULTIPART_FUNC pfFunc, 
    IN VOID *pUserPointer
)
{
    HTTP_MULTIPART_S *pstMultiPart;
    ULONG ulBoundaryLen = 0;
    

    /* 入参合法性检查，pUserPointer可以为NULL，由用户自己决定 */
    if((NULL == pcBoundary) || (NULL == pfFunc))
    {
        return NULL;
    }
    
    ulBoundaryLen = strlen(pcBoundary);
    if ((HTTP_MULTIPART_MAX_BOUNDARY_LEN
        - HTTP_LINE_END_FLAG_LEN
        - HTTP_MULTIPART_BOUNDARY_HEADER_LEN) <= ulBoundaryLen)
    {
        return NULL;
    }
    
    pstMultiPart = (HTTP_MULTIPART_S*)malloc(sizeof(HTTP_MULTIPART_S));
    if (NULL == pstMultiPart)
    {
        return NULL;
    }
    memset(pstMultiPart, 0, sizeof(HTTP_MULTIPART_S));    
    
    memcpy(pstMultiPart->aucBoundaryBuf, HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);
    memcpy(pstMultiPart->aucBoundaryBuf + HTTP_LINE_END_FLAG_LEN, HTTP_MULTIPART_BOUNDARY_HEADER, HTTP_MULTIPART_BOUNDARY_HEADER_LEN);
    memcpy(pstMultiPart->aucBoundaryBuf + HTTP_LINE_END_FLAG_LEN + HTTP_MULTIPART_BOUNDARY_HEADER_LEN, pcBoundary, ulBoundaryLen);
    pstMultiPart->ulBoundaryLen = ulBoundaryLen + HTTP_LINE_END_FLAG_LEN + HTTP_MULTIPART_BOUNDARY_HEADER_LEN;

    pstMultiPart->pfCallback = pfFunc;
    pstMultiPart->pUserPointer = pUserPointer;
    pstMultiPart->enStatus = HTTP_PART_STATUS_DATA;
    pstMultiPart->hHttpHandle = NULL;

    memcpy(pstMultiPart->stVBuf.aucRecBuf, HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);
    pstMultiPart->stVBuf.ulDataLen = HTTP_LINE_END_FLAG_LEN;
    
    return pstMultiPart;
    
}


/*******************************************************************************
  销毁MultiPart句柄
*******************************************************************************/
VOID HTTP_Multipart_DestoryParser(IN HTTP_MULTIPART_HANDLE hMultiParser)
{
    HTTP_MULTIPART_S *pstMultipart;

    pstMultipart = (HTTP_MULTIPART_S*)hMultiParser;
    if (NULL == pstMultipart)
    {
        return;
    }
    if (NULL != pstMultipart->hHttpHandle)
    {
        HTTP_DestoryHeadParser(pstMultipart->hHttpHandle);
    }
    free(pstMultipart);
    return;
}


/*******************************************************************************
  返回给用户当前的处理状态，以及处理的数据和长度
*******************************************************************************/
STATIC BS_STATUS http_multipart_Notify(IN HTTP_MULTIPART_S *pstMultiParser, 
                                            IN HTTP_PART_TYPE_E eType, 
                                            IN UCHAR *pucData, 
                                            IN ULONG ulLen)
{
    /* 入参合法性判断 */
    BS_DBGASSERT(NULL != pstMultiParser);
    BS_DBGASSERT(NULL != pstMultiParser->pfCallback);
    
    if (NULL != pstMultiParser->hHttpHandle)
    {
        return pstMultiParser->pfCallback(eType,
                                          pstMultiParser,
                                          pstMultiParser->hHttpHandle,
                                          pucData,
                                          ulLen,
                                          pstMultiParser->pUserPointer);
    }
    return BS_OK;
}

/*******************************************************************************
  处理MultiPart体数据状态                                                        
*******************************************************************************/
STATIC BS_STATUS http_multipart_ProcessData(IN HTTP_MULTIPART_S *pstMultiParser, IN HTTP_MULTIPART_DATA_S *pstMultiData)
{
    ULONG ulMinLen;
    ULONG ulBoundaryLen;
    ULONG ulSendLen;
    ULONG ulOffSet;
    CHAR *pcLocation;
    HTTP_MULTIPART_VBUF_S *pstVBuf = NULL;

    /* 
        如果当前缓冲区有数据，说明已经在找第二个以后的boundary,可能一部分Boundary在前面数据
        的结束处，正保存在缓冲区中
        1. 最多向缓冲区中拷贝Boundarylen -1 个字节，来确定上一次保存在缓冲区的数据是否有部分
        是boundary的。
            1) 如果缓冲区当前数据小于Boundary  长度，直接返回
            2) 如果缓冲区的长度足够做一次匹配，在缓冲区中匹配Boundary
                a) 如果匹配到了，将Boundary之前的数据发给用户，缓冲区清零，切换状态到Boundary
                b) 如果没匹配到，将缓冲区中除最后Boundary -1 的字节全部交给用户
                    i) 如果数据长度小于Boundary -1,将缓冲区数据左移到缓冲区头
                    ii) 如果数据长度大于Boundary -1, 则说明上一次保存在缓冲区的数据已经都发出去了，这
                    时我们不使用我们的缓冲区了，
                        而是直接从用户传进来的内存做匹配
                            x)  如果没匹配到，拷贝最后Boundary -1 个字节到缓冲区，其它的发出去
                           xx)  如果匹配到了， 将前部分数据交给用户，切换到Boundary状态，我们的缓冲区清零
    */

    /* 
        首先不管怎么样，先拷贝最多Boundary -1 个字节到我们的缓冲区，因为只要这Boundary -1 个字
        节就可以判断出当前缓冲区的数据是否是Boundary 的一部分
    */
    pstVBuf = &(pstMultiParser->stVBuf);
    ulMinLen = MIN (pstMultiData->ulDataLen, pstMultiParser->ulBoundaryLen - 1);
    memcpy(pstVBuf->aucRecBuf + pstVBuf->ulDataLen, pstMultiData->pucData, ulMinLen);
    pstVBuf->ulDataLen += ulMinLen;


    /* 
        1、如果缓冲区的数据仍旧小于Boundary 的长度，那没必要匹配缓冲区了
        2、如果缓冲区的数据大于Boundary 的长度，有可能存在Boundary ，在我们缓冲区中数据匹配BOUNDARY
    */
    if (pstVBuf->ulDataLen < pstMultiParser->ulBoundaryLen)
    {
        if (pstMultiData->ulDataLen == ulMinLen)
        {
            pstMultiData->ulDataLen = 0;
        }
    }
    
    /* 在我们缓冲区中数据匹配BOUNDARY */
    else
    {
        pcLocation = (CHAR*)MEM_Find((CHAR*)(pstVBuf->aucRecBuf), (size_t)(pstVBuf->ulDataLen), 
                                   (CHAR*)(pstMultiParser->aucBoundaryBuf),  
                                   (size_t)(pstMultiParser->ulBoundaryLen));                                  

        /* 
            如果我们的缓冲区没有匹配到Boundary, 将我们缓冲区的最后Boundary -1 个字节留下，
            做下一次的匹配，之前的字节交给用户,而且这里要判断 */
        if (NULL == pcLocation)
        {
            ulSendLen = pstVBuf->ulDataLen - (pstMultiParser->ulBoundaryLen - 1);

            /* 
                pstMultiParser->hHttpHandle只有在解析完Part Header 的时候才被创建
                以此来判断是否是第一个Boundary
            */
            if (BS_OK != http_multipart_Notify(pstMultiParser, HTTP_PART_DATA_BODY, pstVBuf->aucRecBuf, ulSendLen))
            {
                return BS_ERR;
            }
            /* 将我们缓冲区中后面的Boundary -1个字节留下来，其它发走，并移到缓冲区的头部 */
            memmove(pstVBuf->aucRecBuf, pstVBuf->aucRecBuf + ulSendLen, pstMultiParser->ulBoundaryLen - 1);

            /* 更新缓冲区长度 */
            pstMultiParser->stVBuf.ulDataLen = pstMultiParser->ulBoundaryLen - 1;
            
            /* 如果用户传进来的数据小于Boundary 的长度，则全部用户数据都已经拷贝到了我们的缓冲区，
            用户缓冲区已经可以清零了 */
            if (pstMultiData->ulDataLen < pstMultiParser->ulBoundaryLen)
            {
                pstMultiData->ulDataLen = 0;
                return BS_OK;
            }
        }
        /*  
            如果匹配到了，将我们缓冲区之前的内容交给用户，切换到Boundary 状态
            用户缓冲区右移，如: Boundary 为183a，我们缓冲区当前为[ c 1 8 3] 用户缓冲区为[ a \r \n q]
            最多拷贝3个字节到我们的缓冲区，这时我们的缓冲区为[ c 1 8 3 a \r \n]
            那么我们需要将c 交给用户，用户缓冲区的指针需要移动到\r, 这个长度＝(c) + (183a) -(c183)
            
        */
        else
        {
            ulBoundaryLen = (ULONG)pcLocation - (ULONG)(pstVBuf->aucRecBuf);
            if (BS_OK != http_multipart_Notify(pstMultiParser, HTTP_PART_DATA_BODY, pstVBuf->aucRecBuf, ulBoundaryLen))
            {
                return BS_ERR;
            }
            
            ulOffSet = (ulBoundaryLen + pstMultiParser->ulBoundaryLen) - (pstVBuf->ulDataLen - ulMinLen);
            pstMultiData->pucData += ulOffSet;
            memset(pstVBuf, 0, sizeof(HTTP_MULTIPART_VBUF_S));
            pstMultiData->ulDataLen -= ulOffSet;
            pstMultiParser->enStatus = HTTP_PART_STATUS_BOUNDARY;
            
            return BS_OK;
        }
    }

    /* 
        如果我们的缓冲区中没有匹配到Boundary, 而且用户数据长度大于Boundary -1 
        说明之前我们缓冲区的数据已经都交给用户了，当前我们缓冲区中的数据就是用户缓冲
        区的前Boundary -1 个字节，所以可以直接到用户缓冲区中做匹配
    */
    if (pstMultiData->ulDataLen > (pstMultiParser->ulBoundaryLen -1))
    {
        pcLocation = (CHAR*)MEM_Find((CHAR*)(pstMultiData->pucData),
                                    (size_t)(pstMultiData->ulDataLen), 
                                    (CHAR*)(pstMultiParser->aucBoundaryBuf),  
                                    (size_t)(pstMultiParser->ulBoundaryLen));
        /* 
            如果没有匹配到，将最后的Boundary -1 个字节拷贝到我们的缓冲区，等待下一次匹配，
            用户缓冲区已经处理完毕，可以释放
        */
        if (NULL == pcLocation)
        {
            if (BS_OK != http_multipart_Notify(pstMultiParser, 
                                                           HTTP_PART_DATA_BODY, 
                                                           pstMultiData->pucData, 
                                                           pstMultiData->ulDataLen - (pstMultiParser->ulBoundaryLen - 1)))
            {
                return BS_ERR;
            }
            memcpy(pstVBuf->aucRecBuf, 
                   (pstMultiData->pucData + pstMultiData->ulDataLen) - (pstMultiParser->ulBoundaryLen - 1), 
                   pstMultiParser->ulBoundaryLen - 1);
            /* 更新缓冲区长度 */
            pstVBuf->ulDataLen = pstMultiParser->ulBoundaryLen - 1;
            pstMultiData->ulDataLen = 0;
            return BS_OK;
        }
        /* 如果在用户的内存中匹配到Boundary，则Boundary前面的字节为数据传给用户 */
        else
        {
            ulBoundaryLen = (ULONG)pcLocation - (ULONG)(pstMultiData->pucData);
            if (BS_OK != http_multipart_Notify(pstMultiParser, 
                                                          HTTP_PART_DATA_BODY, 
                                                          pstMultiData->pucData, 
                                                          ulBoundaryLen))
            {
                return BS_ERR;
            }
            pstMultiParser->enStatus = HTTP_PART_STATUS_BOUNDARY;
            
            memset(pstVBuf, 0, sizeof(HTTP_MULTIPART_VBUF_S));
            pstMultiData->pucData += ulBoundaryLen;
            pstMultiData->pucData += pstMultiParser->ulBoundaryLen;
            pstMultiData->ulDataLen -= (ulBoundaryLen + pstMultiParser->ulBoundaryLen);
            return BS_OK;
        }
    }

    return BS_OK;
    
}

/*******************************************************************************
  处理MultiPart Boundary数据状态                                                        
*******************************************************************************/
STATIC BS_STATUS http_multipart_ProcessBoundary(IN HTTP_MULTIPART_S *pstMultiParser, IN HTTP_MULTIPART_DATA_S *pstMultiData)
{
    ULONG ulCopyLen;
    HTTP_MULTIPART_VBUF_S *pstVBuf = NULL;
    pstVBuf = &(pstMultiParser->stVBuf);

    /* 拷贝两个字节到缓冲区中 */
    ulCopyLen = MIN (2 - pstVBuf->ulDataLen, pstMultiData->ulDataLen);
    memcpy(pstVBuf->aucRecBuf + pstVBuf->ulDataLen, pstMultiData->pucData, ulCopyLen);
    pstVBuf->ulDataLen += ulCopyLen;
    pstMultiData->pucData += ulCopyLen;
    pstMultiData->ulDataLen -= ulCopyLen;        

    /* 如果缓冲区中数据不足两个，用户内存指针不方便后移，等待下次继续来数据 */
    if (2 > pstVBuf->ulDataLen)
    {
        return BS_OK;
    }

    if ('-' == pstVBuf->aucRecBuf[0])
    {
        pstMultiParser->enStatus = HTTP_PART_STATUS_END;
    }
    /* 这是我们只判断首字符，如果是\r认为Boundary  结束 */
    else if ('\r' ==pstVBuf->aucRecBuf[0])
    {
        pstMultiParser->enStatus = HTTP_PART_STATUS_HEAD;
    }
    else
    {
        return BS_ERR;
    }

    pstVBuf->ulDataLen = 0;
    if (BS_OK != http_multipart_Notify(pstMultiParser, HTTP_PART_DATA_END, NULL, 0UL))
    {
        return BS_ERR;
    }
    
    if (NULL != pstMultiParser->hHttpHandle)
    {
        HTTP_DestoryHeadParser(pstMultiParser->hHttpHandle);
        pstMultiParser->hHttpHandle = NULL;
    }
    return BS_OK;

}

/*******************************************************************************
  处理MultiPart Head数据状态                                                        
*******************************************************************************/
STATIC BS_STATUS http_multipart_ProcessHead(IN HTTP_MULTIPART_S *pstMultiParser, IN HTTP_MULTIPART_DATA_S *pstMultiData)
{
    /* 
        解析PartHeader  就是找 \r\n\r\n ,此时我们的缓冲区为空 
        将数据拷贝到我们的缓冲区，匹配\r\n\r\n
    */

    ULONG ulCopyLen;
    ULONG ulPartHeaderLen;
    ULONG ulOffSet;
    HTTP_HEAD_PARSER hHttpHandle;
    CHAR *pcLocation;
    HTTP_MULTIPART_VBUF_S *pstVBuf = NULL;

    /* 拷贝最小长度，取当前数据和我们缓冲区的最小值 */
    pstVBuf = &(pstMultiParser->stVBuf);
    ulCopyLen = MIN(pstMultiData->ulDataLen, HTTP_MULTIPART_MAX_BUF_LEN - pstVBuf->ulDataLen);
    memcpy(pstVBuf->aucRecBuf + pstVBuf->ulDataLen, 
           pstMultiData->pucData, 
           ulCopyLen);
    
    pstVBuf->ulDataLen += ulCopyLen;
    
    if (HTTP_CRLFCRLF_LEN > pstVBuf->ulDataLen)
    {
        pstMultiData->ulDataLen = 0;
        return BS_OK;
    }
    pcLocation = (CHAR*)MEM_Find((CHAR*)(pstVBuf->aucRecBuf), pstVBuf->ulDataLen, HTTP_CRLFCRLF, HTTP_CRLFCRLF_LEN);

    if (NULL == pcLocation)
    {
        if (pstVBuf->ulDataLen == HTTP_MULTIPART_MAX_BUF_LEN)
        {
            return BS_ERR;
        }
        pstMultiData->ulDataLen = 0; 
        return BS_OK;
    }
    else
    {
        ulPartHeaderLen = (ULONG)pcLocation - (ULONG)(pstVBuf->aucRecBuf);
        ulOffSet = (ulPartHeaderLen + HTTP_CRLFCRLF_LEN) - (pstVBuf->ulDataLen - ulCopyLen);
        pstMultiData->pucData += ulOffSet;
        pstMultiData->ulDataLen -= ulOffSet;
        pstMultiParser->enStatus = HTTP_PART_STATUS_DATA;

        /* 开始解析Part Header */
        hHttpHandle = HTTP_CreateHeadParser();
        if (NULL == hHttpHandle)
        {
            return BS_ERR;
        }
        pstMultiParser->hHttpHandle = hHttpHandle;
        if (BS_OK != HTTP_ParseHead(hHttpHandle, (CHAR*)(pstVBuf->aucRecBuf),  ulPartHeaderLen, HTTP_PART_HEADER))
        {
            return BS_ERR;
        }

        /* 找到Part Header 之后标识数据真正开始 */
        if (BS_OK != http_multipart_Notify(pstMultiParser, HTTP_PART_DATA_BEGIN, NULL, 0UL))
        {
            return BS_ERR;
        }
  
        pstVBuf->ulDataLen = 0;
        
        return BS_OK;
    }
        
}


/*******************************************************************************
  处理MultiPart,状态机工作函数                                                
*******************************************************************************/
BS_STATUS  HTTP_Multipart_Parse(IN HTTP_MULTIPART_HANDLE hMultiParser, IN UCHAR *pucData, IN ULONG ulDataLen)
{
    HTTP_MULTIPART_S *pstMultipart = NULL;
    ULONG ulRet = BS_OK;
    HTTP_MULTIPART_DATA_S stData;

    /* 入参合法性检查 */
    if((NULL == hMultiParser) || (NULL == pucData))
    {
        return BS_ERR;
    }

    pstMultipart = (HTTP_MULTIPART_S*)hMultiParser;

    memset(&stData, 0, sizeof(HTTP_MULTIPART_DATA_S));

    stData.pucData = pucData;
    stData.ulDataLen = ulDataLen;

    do
    {
        switch(pstMultipart->enStatus)
        {
            case HTTP_PART_STATUS_DATA:
            {
                ulRet = http_multipart_ProcessData(pstMultipart, &stData);
                break;
            }
            case HTTP_PART_STATUS_BOUNDARY:
            {
                ulRet = http_multipart_ProcessBoundary(pstMultipart, &stData);
                break;
            }
            case HTTP_PART_STATUS_HEAD:
            {
                ulRet = http_multipart_ProcessHead(pstMultipart, &stData);
                break;
            }
            default:
            {
                break;
            }
        }
    }  while ((BS_OK == ulRet) &&  (HTTP_PART_STATUS_END != pstMultipart->enStatus) && (0 != stData.ulDataLen));

    /* 在没有解析完成的情况下,成功则返回INCOMPLETE */
    if ((BS_OK == ulRet) && (HTTP_PART_STATUS_END != pstMultipart->enStatus))
    {
        ulRet = BS_NOT_COMPLETE;
    }

    return ulRet;
}




