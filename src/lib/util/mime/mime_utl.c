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

/*******************************************************************************
  创建MIME解析器                                                              
*******************************************************************************/
MIME_HANDLE MIME_Create (VOID)
{
    /* 局部变量定义 */
    MIME_DATALIST_S *pstMimeHandle = NULL;
    
    /* 申请内存资源 */
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
    
    /* 初始化链表头 */
    DLL_INIT(&(pstMimeHandle->stDataList));
    
    return ((MIME_HANDLE)pstMimeHandle);
}

/*******************************************************************************
  销毁MIME解析器                                                              
*******************************************************************************/
VOID MIME_Destroy (IN MIME_HANDLE hMimeHandle)
{
    MIME_DATALIST_S* pstParamListHead;

    /* 入参合法性检查 */
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

/*******************************************************************************
  完成Param字段的解码                                                              
*******************************************************************************/
STATIC CHAR* mime_ParamDecode(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen)
{
    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pcData);
    
    return HTTP_UriDecode(pstList->hMemPool, pcData, ulDataLen);
}


/*******************************************************************************
  完成Query字段的解码                                                             
*******************************************************************************/
STATIC CHAR * mime_DataDecode(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen)
{
    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pcData);
    
    return HTTP_DataDecode(pstList->hMemPool, pcData, ulDataLen);
}


