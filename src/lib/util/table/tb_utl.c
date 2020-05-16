/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-12-10
* Description: 
* History:     
******************************************************************************/
    /* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_TB


#include "bs.h"

#include "utl/tb_utl.h"
#include "utl/txt_utl.h"

typedef enum
{
    TB_TYPE_UNKNOWN = 0,
    TB_TYPE_UINT32,
    TB_TYPE_STRING,
    TB_TYPE_MEM,
}TB_DATA_TYPE_E;

typedef struct
{
    UINT ulLen;
    VOID  *pContent;
    BOOL_T bIsMemMalloc;
}_TB_NODE_CONTENT_S;

typedef struct
{
    DLL_NODE_S stRowLinkNode;
    _TB_NODE_CONTENT_S stContent[0];
}_TB_NODE_S;

typedef struct
{
    CHAR szTbName[TB_MAX_NAME_LEN+1];
    DLL_HEAD_S stRowListHead;   /* _TB_NODE_S */
    
    UINT ulKeyNum;
    UINT ulColNum;
    TB_DATA_TYPE_E aeType[0];
}_TB_HEAD_S;

typedef struct
{
    HANDLE ahKey[TB_MAX_KEY_NUM];
}_TB_KEY_S;

BS_STATUS TB_Create(IN CHAR *pszTbName, IN UINT ulColNum, OUT HANDLE *phTbHandle)
{
    _TB_HEAD_S *pstTbHead;
    UINT ulLen;

    if (pszTbName && (strlen(pszTbName) > TB_MAX_NAME_LEN))
    {
        RETURN(BS_TOO_LONG);
    }

    ulLen = sizeof(_TB_HEAD_S) + sizeof(TB_DATA_TYPE_E) * ulColNum;

    pstTbHead = MEM_ZMalloc(ulLen);
    if (NULL == pstTbHead)
    {
        RETURN(BS_NO_MEMORY);
    }

    if (pszTbName)
    {
        TXT_StrCpy(pstTbHead->szTbName, pszTbName);
    }

    pstTbHead->ulColNum = ulColNum;

    *phTbHandle = pstTbHead;

    return BS_OK;
}

BS_STATUS TB_SetColType(IN HANDLE hTbHandle, IN UINT ulCol, IN TB_DATA_TYPE_E eType)
{
    _TB_HEAD_S *pstTbHead = (_TB_HEAD_S *)hTbHandle;

    if (NULL == pstTbHead)
    {
        RETURN(BS_NULL_PARA);
    }

    if (ulCol >= pstTbHead->ulColNum)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    pstTbHead->aeType[ulCol] = eType;

    return BS_OK;
}

BS_STATUS TB_SetKeyNum(IN HANDLE hTbHandle, IN UINT ulKeyNum)
{
    _TB_HEAD_S *pstTbHead = (_TB_HEAD_S *)hTbHandle;

    if (NULL == pstTbHead)
    {
        RETURN(BS_NULL_PARA);
    }

    if (ulKeyNum > pstTbHead->ulColNum)
    {
        ulKeyNum = pstTbHead->ulColNum;
    }

    if (ulKeyNum > TB_MAX_KEY_NUM)
    {
        RETURN(BS_NOT_SUPPORT);
    }

    pstTbHead->ulKeyNum = ulKeyNum;

    return BS_OK;
}

static INT _TB_Cmp(IN _TB_HEAD_S *pstTbHead, IN _TB_NODE_S *pstNode, IN _TB_KEY_S *pstKey)
{
    UINT i;
    INT lRet;

    for (i=0; i<pstTbHead->ulKeyNum; i++)
    {
        switch (pstTbHead->aeType[i])
        {
        case TB_TYPE_UNKNOWN:
		case TB_TYPE_MEM:
            break;
            
        case TB_TYPE_UINT32:
            if ((pstNode->stContent[i].pContent) == pstKey->ahKey[i])
            {
                break;
            }
            else 
            {
                return (pstNode->stContent[i].pContent) > pstKey->ahKey[i] ? 1 : -1;
            }
            break;

        case TB_TYPE_STRING:
            lRet = strcmp(pstNode->stContent[i].pContent, (CHAR*)pstKey->ahKey[i]);
            if (lRet != 0)
            {
                return lRet;
            }
            break;
        }
    }

    return 0;
}

