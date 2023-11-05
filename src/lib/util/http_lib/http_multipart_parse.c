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
    ULONG ulDataLen;               
}HTTP_MULTIPART_VBUF_S;

typedef enum tagHTTP_StatusMachine
{
    HTTP_PART_STATUS_DATA = 0,     
    HTTP_PART_STATUS_BOUNDARY,     
    HTTP_PART_STATUS_HEAD,         
    HTTP_PART_STATUS_END,          
}HTTP_STATUS_MACHINE_E;

typedef struct tagHTTP_MultipartData
{
    UCHAR *pucData;
    ULONG ulDataLen;
}HTTP_MULTIPART_DATA_S;

typedef struct tagHTTP_Multipart
{
    
    UCHAR aucBoundaryBuf[HTTP_MULTIPART_MAX_BOUNDARY_LEN]; 
    ULONG  ulBoundaryLen;                                  
    PF_HTTP_MULTIPART_FUNC pfCallback;                     
    VOID *pUserPointer;                                    
    
    HTTP_STATUS_MACHINE_E enStatus;                        
    HTTP_MULTIPART_VBUF_S stVBuf;

    HTTP_HEAD_PARSER hHttpHandle;
    
}HTTP_MULTIPART_S;




HTTP_MULTIPART_HANDLE HTTP_Multipart_CreateParser
(
    IN CHAR *pcBoundary, 
    IN PF_HTTP_MULTIPART_FUNC pfFunc, 
    IN VOID *pUserPointer
)
{
    HTTP_MULTIPART_S *pstMultiPart;
    ULONG ulBoundaryLen = 0;
    

    
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



STATIC BS_STATUS http_multipart_Notify(IN HTTP_MULTIPART_S *pstMultiParser, 
                                            IN HTTP_PART_TYPE_E eType, 
                                            IN UCHAR *pucData, 
                                            IN ULONG ulLen)
{
    
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


STATIC BS_STATUS http_multipart_ProcessData(IN HTTP_MULTIPART_S *pstMultiParser, IN HTTP_MULTIPART_DATA_S *pstMultiData)
{
    ULONG ulMinLen;
    ULONG ulBoundaryLen;
    ULONG ulSendLen;
    ULONG ulOffSet;
    CHAR *pcLocation;
    HTTP_MULTIPART_VBUF_S *pstVBuf = NULL;

    

    
    pstVBuf = &(pstMultiParser->stVBuf);
    ulMinLen = MIN (pstMultiData->ulDataLen, pstMultiParser->ulBoundaryLen - 1);
    memcpy(pstVBuf->aucRecBuf + pstVBuf->ulDataLen, pstMultiData->pucData, ulMinLen);
    pstVBuf->ulDataLen += ulMinLen;


    
    if (pstVBuf->ulDataLen < pstMultiParser->ulBoundaryLen)
    {
        if (pstMultiData->ulDataLen == ulMinLen)
        {
            pstMultiData->ulDataLen = 0;
        }
    }
    
    
    else
    {
        pcLocation = (CHAR*)MEM_Find((CHAR*)(pstVBuf->aucRecBuf), (size_t)(pstVBuf->ulDataLen), 
                                   (CHAR*)(pstMultiParser->aucBoundaryBuf),  
                                   (size_t)(pstMultiParser->ulBoundaryLen));                                  

        
        if (NULL == pcLocation)
        {
            ulSendLen = pstVBuf->ulDataLen - (pstMultiParser->ulBoundaryLen - 1);

            
            if (BS_OK != http_multipart_Notify(pstMultiParser, HTTP_PART_DATA_BODY, pstVBuf->aucRecBuf, ulSendLen))
            {
                return BS_ERR;
            }
            
            memmove(pstVBuf->aucRecBuf, pstVBuf->aucRecBuf + ulSendLen, pstMultiParser->ulBoundaryLen - 1);

            
            pstMultiParser->stVBuf.ulDataLen = pstMultiParser->ulBoundaryLen - 1;
            
            
            if (pstMultiData->ulDataLen < pstMultiParser->ulBoundaryLen)
            {
                pstMultiData->ulDataLen = 0;
                return BS_OK;
            }
        }
        
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

    
    if (pstMultiData->ulDataLen > (pstMultiParser->ulBoundaryLen -1))
    {
        pcLocation = (CHAR*)MEM_Find((CHAR*)(pstMultiData->pucData),
                                    (size_t)(pstMultiData->ulDataLen), 
                                    (CHAR*)(pstMultiParser->aucBoundaryBuf),  
                                    (size_t)(pstMultiParser->ulBoundaryLen));
        
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
            
            pstVBuf->ulDataLen = pstMultiParser->ulBoundaryLen - 1;
            pstMultiData->ulDataLen = 0;
            return BS_OK;
        }
        
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


STATIC BS_STATUS http_multipart_ProcessBoundary(IN HTTP_MULTIPART_S *pstMultiParser, IN HTTP_MULTIPART_DATA_S *pstMultiData)
{
    ULONG ulCopyLen;
    HTTP_MULTIPART_VBUF_S *pstVBuf = NULL;
    pstVBuf = &(pstMultiParser->stVBuf);

    
    ulCopyLen = MIN (2 - pstVBuf->ulDataLen, pstMultiData->ulDataLen);
    memcpy(pstVBuf->aucRecBuf + pstVBuf->ulDataLen, pstMultiData->pucData, ulCopyLen);
    pstVBuf->ulDataLen += ulCopyLen;
    pstMultiData->pucData += ulCopyLen;
    pstMultiData->ulDataLen -= ulCopyLen;        

    
    if (2 > pstVBuf->ulDataLen)
    {
        return BS_OK;
    }

    if ('-' == pstVBuf->aucRecBuf[0])
    {
        pstMultiParser->enStatus = HTTP_PART_STATUS_END;
    }
    
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


STATIC BS_STATUS http_multipart_ProcessHead(IN HTTP_MULTIPART_S *pstMultiParser, IN HTTP_MULTIPART_DATA_S *pstMultiData)
{
    

    ULONG ulCopyLen;
    ULONG ulPartHeaderLen;
    ULONG ulOffSet;
    HTTP_HEAD_PARSER hHttpHandle;
    CHAR *pcLocation;
    HTTP_MULTIPART_VBUF_S *pstVBuf = NULL;

    
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

        
        if (BS_OK != http_multipart_Notify(pstMultiParser, HTTP_PART_DATA_BEGIN, NULL, 0UL))
        {
            return BS_ERR;
        }
  
        pstVBuf->ulDataLen = 0;
        
        return BS_OK;
    }
        
}



BS_STATUS  HTTP_Multipart_Parse(IN HTTP_MULTIPART_HANDLE hMultiParser, IN UCHAR *pucData, IN ULONG ulDataLen)
{
    HTTP_MULTIPART_S *pstMultipart = NULL;
    ULONG ulRet = BS_OK;
    HTTP_MULTIPART_DATA_S stData;

    
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

    
    if ((BS_OK == ulRet) && (HTTP_PART_STATUS_END != pstMultipart->enStatus))
    {
        ulRet = BS_NOT_COMPLETE;
    }

    return ulRet;
}




