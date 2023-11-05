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
    HTTP_CHUNK_BEGIN_FLAG,      
    HTTP_CHUNK_BODY,            
    HTTP_CHUNK_END_FLAG,        
    HTTP_CHUNK_OVER_FLAG,       
    HTTP_CHUNK_BUTT             
}HTTP_CHUNK_STATUS_E;

typedef struct tagHTTP_ChunkParse
{
    PF_HTTP_CHUNK_FUNC pfFunc;            
    USER_HANDLE_S stUserHandle;           
    UCHAR *pucData;                       
    UINT uiDataLen;                      
    HTTP_CHUNK_STATUS_E enCurState;       
    UINT uiOverFlag;                     
    UINT uiResLength;                    
    UINT uiLastDataLen;                  
    UCHAR aucChunkBegin[HTTP_CHUNK_FLAG_MAX_SIZE]; 
}HTTP_CHUNK_PARSE_S;





VOID HTTP_Chunk_BuildChunkFlag(IN UINT64 ui64DataLen, OUT CHAR szChunkBeginFlag[HTTP_CHUNK_FLAG_MAX_LEN + 1])
{
    (VOID)scnprintf(szChunkBeginFlag, HTTP_CHUNK_FLAG_MAX_LEN + 1, "%llx\r\n", ui64DataLen);

    return;
}



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
    ULONG ulCRLFLen = 0;                    
    ULONG ulLen = 0;    

    
    if((NULL == pucData) || (0 == ulDataLen) || (NULL == pulChunkDataLen) || (NULL == pulChunkFlagLen))
    {
        return BS_ERR;
    }

    
    pucCRLF = (UCHAR*)MEM_Find((VOID*)pucData, ulDataLen, HTTP_LINE_END_FLAG, HTTP_LINE_END_FLAG_LEN);
    if (NULL == pucCRLF)
    {
        if(ulDataLen < HTTP_CHUNK_FLAG_MAX_SIZE)
        {
            return BS_NOT_COMPLETE;
        }
        
        return BS_ERR;
    }
    ulCRLFLen = (ULONG)pucCRLF - (ULONG)pucData;

    
    pucSem = (UCHAR*)memchr((VOID*)pucData, HTTP_SEMICOLON_CHAR, ulCRLFLen);
    
    if (NULL == pucSem)
    {
        ulLen = ulCRLFLen;
    }
    else
    {
        ulLen = (ULONG)pucSem - (ULONG)pucData;
    }

    
    if (ulLen > HTTP_MAX_UINT_HEX_LEN)
    {
        return BS_ERR;  
    }

    
    if (TRUE != http_chunk_CheckHex(pucData, ulLen))
    {
        return BS_ERR;
    }    

    
    memcpy(szSize, pucData, ulLen);
    szSize[ulLen] = '\0';
    ulTemp = (ULONG)strtol(szSize, (CHAR **)NULL, 16);
    *pulChunkDataLen = (ULONG)ulTemp;
    *pulChunkFlagLen = ulCRLFLen + HTTP_LINE_END_FLAG_LEN;

    return BS_OK;
}


HTTP_CHUNK_HANDLE HTTP_Chunk_CreateParser(IN PF_HTTP_CHUNK_FUNC pfFunc,IN USER_HANDLE_S *pstUserPointer)
{ 
    HTTP_CHUNK_PARSE_S *pstChunk = NULL;

    
    if(NULL == pfFunc)
    {
        return NULL;
    }
    
    pstChunk = (HTTP_CHUNK_PARSE_S *)malloc(sizeof(HTTP_CHUNK_PARSE_S));
    if(pstChunk == NULL)
    {
        return NULL;
    }
    
    memset(pstChunk, 0, sizeof(HTTP_CHUNK_PARSE_S));
    
    if (NULL != pstUserPointer)
    {
        pstChunk->stUserHandle = *pstUserPointer;
    }
    pstChunk->pfFunc = pfFunc;
    pstChunk->enCurState = HTTP_CHUNK_BEGIN_FLAG;

    return pstChunk;
}


VOID HTTP_Chunk_DestoryParser(IN HTTP_CHUNK_HANDLE hParser)
{
    if(NULL != hParser)
    {
        free(hParser);            
    }
    return ;
}