static _TB_NODE_S * _TB_FindRow(IN _TB_HEAD_S *pstTbHead, IN _TB_KEY_S *pstKey)
{
    _TB_NODE_S *pstNode;

    if (pstTbHead->ulKeyNum == 0)
    {
        return DLL_FIRST(&pstTbHead->stRowListHead);
    }

    DLL_SCAN(&pstTbHead->stRowListHead, pstNode)
    {
        if (_TB_Cmp (pstTbHead, pstNode, pstKey) == 0)
        {
            return pstNode;
        }
    }

    return NULL;
}

static BS_STATUS _TB_AddRowWithOutCheck(IN _TB_HEAD_S *pstTbHead, IN _TB_KEY_S *pstKey)
{
    _TB_NODE_S *pstNode;
    UINT ulLen;
    UINT i, j;

    ulLen = sizeof(_TB_NODE_S) + sizeof(_TB_NODE_CONTENT_S) * pstTbHead->ulColNum;
    
    pstNode = MEM_ZMalloc(ulLen);
    if (NULL == pstNode)
    {
        RETURN(BS_NO_MEMORY);
    }

    for (i=0,j=0; j<pstTbHead->ulKeyNum; i++,j++)
    {
        switch (pstTbHead->aeType[i])
        {
        case TB_TYPE_UNKNOWN:   /* 当作UINT32 */
		case TB_TYPE_MEM:
        case TB_TYPE_UINT32:
            pstNode->stContent[j].pContent = (VOID*)pstKey->ahKey[i];
            pstNode->stContent[j].ulLen = sizeof(UINT);
            break;

        case TB_TYPE_STRING:
            ulLen = strlen((CHAR*)pstKey->ahKey[i]);
            if (NULL == (pstNode->stContent[j].pContent = MEM_Malloc(ulLen+1)))
            {
                goto LAB_ERR;
            }
            TXT_StrCpy(pstNode->stContent[j].pContent, (CHAR*)pstKey->ahKey[i]);
            pstNode->stContent[j].ulLen = ulLen;
            pstNode->stContent[j].bIsMemMalloc = TRUE;
            break;
        }
    }

    DLL_ADD(&pstTbHead->stRowListHead, pstNode);

    return BS_OK;
    
LAB_ERR:
    for (i=0; i<j; i++)
    {
        if (pstNode->stContent[j].bIsMemMalloc == TRUE)
        {
            MEM_Free(pstNode->stContent[j].pContent);
        }
    }

    RETURN(BS_ERR);
}

BS_STATUS TB_AddRow(IN HANDLE hTbHandle, IN _TB_KEY_S *pstKey)
{
    _TB_HEAD_S *pstTbHead = (_TB_HEAD_S *)hTbHandle;

    if (NULL == pstTbHead)
    {
        RETURN(BS_NULL_PARA);
    }

    if (pstTbHead->ulKeyNum > 0)
    {
        if (NULL != _TB_FindRow(pstTbHead, pstKey))
        {
            RETURN(BS_ALREADY_EXIST);
        }
    }

    return _TB_AddRowWithOutCheck(pstTbHead, pstKey);
}

static VOID _TB_FreeRow(IN _TB_HEAD_S *pstTbHead, IN _TB_NODE_S *pstNode)
{
    UINT i;
    
    DLL_DEL(&pstTbHead->stRowListHead, pstNode);

    for (i=0; i<pstTbHead->ulColNum; i++)
    {
        if (pstNode->stContent[i].bIsMemMalloc == TRUE)
        {
            MEM_Free(pstNode->stContent[i].pContent);
        }
    }

    MEM_Free(pstNode);
}

BS_STATUS TB_DelRow(IN HANDLE hTbHandle, IN _TB_KEY_S *pstKey)
{
    _TB_HEAD_S *pstTbHead = (_TB_HEAD_S *)hTbHandle;
    _TB_NODE_S *pstNode;

    if (NULL == pstTbHead)
    {
        RETURN(BS_NULL_PARA);
    }


    if (pstTbHead->ulKeyNum > 0)
    {
        pstNode = _TB_FindRow(pstTbHead, pstKey);
        if (NULL != pstNode)
        {
            RETURN(BS_NO_SUCH);
        }

        _TB_FreeRow(pstTbHead, pstNode);
        return BS_OK;
    }

    RETURN(BS_NO_SUCH);
}