/*******************************************************************************
  完成Cookie字段的解码                                                             
*******************************************************************************/
STATIC CHAR * mime_CookieDecode(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen)
{
    CHAR *pcOutData;
    
    /* 入参合法性检查 */
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

/*******************************************************************************
  完成Cookie字段的解码                                                             
*******************************************************************************/
STATIC CHAR * mime_ContentDisposDecode(IN MIME_DATALIST_S *pstList, IN CHAR *pcData, IN ULONG ulDataLen)
{
    ULONG ulNewLen = 0;
    CHAR *pcTemp;
    CHAR *pcDecode;

	/* 入参合法性检查 */
    BS_DBGASSERT(NULL != pcData);

    /* 参数值前后去引号操作 */
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

/*****************************************************************************
  构造节点的参数值域
******************************************************************************/
STATIC CHAR* mime_CreateKeyValue(IN MIME_DATALIST_S *pstList, IN CHAR *pcValue, IN ULONG ulValueLen, IN DECODE_FUNC_PF pfDecodeFunc)
{
    CHAR *pcKeyValue = NULL;

	/*  入参合法性检查 */
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

/*****************************************************************************
  创建数据节点，分配内存空间
******************************************************************************/
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

	/* 入参合法性判断 */
    BS_DBGASSERT(NULL != pcKeyName);
    BS_DBGASSERT(0 != ulKeyLen);

    /* 申请内存，创建数据结点 */
    pstNode = (MIME_DATA_NODE_S *)MEMPOOL_ZAlloc(pstList->hMemPool, sizeof(MIME_DATA_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }
    
    /* 复制参数名，pcKey域  */
    pcKey = pfDecodeFunc(pstList, pcKeyName, ulKeyLen); 
    pcNewValue = mime_CreateKeyValue(pstList, pcValue, ulValueLen, pfDecodeFunc);   
    
    pstNode->pcKey   = pcKey;
    pstNode->pcValue = pcNewValue;
    if((NULL == pcKey) || (NULL == pcNewValue))
    {
        pstNode = NULL;
    }
    return pstNode;
}


/*****************************************************************************
  创建参数结点，并加入到链表中
******************************************************************************/
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
    /* 局部变量定义 */
    CHAR *pcNewKey;
    ULONG ulNewKeyLen = 0;
    CHAR *pcNewValue;
    ULONG ulNewValueLen = 0;
    MIME_DATA_NODE_S *pstDataNode = NULL;

    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != pstList);
    BS_DBGASSERT(NULL != pcKey);

    if (NULL == pfDecodeFunc)
    {
        pfDecodeFunc = mime_DftDecode;
    }

    /* 对参数名做去前后空格处理 */
    pcNewKey = HTTP_Strim(pcKey, ulKeyLen, HTTP_SP_HT_STRING, &ulNewKeyLen);
    if( (NULL == pcNewKey)||(0 == ulNewKeyLen) )
    {
        /* 参数域必须存在，如果参数域非法，直接返回成功，不做当前参数节点挂接 */
        /* 如:出现"uid=ab800100&="情况时,直接返回ERROR_SUCCESS */
        return BS_OK;
    }

    /* 处理值域，值域允许传入为空，则存储空字符串值域 */
    pcNewValue = pcValue;
    ulNewValueLen = ulValueLen;
    
    if((NULL != pcValue)&&(0 != ulValueLen))
    {
        /* 存在值域则进行去前后空格处理,值域不存在则直接挂接空字符串值域 */
        pcNewValue = HTTP_Strim(pcValue, ulValueLen, HTTP_SP_HT_STRING, &ulNewValueLen);
    }

    /* 解析Param或者Query，调用解码函数解码 */
    pstDataNode = mime_CreatDecodeNode(pstList, pcNewKey, ulNewKeyLen, pcNewValue, ulNewValueLen, pfDecodeFunc);
    if (NULL == pstDataNode)
    {
        return BS_ERR;
    }


    /* 把节点加入链表 */
    DLL_ADD(&(pstList->stDataList), (DLL_NODE_S*)pstDataNode);

    return BS_OK;
}

/*******************************************************************************
  解析输入的字符串                                                               
*******************************************************************************/
STATIC BS_STATUS mime_Parse
(
    IN MIME_HANDLE hMimeHandle, 
    IN CHAR *pcData, 
    IN CHAR cSeparator, 
    IN DECODE_FUNC_PF pfDecodeFunc
)
{
    /* 局部变量定义 */
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
    
    /* 入参合法性检查 */
    BS_DBGASSERT(NULL != hMimeHandle);
    BS_DBGASSERT(NULL != pcData);
    
    /* 记录输入字符串长度及起始位置 */
    ulLength = strlen(pcData);
    pcBeg = pcData;
    pcEnd = pcData + ulLength;

    
    /* 获取首个参数位置 */
    pcKey = pcBeg;
    pstList = (MIME_DATALIST_S *)hMimeHandle;

    /* 请求样式: http://host/abs_path;para1=a;para2=b?query1=c&query2=d */
    /* 遍历字符串 */
    for (pcCur = pcBeg; pcCur < pcEnd; pcCur++)
    {   
        /* 当前出现'='号 */
        if (HTTP_EQUAL_CHAR == *pcCur)
        {
            /* 当前第一次出现'='号,记录当前位置 */            
            if(NULL == pcEqual)
            {
                pcEqual = pcCur;
            }
        }

        /* 出现分隔符 */
        else if (cSeparator == *pcCur)
        {   
            /* 1、Param第一个字符为'='号，例如=;a=b;c=d，继续后面参数a=b;c=d的解析 */
            /* 2、Param中间出现只有'='号情况，例如a=b;=;c=d，继续后面参数c=d的解析 */
            if (pcKey == pcEqual)
            {
                pcKey = pcCur + 1;
                pcEqual = NULL;
                continue;
            }
            /* 分隔符前存在'='号，进行提取参数和参数值,例如a=b;c=d;e=f或者a=b;c=;e=f */
            if ((NULL != pcEqual)&&(pcEqual > pcKey)&&(pcEqual < pcCur))
            {
                /* 存在'='号 */
                ulKeyLen =(ULONG)pcEqual - (ULONG)pcKey;
                pcPara   = (pcEqual + 1);
                ulParaLen = (ULONG)pcCur - (ULONG)(pcEqual + 1);
            }
            else
            {
                /* 找不到'='号，只有参数名，而无参数域 如:a=b;c;e=f */
                /* 此时同c=;做同样的处理，参数值为c、参数值域为空 */     
                ulKeyLen = (ULONG)pcCur - (ULONG)pcKey;
                pcPara = NULL;
                ulParaLen = 0;
            }
            

            /* 从pucBeg到pucEqual是Name，从pucEqual到pucCur是Value */
            if (BS_OK != mime_AddNode(pstList, pcKey, ulKeyLen, pcPara, ulParaLen, pfDecodeFunc))
            {
                return BS_ERR;
            }

            /* 添加完成，更新起始参数 */
            pcKey = pcCur + 1;
            pcEqual = NULL;
        }
        else
        {
            /* do nothing */
        }
    }
    
    /* 不存在参数名忽略该节点 */
    if (pcEqual == pcKey)
    {
        return BS_OK;
    }
    /* 只有一个参数或者是最后一个参数的情况下如a=b或者a=，进行参数和参数值提取 */
    if ((NULL != pcEqual)&&(pcEqual > pcKey)&&(pcEqual < pcCur))
    {
        /* 存在'='号 */
        ulKeyLen = (ULONG)pcEqual - (ULONG)pcKey;
        pcPara   = (pcEqual + 1);
        ulParaLen = (ULONG)pcCur - (ULONG)(pcEqual + 1);
    }
    else
    {
        /* 找不到'='号，只有参数名，而无参数域 */
        /* 此时同abcdef=;做同样的处理，参数值域为空 */     
        ulKeyLen = (ULONG)pcCur - (ULONG)pcKey;
        pcPara = NULL;
        ulParaLen = 0;
    }
    
    /* 从pucBeg到pucEqual是Name，从pucEqual到pucCur是Value */
    if (BS_OK != mime_AddNode(pstList, pcKey, ulKeyLen, pcPara, ulParaLen, pfDecodeFunc))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static MIME_DATA_NODE_S * mime_Find(IN MIME_DATALIST_S *pstParamList, IN CHAR *pcKey)
{
    /* 局部变量定义 */
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
    /* 入参合法性检查 */
    if ((NULL == hMimeHandle) || (NULL == pcString))
    {
        return BS_ERR;
    }
    
    /* 解析Param */
    return mime_Parse(hMimeHandle, pcString, cSeparator, mime_DftDecode);
}

/*******************************************************************************
  解析URL中Param字段                                                               
*******************************************************************************/
BS_STATUS MIME_ParseParam(IN MIME_HANDLE hMimeHandle, IN CHAR *pcParam)
{

    /* 入参合法性检查 */
    if ((NULL == hMimeHandle) || (NULL == pcParam))
    {
        return BS_ERR;
    }
    
    /* 解析Param */
    return mime_Parse(hMimeHandle, pcParam, HTTP_SEMICOLON_CHAR, mime_ParamDecode);
}

/*******************************************************************************
  解析URL中Query字段                                                             
*******************************************************************************/
BS_STATUS MIME_ParseData (IN MIME_HANDLE hMimeHandle, IN CHAR *pcData)
{ 
    /* 入参合法性检查 */
    if ((NULL == hMimeHandle) || (NULL == pcData))
    {
        return BS_ERR;
    }
    
    /* 解析Query */
    return mime_Parse(hMimeHandle, pcData, HTTP_AND_CHAR, mime_DataDecode);  
}

/*******************************************************************************
  解析URL中Cookie字段                                                                
*******************************************************************************/
BS_STATUS MIME_ParseCookie (IN MIME_HANDLE hMimeHandle, IN CHAR *pcData)
{
    /* 入参合法性检查 */
    if ((NULL == hMimeHandle) || (NULL == pcData))
    {
        return BS_ERR;
    }

    /* 解析Cookie */
    return mime_Parse(hMimeHandle, pcData, HTTP_SEMICOLON_CHAR, mime_CookieDecode);
}

/*******************************************************************************
  解析报文头中Content-Disposition域                                                                
*******************************************************************************/
BS_STATUS MIME_ParseContentDispos(IN MIME_HANDLE hMimeHandle, IN CHAR *pcData)
{
    CHAR *pcTemp = NULL;
    ULONG ulLength = 0;
    ULONG ulDataLen = 0;
    ULONG ulTotal = 0;
    ULONG ulRel = BS_OK;
    MIME_DATALIST_S *pstList = NULL;
    
    /* 入参合法性检查 */
    if ((NULL == hMimeHandle) || (NULL == pcData))
    {
        return BS_ERR;
    }

    pstList = hMimeHandle;

    ulDataLen = strlen(pcData);
    ulLength = strlen(HTTP_PART_HEAD_DISPOSITION_TYPE);

    /* 用户输入数据长度、Disopsition-Type域长度及等号长度之和 */
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

/* 设置Key Value, 对已经存在的进行覆盖 */
BS_STATUS MIME_SetKeyValue(IN MIME_HANDLE hMimeHandle, IN CHAR *pcKey, IN CHAR *pcValue)
{
    MIME_DATALIST_S *pstList = hMimeHandle;
    MIME_DATA_NODE_S *pstNodeOld = NULL;
    BS_STATUS eRet;

    /* 入参合法性检查 */
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

/*******************************************************************************
  根据参数名，得到相应的域值                                                              
*******************************************************************************/
CHAR * MIME_GetKeyValue(IN MIME_HANDLE hMimeHandle, IN CHAR *pcName)
{
    /* 局部变量定义 */
    MIME_DATALIST_S *pstParamList = NULL;
    MIME_DATA_NODE_S *pstParamNode = NULL;

    /* 入参合法性检查 */
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

/*******************************************************************************
  根据输入的链表节点，得到链表的下一个节点, NULL表示获取第一个节点                                                             
*******************************************************************************/
MIME_DATA_NODE_S * MIME_GetNextParam(IN MIME_HANDLE hMimeHandle, IN MIME_DATA_NODE_S *pstParam)
{
    MIME_DATA_NODE_S *pstNextParam;
    MIME_DATALIST_S *pstParamList;

    /* 入参合法性检查 */
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

