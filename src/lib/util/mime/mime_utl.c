/******************************************************************************
* Copyright (C), LiXingang
* Author:      lixingang  Version: 1.0  Date: 2007-2-8
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/mime_utl.h"
#include "utl/http_lib.h"


typedef CHAR* (*DECODE_FUNC_PF)(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen);

static CHAR * mime_DftDecode(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen)
{
    CHAR *pcBuf;

    pcBuf = (CHAR *)MEMPOOL_Alloc(pstList->hMemPool, ulDataLen + 1 );
    if( NULL == pcBuf )
    {
        return NULL;
    }

    TXT_Strlcpy(pcBuf, pcData, ulDataLen + 1);

    return pcBuf;
}


MIME_HANDLE MIME_Create (VOID)
{
    
    MIME_DATALIST_S *pstMimeHandle = NULL;
    
    
    pstMimeHandle = (MIME_DATALIST_S*)MEM_ZMalloc(sizeof(MIME_DATALIST_S));
    if (NULL == pstMimeHandle)
    {
        return NULL;
    }

    pstMimeHandle->hMemPool = MEMPOOL_Create(0);
    if (NULL == pstMimeHandle->hMemPool)
    {
        MEM_Free(pstMimeHandle);
        return NULL;
    }
    
    
    DLL_INIT(&(pstMimeHandle->stDataList));
    
    return ((MIME_HANDLE)pstMimeHandle);
}


VOID MIME_Destroy (IN MIME_HANDLE hMimeHandle)
{
    MIME_DATALIST_S* pstParamListHead;

    
    if (NULL == hMimeHandle)
    {
        return;
    }
    
    pstParamListHead = (MIME_DATALIST_S*)hMimeHandle;

    if (NULL != pstParamListHead->hMemPool)
    {
        MEMPOOL_Destory(pstParamListHead->hMemPool);
        pstParamListHead->hMemPool = NULL;
    }

    MEM_Free(pstParamListHead);

    return;
}


STATIC CHAR* mime_ParamDecode(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen)
{
    
    BS_DBGASSERT(NULL != pcData);
    
    return HTTP_UriDecode(pstList->hMemPool, pcData, ulDataLen);
}



STATIC CHAR * mime_DataDecode(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen)
{
    
    BS_DBGASSERT(NULL != pcData);
    
    return HTTP_DataDecode(pstList->hMemPool, pcData, ulDataLen);
}



STATIC CHAR * mime_CookieDecode(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen)
{
    CHAR *pcOutData;
    
    
    BS_DBGASSERT(NULL != pcData);

    pcOutData = MEMPOOL_Alloc(pstList->hMemPool, ulDataLen + 1);
    if (NULL == pcOutData)
    {
        return NULL;
    }
    memcpy(pcOutData, pcData, ulDataLen);
    pcOutData[ulDataLen] = '\0';

    return pcOutData;
}


STATIC CHAR * mime_ContentDisposDecode(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen)
{
    ULONG ulNewLen = 0;
    CHAR *pcTemp;
    CHAR *pcDecode;

	
    BS_DBGASSERT(NULL != pcData);

    
    pcTemp = HTTP_Strim(pcData, ulDataLen, HTTP_HEAD_DOUBLE_QUOTATION_STRING, &ulNewLen);

    pcDecode = MEMPOOL_Alloc(pstList->hMemPool, ulNewLen + 1);
    if (NULL == pcDecode)
    {
        return NULL;
    }
    if (0 != ulNewLen)
    {
        memcpy(pcDecode, pcTemp, ulNewLen);
    }
    pcDecode[ulNewLen] = '\0';
    return pcDecode;
}


STATIC CHAR* mime_CreateKeyValue(IN MIME_DATALIST_S *pstList, IN CHAR *pcValue, IN ULONG ulValueLen, IN DECODE_FUNC_PF pfDecodeFunc)
{
    CHAR *pcKeyValue = NULL;

	
    BS_DBGASSERT(NULL != pfDecodeFunc);

	if ((NULL == pcValue) || (0 == ulValueLen))
    {
        pcKeyValue = MEMPOOL_Alloc(pstList->hMemPool, ulValueLen + 1);
        if (NULL != pcKeyValue)
        {
            pcKeyValue[0] = '\0';
        } 
    }
    else
    {
        pcKeyValue = pfDecodeFunc(pstList, pcValue, ulValueLen);   
    }
    return pcKeyValue;
}


STATIC MIME_DATA_NODE_S * mime_CreatDecodeNode
(
    IN MIME_DATALIST_S *pstList,
    IN CHAR *pcKeyName, 
    IN ULONG ulKeyLen, 
    IN CHAR *pcValue, 
    IN ULONG ulValueLen,
    IN DECODE_FUNC_PF pfDecodeFunc
)
{
    CHAR *pcKey = NULL;
    CHAR *pcNewValue = NULL;
    MIME_DATA_NODE_S *pstNode = NULL;

	
    BS_DBGASSERT(NULL != pcKeyName);
    BS_DBGASSERT(0 != ulKeyLen);

    
    pstNode = (MIME_DATA_NODE_S *)MEMPOOL_ZAlloc(pstList->hMemPool, sizeof(MIME_DATA_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }
    
    
    pcKey = pfDecodeFunc(pstList, pcKeyName, ulKeyLen); 
    if (! pcKey) {
        MEMPOOL_Free(pstList->hMemPool, pstNode);
        return NULL;
    }
    pcNewValue = mime_CreateKeyValue(pstList, pcValue, ulValueLen, pfDecodeFunc);   
    if (! pcNewValue) {
        MEMPOOL_Free(pstList->hMemPool, pstNode);
        MEMPOOL_Free(pstList->hMemPool, pcKey);
        return NULL;
    }
    
    pstNode->pcKey   = pcKey;
    pstNode->pcValue = pcNewValue;

    return pstNode;
}



STATIC BS_STATUS mime_AddNode
(
    IN MIME_DATALIST_S *pstList,
    IN CHAR *pcKey,
    IN ULONG ulKeyLen,
    IN CHAR *pcValue,
    IN ULONG ulValueLen,
    IN DECODE_FUNC_PF pfDecodeFunc
)
{
    
    CHAR *pcNewKey;
    ULONG ulNewKeyLen = 0;
    CHAR *pcNewValue;
    ULONG ulNewValueLen = 0;
    MIME_DATA_NODE_S *pstDataNode = NULL;

    
    BS_DBGASSERT(NULL != pstList);
    BS_DBGASSERT(NULL != pcKey);

    if (NULL == pfDecodeFunc)
    {
        pfDecodeFunc = mime_DftDecode;
    }

    
    pcNewKey = HTTP_Strim(pcKey, ulKeyLen, HTTP_SP_HT_STRING, &ulNewKeyLen);
    if( (NULL == pcNewKey)||(0 == ulNewKeyLen) )
    {
        
        
        return BS_OK;
    }

    
    pcNewValue = pcValue;
    ulNewValueLen = ulValueLen;
    
    if((NULL != pcValue)&&(0 != ulValueLen))
    {
        
        pcNewValue = HTTP_Strim(pcValue, ulValueLen, HTTP_SP_HT_STRING, &ulNewValueLen);
    }

    
    pstDataNode = mime_CreatDecodeNode(pstList, pcNewKey, ulNewKeyLen, pcNewValue, ulNewValueLen, pfDecodeFunc);
    if (NULL == pstDataNode)
    {
        return BS_ERR;
    }


    
    DLL_ADD(&(pstList->stDataList), (DLL_NODE_S*)pstDataNode);

    return BS_OK;
}


STATIC BS_STATUS mime_Parse
(
    IN MIME_HANDLE hMimeHandle, 
    IN CHAR *pcData, 
    IN CHAR cSeparator, 
    IN DECODE_FUNC_PF pfDecodeFunc
)
{
    
    CHAR *pcCur = NULL;
    CHAR *pcBeg = NULL;
    CHAR *pcEnd = NULL;
    CHAR *pcEqual = NULL;
    CHAR *pcKey   = NULL;
    CHAR *pcPara  = NULL;    

    ULONG ulKeyLen  = 0;
    ULONG ulParaLen = 0;
    ULONG ulLength =  0;

    MIME_DATALIST_S *pstList = NULL;
    
    
    BS_DBGASSERT(NULL != hMimeHandle);
    BS_DBGASSERT(NULL != pcData);
    
    
    ulLength = strlen(pcData);
    pcBeg = pcData;
    pcEnd = pcData + ulLength;

    
    
    pcKey = pcBeg;
    pstList = (MIME_DATALIST_S *)hMimeHandle;

    
    
    for (pcCur = pcBeg; pcCur < pcEnd; pcCur++)
    {   
        
        if (HTTP_EQUAL_CHAR == *pcCur)
        {
                        
            if(NULL == pcEqual)
            {
                pcEqual = pcCur;
            }
        }

        
        else if (cSeparator == *pcCur)
        {   
            
            
            if (pcKey == pcEqual)
            {
                pcKey = pcCur + 1;
                pcEqual = NULL;
                continue;
            }
            
            if ((NULL != pcEqual)&&(pcEqual > pcKey)&&(pcEqual < pcCur))
            {
                
                ulKeyLen =(ULONG)pcEqual - (ULONG)pcKey;
                pcPara   = (pcEqual + 1);
                ulParaLen = (ULONG)pcCur - (ULONG)(pcEqual + 1);
            }
            else
            {
                
                     
                ulKeyLen = (ULONG)pcCur - (ULONG)pcKey;
                pcPara = NULL;
                ulParaLen = 0;
            }
            

            
            if (BS_OK != mime_AddNode(pstList, pcKey, ulKeyLen, pcPara, ulParaLen, pfDecodeFunc))
            {
                return BS_ERR;
            }

            
            pcKey = pcCur + 1;
            pcEqual = NULL;
        }
        else
        {
            
        }
    }
    
    
    if (pcEqual == pcKey)
    {
        return BS_OK;
    }
    
    if ((NULL != pcEqual)&&(pcEqual > pcKey)&&(pcEqual < pcCur))
    {
        
        ulKeyLen = (ULONG)pcEqual - (ULONG)pcKey;
        pcPara   = (pcEqual + 1);
        ulParaLen = (ULONG)pcCur - (ULONG)(pcEqual + 1);
    }
    else
    {
        
             
        ulKeyLen = (ULONG)pcCur - (ULONG)pcKey;
        pcPara = NULL;
        ulParaLen = 0;
    }
    
    
    if (BS_OK != mime_AddNode(pstList, pcKey, ulKeyLen, pcPara, ulParaLen, pfDecodeFunc))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static MIME_DATA_NODE_S * mime_Find(IN MIME_DATALIST_S *pstParamList, IN CHAR *pcKey)
{
    
    MIME_DATA_NODE_S *pstParamNode = NULL;

    DLL_SCAN(&(pstParamList->stDataList), pstParamNode)
    {
        if (0 == (LONG)strcmp( pstParamNode->pcKey, pcKey ))
        {
            return pstParamNode;
        }
    }

    return NULL;
}

BS_STATUS MIME_Parse(IN MIME_HANDLE hMimeHandle, IN CHAR cSeparator, IN CHAR *pcString)
{
    
    if ((NULL == hMimeHandle) || (NULL == pcString))
    {
        return BS_ERR;
    }
    
    
    return mime_Parse(hMimeHandle, pcString, cSeparator, mime_DftDecode);
}


BS_STATUS MIME_ParseParam(IN MIME_HANDLE hMimeHandle, IN CHAR *pcParam)
{

    
    if ((NULL == hMimeHandle) || (NULL == pcParam))
    {
        return BS_ERR;
    }
    
    
    return mime_Parse(hMimeHandle, pcParam, HTTP_SEMICOLON_CHAR, mime_ParamDecode);
}


BS_STATUS MIME_ParseData (IN MIME_HANDLE hMimeHandle, IN CHAR *pcData)
{ 
    
    if ((NULL == hMimeHandle) || (NULL == pcData))
    {
        return BS_ERR;
    }
    
    
    return mime_Parse(hMimeHandle, pcData, HTTP_AND_CHAR, mime_DataDecode);  
}


BS_STATUS MIME_ParseCookie (IN MIME_HANDLE hMimeHandle, IN CHAR *pcData)
{
    
    if ((NULL == hMimeHandle) || (NULL == pcData))
    {
        return BS_ERR;
    }

    
    return mime_Parse(hMimeHandle, pcData, HTTP_SEMICOLON_CHAR, mime_CookieDecode);
}


BS_STATUS MIME_ParseContentDispos(IN MIME_HANDLE hMimeHandle, IN CHAR *pcData)
{
    CHAR *pcTemp = NULL;
    ULONG ulLength = 0;
    ULONG ulDataLen = 0;
    ULONG ulTotal = 0;
    ULONG ulRel = BS_OK;
    MIME_DATALIST_S *pstList = NULL;
    
    
    if ((NULL == hMimeHandle) || (NULL == pcData))
    {
        return BS_ERR;
    }

    pstList = hMimeHandle;

    ulDataLen = strlen(pcData);
    ulLength = strlen(HTTP_PART_HEAD_DISPOSITION_TYPE);

    
    ulTotal = ulDataLen + ulLength + 1;
    pcTemp = MEMPOOL_Alloc(pstList->hMemPool, ulTotal + 1);
    if (NULL == pcTemp)
    {
       return BS_ERR;
    }
    snprintf(pcTemp, ulTotal + 1, "%s%c%s", HTTP_PART_HEAD_DISPOSITION_TYPE, HTTP_EQUAL_CHAR, pcData);
    
    ulRel = mime_Parse(hMimeHandle, pcTemp, HTTP_SEMICOLON_CHAR, mime_ContentDisposDecode);

    return ulRel;

}


BS_STATUS MIME_SetKeyValue(IN MIME_HANDLE hMimeHandle, IN CHAR *pcKey, IN CHAR *pcValue)
{
    MIME_DATALIST_S *pstList = hMimeHandle;
    MIME_DATA_NODE_S *pstNodeOld = NULL;
    BS_STATUS eRet;

    
    if ((NULL == hMimeHandle) || (NULL == pcKey))
    {
        return BS_NULL_PARA;
    }

    if (NULL == pcValue)
    {
        pcValue = "";
    }

    pstNodeOld = mime_Find(pstList, pcKey);

    eRet = mime_AddNode(pstList, pcKey, strlen(pcKey), pcValue, strlen(pcValue), NULL);

    if ((BS_OK == eRet) && (NULL != pstNodeOld))
    {
        DLL_DEL(&pstList->stDataList, pstNodeOld);
    }

    return eRet;
}


CHAR * MIME_GetKeyValue(IN MIME_HANDLE hMimeHandle, IN CHAR *pcName)
{
    
    MIME_DATALIST_S *pstParamList = NULL;
    MIME_DATA_NODE_S *pstParamNode = NULL;

    
    if ((NULL == hMimeHandle) || (NULL == pcName))
    {
        return NULL;
    }
    
    pstParamList = (MIME_DATALIST_S *)hMimeHandle;
   
    pstParamNode = mime_Find(pstParamList, pcName);
    if (NULL == pstParamNode)
    {
        return NULL;
    }
    
    return pstParamNode->pcValue;
}


MIME_DATA_NODE_S * MIME_GetNextParam(IN MIME_HANDLE hMimeHandle, IN MIME_DATA_NODE_S *pstParam)
{
    MIME_DATA_NODE_S *pstNextParam;
    MIME_DATALIST_S *pstParamList;

    
    if (NULL == hMimeHandle)
    {
        return NULL;
    }

    pstParamList = (MIME_DATALIST_S *)hMimeHandle;

    pstNextParam = (MIME_DATA_NODE_S*)DLL_NEXT(&(pstParamList->stDataList), pstParam);

    return pstNextParam;
}

VOID MIME_Cat(IN MIME_HANDLE hMimeHandleDst, IN MIME_HANDLE hMimeHandleSrc)
{
    MIME_DATALIST_S *pstParamList1;
    MIME_DATALIST_S *pstParamList2;

    pstParamList1 = (MIME_DATALIST_S *)hMimeHandleDst;
    pstParamList2 = (MIME_DATALIST_S *)hMimeHandleSrc;

    DLL_CAT(&(pstParamList1->stDataList), &(pstParamList2->stDataList));
}

void MIME_Print(MIME_HANDLE hMime)
{
    MIME_DATA_NODE_S *node = NULL;

    while ((node = MIME_GetNextParam(hMime, node))) {
        printf("%s:%s\n", node->pcKey, node->pcValue);
    }
}