STATIC BS_STATUS http_chunk_ParseBegin(IN HTTP_CHUNK_PARSE_S *pstChunk)
{
    ULONG ulChunkDataLen = 0;        
    UINT uiCopyLen = 0;             
    UINT uiOldDataLen;              
    ULONG ulChunkFlagLen;
    BS_STATUS eRet;
    
    
    BS_DBGASSERT(NULL != pstChunk);
    BS_DBGASSERT(0 != pstChunk->uiDataLen);

    
    uiOldDataLen = pstChunk->uiLastDataLen;
    uiCopyLen = HTTP_CHUNK_FLAG_MAX_SIZE - uiOldDataLen;
    uiCopyLen = MIN(uiCopyLen, pstChunk->uiDataLen);

    
    memcpy((CHAR *)pstChunk->aucChunkBegin + uiOldDataLen, (CHAR *)pstChunk->pucData, (size_t)uiCopyLen);

    
    pstChunk->uiLastDataLen += uiCopyLen;  

    
    eRet = HTTP_Chunk_GetChunkDataLen(pstChunk->aucChunkBegin, pstChunk->uiLastDataLen, &ulChunkDataLen, &ulChunkFlagLen);
    if (BS_OK != eRet)
    {
        pstChunk->uiDataLen -= uiCopyLen;
        return eRet;
    }

    
    CHUNK_UPDATE_STATUS(pstChunk, (ulChunkFlagLen - uiOldDataLen));

    
    pstChunk->uiResLength = ulChunkDataLen;

    
    memset(pstChunk->aucChunkBegin, 0, HTTP_CHUNK_FLAG_MAX_SIZE);
    pstChunk->uiLastDataLen = 0;

    
    if (0 == ulChunkDataLen)
    {
        pstChunk->enCurState = HTTP_CHUNK_OVER_FLAG;
        pstChunk->uiOverFlag = HTTP_LINE_END_FLAG_LEN;
    }
    else
    {
        
        pstChunk->enCurState = HTTP_CHUNK_BODY;
    }

    return BS_NOT_COMPLETE;
}


STATIC BS_STATUS http_chunk_ParseBody(IN HTTP_CHUNK_PARSE_S *pstChunk)
{
    UINT uiBodyLen = 0;  
    UCHAR *pucDataBody = NULL;
    
    
    BS_DBGASSERT(NULL != pstChunk);

    if(0 == pstChunk->uiDataLen)
    {
        return BS_NOT_COMPLETE;
    }      
    
    pucDataBody = pstChunk->pucData;

    
    uiBodyLen = MIN(pstChunk->uiDataLen, pstChunk->uiResLength);
    if (uiBodyLen > 0)
    {
        
        if (BS_OK != pstChunk->pfFunc(pucDataBody, uiBodyLen, &pstChunk->stUserHandle))
        {
            return BS_ERR;
        }
        
        CHUNK_UPDATE_STATUS(pstChunk, uiBodyLen);
        pstChunk->uiResLength -= uiBodyLen;
    }

    if (0 == pstChunk->uiResLength)
    {
        
        pstChunk->uiResLength = HTTP_LINE_END_FLAG_LEN;
        pstChunk->enCurState = HTTP_CHUNK_END_FLAG;
    }

    return BS_NOT_COMPLETE;
}


STATIC BS_STATUS http_chunk_ParseEnd(IN HTTP_CHUNK_PARSE_S *pstChunk)
{
    UINT uiLen;
    
    
    BS_DBGASSERT(NULL != pstChunk);

    if(0 == pstChunk->uiDataLen)
    {
        return BS_NOT_COMPLETE;
    }

    
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


STATIC BS_STATUS http_chunk_ParseOver(IN HTTP_CHUNK_PARSE_S *pstChunk)
{
    UINT i = 0;
    UCHAR ucChar;
    CHAR *pcOverFlag = HTTP_CRLFCRLF;
    BS_STATUS eRet = BS_NOT_COMPLETE;
    
    
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
            
            (VOID)pstChunk->pfFunc(NULL, (ULONG)0, &pstChunk->stUserHandle);
            eRet = BS_OK;
            break;
        }
        i++;
    }

    CHUNK_UPDATE_STATUS(pstChunk, pstChunk->uiDataLen);
    
    return eRet;
}


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

    
    if((NULL == hParser) || (NULL == pucData) || (NULL == uiParsedLen))
    {
        return BS_ERR;
    }

    *uiParsedLen = 0;

    
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
                
                eResult = http_chunk_ParseBegin(pstChunk);
                break;
            }
            case HTTP_CHUNK_BODY:
            {
                
                eResult = http_chunk_ParseBody(pstChunk);
                break;
            }
            case HTTP_CHUNK_END_FLAG:
            {
                
                eResult = http_chunk_ParseEnd(pstChunk);
                break;
            }
            case HTTP_CHUNK_OVER_FLAG:
            {
                
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