BS_STATUS TB_SetValueAsLong(IN HANDLE hTbHandle, IN _TB_KEY_S *pstKey, IN UINT ulColNum, IN UINT ulValue)
{
    _TB_HEAD_S *pstTbHead = (_TB_HEAD_S *)hTbHandle;
    _TB_NODE_S *pstNode;

    if (NULL == pstTbHead)
    {
        RETURN(BS_NULL_PARA);
    }

    if (pstTbHead->aeType[ulColNum] != TB_TYPE_UINT32)
    {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    if (pstTbHead->ulKeyNum > 0)
    {
        pstNode = _TB_FindRow(pstTbHead, pstKey);
        if (NULL != pstNode)
        {
            RETURN(BS_NO_SUCH);
        }

        pstNode->stContent[ulColNum].pContent = UINT_HANDLE(ulValue);
        pstNode->stContent[ulColNum].ulLen = 4;
        pstNode->stContent[ulColNum].bIsMemMalloc = FALSE;
        return BS_OK;
    }

    RETURN(BS_NO_SUCH);
}

BS_STATUS TB_SetValueAsString(IN HANDLE hTbHandle, IN _TB_KEY_S *pstKey, IN UINT ulColNum, IN CHAR *szValue)
{
    _TB_HEAD_S *pstTbHead = (_TB_HEAD_S *)hTbHandle;
    UINT ulLen;
    _TB_NODE_S *pstNode;

    if (NULL == pstTbHead)
    {
        RETURN(BS_NULL_PARA);
    }

    if (pstTbHead->aeType[ulColNum] != TB_TYPE_STRING)
    {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    if (pstTbHead->ulKeyNum > 0)
    {
        pstNode = _TB_FindRow(pstTbHead, pstKey);
        if (NULL != pstNode)
        {
            RETURN(BS_NO_SUCH);
        }

        ulLen = strlen(szValue);
        if (NULL == (pstNode->stContent[ulColNum].pContent = MEM_Malloc(ulLen+1)))
        {
            RETURN(BS_NO_MEMORY);
        }            
        
        TXT_StrCpy(pstNode->stContent[ulColNum].pContent, szValue);
        pstNode->stContent[ulColNum].ulLen = ulLen;
        pstNode->stContent[ulColNum].bIsMemMalloc = TRUE;
        return BS_OK;
    }

    RETURN(BS_NO_SUCH);
}

BS_STATUS TB_SetValueAsMem(IN HANDLE hTbHandle, IN _TB_KEY_S *pstKey, IN UINT ulColNum, IN UCHAR *pucValue, IN UINT ulLen)
{
    _TB_HEAD_S *pstTbHead = (_TB_HEAD_S *)hTbHandle;
    _TB_NODE_S *pstNode;

    if (NULL == pstTbHead)
    {
        RETURN(BS_NULL_PARA);
    }

    if (pstTbHead->aeType[ulColNum] != TB_TYPE_MEM)
    {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    if (pstTbHead->ulKeyNum > 0)
    {
        pstNode = _TB_FindRow(pstTbHead, pstKey);
        if (NULL != pstNode)
        {
            RETURN(BS_NO_SUCH);
        }


        if (NULL == (pstNode->stContent[ulColNum].pContent = MEM_Malloc(ulLen)))
        {
            RETURN(BS_NO_MEMORY);
        }
        
        MEM_Copy(pstNode->stContent[ulColNum].pContent, pucValue, ulLen);
        pstNode->stContent[ulColNum].ulLen = ulLen;
        pstNode->stContent[ulColNum].bIsMemMalloc = TRUE;
        return BS_OK;
    }

    RETURN(BS_NO_SUCH);
}

BOOL_T TB_IsExist(IN HANDLE hTbHandle, IN _TB_KEY_S *pstKey)
{
    _TB_HEAD_S *pstTbHead = (_TB_HEAD_S *)hTbHandle;

    if (NULL == pstTbHead)
    {
        return FALSE;
    }

    if (pstTbHead->ulKeyNum > 0)
    {
        if (NULL != _TB_FindRow(pstTbHead, pstKey))
        {
            return TRUE;
        }
    }

    return FALSE;
}

