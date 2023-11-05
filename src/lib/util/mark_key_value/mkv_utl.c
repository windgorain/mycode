/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-5-22
* Description:  makr_key_value 树管理
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_MKV
    
#include "bs.h"

#include "utl/mkv_utl.h"
#include "utl/txt_utl.h"

static INT _MKV_CompMark(IN MKV_MARK_S *pstNode1, IN MKV_MARK_S *pstNode2, IN UINT ulUserHandle)
{
    return strcmp(pstNode1->pucMarkName, pstNode2->pucMarkName);
}

static inline MKV_MARK_S * _MKV_FormNewMark(IN CHAR *pszMarkName, IN BOOL_T bCopy)
{
    MKV_MARK_S *pstMark = NULL;
    UINT uiLen;

    pstMark = malloc(sizeof(MKV_MARK_S));
    if (NULL == pstMark)
    {
        return NULL;
    }
    memset(pstMark, 0, sizeof(MKV_MARK_S));

    pstMark->bIsCopy = bCopy;
    DLL_INIT(&pstMark->stKeyValueDllHead);
    DLL_INIT(&pstMark->stSectionDllHead);
    
    if (bCopy)
    {
        uiLen = strlen(pszMarkName) + 1;
        pstMark->pucMarkName = malloc(uiLen);
        if (NULL == pstMark->pucMarkName)
        {
            free(pstMark);
            return NULL;
        }
        
        TXT_Strlcpy(pstMark->pucMarkName, pszMarkName, uiLen);
    }
    else
    {
        pstMark->pucMarkName = pszMarkName;
    }

    return pstMark;
}

MKV_MARK_S * MKV_GetLastMarkOfLevel(MKV_MARK_S *pstRoot, IN UINT ulLevel)
{
    UINT ulLevelTmp = 0;
    MKV_MARK_S *pstMark = pstRoot;

    if (ulLevel == 0)
    {
        return pstMark;
    }

    for (ulLevelTmp=1; ulLevelTmp <= ulLevel; ulLevelTmp++)
    {
        pstMark = DLL_LAST(&(pstMark->stSectionDllHead));

        if (NULL == pstMark)
        {
            return NULL;
        }
    }

    return pstMark;    
}


MKV_MARK_S * MKV_AddMark2Mark(IN MKV_MARK_S *pstRoot, IN CHAR *pszMarkName, IN BOOL_T bCopy)
{
    MKV_MARK_S *pstMark;

    pstMark = _MKV_FormNewMark(pszMarkName, bCopy);
    if (NULL == pstMark)
    {
        return NULL;
    }

    DLL_ADD(&pstRoot->stSectionDllHead, pstMark);

    return pstMark;
}

MKV_MARK_S * MKV_AddMark2MarkWithSort(IN MKV_MARK_S *pstRoot, IN CHAR *pszMarkName, IN BOOL_T bCopy)
{
    MKV_MARK_S *pstMark, *pstMarkTmp;

    pstMark = _MKV_FormNewMark(pszMarkName, bCopy);
    if (NULL == pstMark)
    {
        return NULL;
    }

    DLL_SCAN(&pstRoot->stSectionDllHead, pstMarkTmp)
    {
        if (strcmp(pszMarkName, pstMarkTmp->pucMarkName) <= 0)
        {
            DLL_INSERT_BEFORE(&pstRoot->stSectionDllHead, pstMark, pstMarkTmp);
            return pstMark;
        }
    }

    DLL_ADD(&pstRoot->stSectionDllHead, pstMark);

    return pstMark;
}

MKV_KEY_S * MKV_AddNewKey2Mark
(
    IN MKV_MARK_S *pstMark,
    IN CHAR *pszKeyName,
    IN CHAR *pszValue,
    IN BOOL_T bCopy,
    IN BOOL_T bSort
)
{
    MKV_KEY_S *pstKey = NULL;
    MKV_KEY_S *pstKeyTmp;
    UINT uiLen;
    
    pstKey = malloc(sizeof(MKV_KEY_S));
    if (NULL == pstKey)
    {
        return NULL;
    }
    memset(pstKey, 0, sizeof(MKV_KEY_S));

    pstKey->bIsCopy = bCopy;
    
    if (bCopy)
    {
        uiLen = strlen(pszKeyName) + 1;
        pstKey->pucKeyName = malloc(uiLen);
        if (NULL == pstKey->pucKeyName)
        {
            free(pstKey);
            return NULL;
        }
        TXT_Strlcpy(pstKey->pucKeyName, pszKeyName, uiLen);

        uiLen = strlen(pszValue) + 1;
        pstKey->pucKeyValue = malloc(uiLen);
        if (NULL == pstKey->pucKeyValue)
        {
            free(pstKey->pucKeyName);
            free(pstKey);
            return NULL;
        }        
        TXT_Strlcpy (pstKey->pucKeyValue, pszValue, uiLen);
    }
    else
    {
        pstKey->pucKeyName = pszKeyName;
        pstKey->pucKeyValue = pszValue;
    }

    if (bSort)
    {
        DLL_SCAN(&pstMark->stKeyValueDllHead, pstKeyTmp)
        {
            if (strcmp(pszKeyName, pstKeyTmp->pucKeyName) < 0)
            {
                DLL_INSERT_BEFORE(&pstMark->stKeyValueDllHead, pstKey, pstKeyTmp);
                return pstKey;
            }
        }
    }

    DLL_ADD(&pstMark->stKeyValueDllHead, pstKey);

    return pstKey;

}

VOID MKV_SortMark(IN MKV_MARK_S *pstRoot)
{
    MKV_MARK_S *pstNextMark;

    BS_DBGASSERT(NULL != pstRoot);

    DLL_Sort(&pstRoot->stSectionDllHead, (PF_DLL_CMP_FUNC)_MKV_CompMark, 0);

    DLL_SCAN(&pstRoot->stSectionDllHead, pstNextMark)
    {
        MKV_SortMark(pstNextMark);
    }
}

MKV_MARK_S * MKV_FindMarkInMark(IN MKV_MARK_S *pstRoot, IN CHAR *pszMarkName)
{
    MKV_MARK_S *pstNode;
    
    DLL_SCAN(&pstRoot->stSectionDllHead, pstNode)
    {
        if (strcmp(pszMarkName, pstNode->pucMarkName) == 0)
        {
            return pstNode;
        }
    }

    return NULL;    
}

VOID MKV_DelMarkInMark(IN MKV_MARK_S *pstMarkRoot, IN MKV_MARK_S *pstMark)
{
    MKV_KEY_S *pstKey, *pstKeyNodeTmp;
    MKV_MARK_S *pstNode, *pstNodeTmp;

    DLL_SAFE_SCAN (&pstMark->stKeyValueDllHead, pstKey, pstKeyNodeTmp)
    {
        DLL_DEL(&pstMark->stKeyValueDllHead, pstKey);
        if (pstKey->bIsCopy == TRUE)
        {
            free(pstKey->pucKeyName);
            free(pstKey->pucKeyValue);
        }
        free(pstKey);
    }

    DLL_SAFE_SCAN(&pstMark->stSectionDllHead, pstNode, pstNodeTmp)
    {
        MKV_DelMarkInMark(pstMark, pstNode);
    }

    DLL_DEL(&pstMarkRoot->stSectionDllHead, &pstMark->stDllNode);
    if (pstMark->bIsCopy == TRUE)
    {
        free(pstMark->pucMarkName);
    }
    free(pstMark);

    return;
}

VOID MKV_DelAllMarkInMark(IN MKV_MARK_S *pstMarkRoot)
{
    MKV_MARK_S *pstSecNode, *pstSecNodeTmp;

    DLL_SAFE_SCAN (&pstMarkRoot->stSectionDllHead, pstSecNode, pstSecNodeTmp)
    {
        MKV_DelMarkInMark(pstMarkRoot, pstSecNode);
    }

    return;
}

VOID MKV_DelAllInMark(IN MKV_MARK_S *pstMarkRoot)
{
    MKV_DelAllMarkInMark(pstMarkRoot);
    MKV_DelAllKeyInMark(pstMarkRoot);
}

MKV_KEY_S* MKV_FindKeyInMark(IN MKV_MARK_S* pstMark, IN CHAR *pucKeyName)
{
    MKV_KEY_S *pstKeyNode = NULL;

    DLL_SCAN (&pstMark->stKeyValueDllHead, pstKeyNode)
    {
        if (strcmp(pucKeyName, pstKeyNode->pucKeyName) == 0)
        {
            return pstKeyNode;
        }
    }

    return NULL;
}

MKV_KEY_S * MKV_SetKeyValueInMark
(
    IN MKV_MARK_S *pstMark,
    IN CHAR *pszKeyName,
    IN CHAR *pszValue,
    IN BOOL_T bCopy,
    IN BOOL_T bSort
)
{
    MKV_KEY_S *pstOldKey;
    MKV_KEY_S *pstNewKey;

    pstOldKey = MKV_FindKeyInMark(pstMark, pszKeyName);
    pstNewKey = MKV_AddNewKey2Mark(pstMark, pszKeyName, pszValue, bCopy, bSort);
    if (NULL != pstNewKey)
    {
        if (NULL != pstOldKey)
        {
            MKV_DelKeyInMark(pstMark, pstOldKey);
        }
    }

    return pstNewKey;
}

BS_STATUS MKV_DelKeyInMark(IN MKV_MARK_S *pstMark, IN MKV_KEY_S *pstKey)
{
    DLL_DEL(&pstMark->stKeyValueDllHead, &pstKey->stDllNode);
    if (pstKey->bIsCopy == TRUE)
    {
        free(pstKey->pucKeyName);
        free(pstKey->pucKeyValue);
    }

    free(pstKey);

    return BS_OK;
}

VOID MKV_DelAllKeyInMark(IN MKV_MARK_S *pstMark)
{
    MKV_KEY_S *pstKey, *pstKeyTmp;

    DLL_SAFE_SCAN(&pstMark->stKeyValueDllHead, pstKey, pstKeyTmp)
    {
        MKV_DelKeyInMark(pstMark, pstKey);
    }
}

MKV_MARK_S * MKV_FindMarkByLevel(IN MKV_MARK_S *pstRoot, IN UINT ulLevel, IN CHAR ** apszArgs)
{
    MKV_MARK_S * pstMark;

    if (ulLevel == 0)
    {
        return pstRoot;
    }
    
    DLL_SCAN(&pstRoot->stSectionDllHead, pstMark)
    {
        if (strcmp(pstMark->pucMarkName, apszArgs[0]) == 0)
        {
            return MKV_FindMarkByLevel(pstMark, ulLevel - 1, apszArgs + 1);
        }
    }

    return NULL;
}

BS_STATUS MKV_DelKey(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks, IN CHAR *pszKey)
{
    MKV_MARK_S *pstMark;
    MKV_KEY_S *pstKey;

    BS_DBGASSERT(pstMarks->ulLevle <= MKV_MAX_LEVEL);

    pstMark = MKV_FindMarkByLevel(pstRoot, pstMarks->ulLevle, pstMarks->apszMarkName);
    if (NULL == pstMark)
    {
        RETURN(BS_NO_SUCH);
    }

    pstKey = MKV_FindKeyInMark(pstMark, pszKey);
    if (NULL == pstKey)
    {
        RETURN(BS_NO_SUCH);
    }

    return MKV_DelKeyInMark(pstMark, pstKey);
}

BS_STATUS MKV_DelAllKeyOfMark(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks)
{
    MKV_MARK_S *pstMark;

    BS_DBGASSERT(pstMarks->ulLevle <= MKV_MAX_LEVEL);

    pstMark = MKV_FindMarkByLevel(pstRoot, pstMarks->ulLevle, pstMarks->apszMarkName);
    if (NULL == pstMark)
    {
        RETURN(BS_NO_SUCH);
    }

    MKV_DelAllKeyInMark(pstMark);

    return BS_OK;
}

BS_STATUS MKV_DelMark(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks)
{
    MKV_MARK_S *pstMark;
    MKV_MARK_S *pstMarkRoot;

    BS_DBGASSERT(pstMarks->ulLevle <= MKV_MAX_LEVEL);

    pstMarkRoot = MKV_FindMarkByLevel(pstRoot, pstMarks->ulLevle - 1, pstMarks->apszMarkName);
    if (NULL == pstMarkRoot)
    {
        RETURN(BS_NO_SUCH);
    }

    pstMark = MKV_FindMarkInMark(pstMarkRoot, pstMarks->apszMarkName[pstMarks->ulLevle - 1]);
    if (NULL == pstMark)
    {
        RETURN(BS_NO_SUCH);
    }

    MKV_DelMarkInMark(pstMarkRoot, pstMark);

    return BS_OK;
}


BS_STATUS MKV_AddMark(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks, IN BOOL_T bSort)
{
    MKV_MARK_S *pstMark;
    MKV_MARK_S *pstMarkRoot;
    UINT i;

    BS_DBGASSERT(pstMarks->ulLevle <= MKV_MAX_LEVEL);

    pstMarkRoot = pstRoot;

    for (i=0; i<pstMarks->ulLevle; i++)
    {
        pstMark = MKV_FindMarkInMark(pstMarkRoot, pstMarks->apszMarkName[i]);
        if (NULL == pstMark)
        {
            if (bSort == TRUE)
            {
                pstMark = MKV_AddMark2MarkWithSort(pstMarkRoot, pstMarks->apszMarkName[i], TRUE);
            }
            else
            {
                pstMark = MKV_AddMark2Mark(pstMarkRoot, pstMarks->apszMarkName[i], TRUE);
            }
        }

        if (NULL == pstMark)
        {
            RETURN(BS_NO_MEMORY);
        }

        pstMarkRoot = pstMark;
    }

    return BS_OK;
}

MKV_MARK_S * MKV_GetMark(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks)
{
    return MKV_FindMarkByLevel(pstRoot, pstMarks->ulLevle, pstMarks->apszMarkName);
}

BOOL_T MKV_IsMarkExist(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks)
{
    if (NULL == MKV_GetMark(pstRoot, pstMarks))
    {
        return FALSE;
    }

    return TRUE;
}

CHAR * MKV_GetNextMarkInMark(IN MKV_MARK_S *pstRoot, IN CHAR *pcCurMarkName)
{
    MKV_MARK_S *pstMark;
    BOOL_T bFind = FALSE;

    if ((pcCurMarkName == NULL) || (pcCurMarkName[0] == '\0'))
    {
        pstMark = DLL_FIRST(&pstRoot->stSectionDllHead);
        if (NULL == pstMark)
        {
            return NULL;
        }
        
        return pstMark->pucMarkName;
    }

    DLL_SCAN(&pstRoot->stSectionDllHead, pstMark)
    {
        if (bFind)
        {
            return pstMark->pucMarkName;
        }

        if (strcmp(pcCurMarkName, pstMark->pucMarkName) == 0)
        {
            bFind = TRUE;
        }
    }

    return NULL;
}

CHAR * MKV_GetMarkByIndexInMark(IN MKV_MARK_S *pstRoot, IN UINT uiIndex)
{
    MKV_MARK_S *pstMark;
    UINT i = 0;

    DLL_SCAN(&pstRoot->stSectionDllHead, pstMark)
    {
        if (i == uiIndex)
        {
            return pstMark->pucMarkName;
        }

        i++;
    }

    return NULL;
}

CHAR * MKV_GetNextMark(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks, IN CHAR *pcCurMarkName)
{
    MKV_MARK_S *pstMarkRoot;

    pstMarkRoot = MKV_GetMark(pstRoot, pstMarks);
    if (NULL == pstMarkRoot)
    {
        return NULL;
    }

    return MKV_GetNextMarkInMark(pstMarkRoot, pcCurMarkName);
}

CHAR * MKV_GetMarkByIndex(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks, IN UINT uiIndex)
{
    MKV_MARK_S *pstMarkRoot;

    pstMarkRoot = MKV_GetMark(pstRoot, pstMarks);
    if (NULL == pstMarkRoot)
    {
        return NULL;
    }

    return MKV_GetMarkByIndexInMark(pstMarkRoot, uiIndex);
}

BS_STATUS MKV_GetNextKeyInMark(IN MKV_MARK_S *pstMark, INOUT CHAR **ppszKeyName)
{
    MKV_KEY_S *pstKey;

    if ((*ppszKeyName == NULL) || ((*ppszKeyName)[0] == '\0'))
    {
        pstKey = DLL_FIRST(&pstMark->stKeyValueDllHead);
        if (NULL == pstKey)
        {
            RETURN(BS_NO_SUCH);
        }
        
        *ppszKeyName = pstKey->pucKeyName;
        return BS_OK;
    }

    DLL_SCAN(&pstMark->stKeyValueDllHead, pstKey)
    {
        if (strcmp(*ppszKeyName, pstKey->pucKeyName) < 0)
        {
            *ppszKeyName = pstKey->pucKeyName;
            return BS_OK;
        }
    }
    
    RETURN(BS_NO_SUCH);
}

BS_STATUS MKV_GetNextKey(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks, INOUT CHAR **ppszKeyName)
{
    MKV_MARK_S *pstMark;

    BS_DBGASSERT(NULL != ppszKeyName);

    pstMark = MKV_GetMark(pstRoot, pstMarks);
    if (NULL == pstMark)
    {
        RETURN(BS_NO_SUCH);
    }

    return MKV_GetNextKeyInMark(pstMark, ppszKeyName);
}

BS_STATUS MKV_SetKeyValueAsString
(
    IN MKV_MARK_S *pstRoot,
    IN MKV_X_PARA_S *pstMarks,
    IN CHAR *pszKeyName,
    IN CHAR *pszValue,
    IN BOOL_T bMarkSort
)
{
    MKV_MARK_S *pstMark;
    BS_STATUS eRet;

    BS_DBGASSERT(pstMarks->ulLevle <= MKV_MAX_LEVEL);

    eRet = MKV_AddMark(pstRoot, pstMarks, bMarkSort);
    if (eRet != BS_OK)
    {
        return eRet;
    }

    pstMark = MKV_GetMark(pstRoot, pstMarks);
    if (NULL == pstMark)
    {
        BS_DBGASSERT(0);
        RETURN(BS_NO_SUCH);
    }

    if (NULL == MKV_SetKeyValueInMark(pstMark, pszKeyName, pszValue, TRUE, bMarkSort))
    {
        RETURN(BS_NO_MEMORY);
    }

    return BS_OK;
}

BS_STATUS MKV_SetKeyValueAsUint
(
    IN MKV_MARK_S *pstRoot,
    IN MKV_X_PARA_S *pstMarks,
    IN CHAR *pucKeyName,
    IN UINT uiKeyValue,
    IN BOOL_T bMarkSort
)
{
    CHAR szValue[32];

    if ((NULL == pstMarks) || (pucKeyName == NULL))
    {
        RETURN(BS_NULL_PARA);
    }

    sprintf(szValue, "%u", uiKeyValue);
    
    return MKV_SetKeyValueAsString(pstRoot, pstMarks, pucKeyName, szValue, bMarkSort);
}

BS_STATUS MKV_SetKeyValueAsUint64
(
    IN MKV_MARK_S *pstRoot,
    IN MKV_X_PARA_S *pstMarks,
    IN CHAR *pucKeyName,
    IN UINT64 uiKeyValue,
    IN BOOL_T bMarkSort
)
{
    CHAR szValue[64];

    if ((NULL == pstMarks) || (pucKeyName == NULL))
    {
        RETURN(BS_NULL_PARA);
    }

    sprintf(szValue, "%llu", uiKeyValue);
    
    return MKV_SetKeyValueAsString(pstRoot, pstMarks, pucKeyName, szValue, bMarkSort);
}

BS_STATUS MKV_GetKeyValueAsString
(
    IN MKV_MARK_S *pstRoot,
    IN MKV_X_PARA_S *pstMarks,
    IN CHAR *pszKeyName,
    OUT CHAR **ppszKeyValue
)
{
    MKV_MARK_S *pstMark;
    MKV_KEY_S *pstKey;

    BS_DBGASSERT(pstMarks->ulLevle <= MKV_MAX_LEVEL);

    pstMark = MKV_FindMarkByLevel(pstRoot, pstMarks->ulLevle, pstMarks->apszMarkName);
    if (NULL == pstMark)
    {
        RETURN(BS_NO_SUCH);
    }

    pstKey = MKV_FindKeyInMark(pstMark, pszKeyName);
    if (NULL == pstKey)
    {
        RETURN(BS_NO_SUCH);
    }

    *ppszKeyValue = pstKey->pucKeyValue;

    return BS_OK;
}

BS_STATUS MKV_GetKeyValueAsUint64
(
    IN MKV_MARK_S *pstRoot,
    IN MKV_X_PARA_S *pstMarks,
    IN CHAR *pucKeyName,
    OUT UINT64 *puiKeyValue
)
{
    CHAR *pucKeyValue;
    UINT ulRet;
    UINT64 uiKeyValue;

    if ((NULL == pstMarks) || (pucKeyName == NULL) || (puiKeyValue == NULL))
    {
        RETURN(BS_NULL_PARA);
    }

    ulRet = MKV_GetKeyValueAsString(pstRoot, pstMarks, pucKeyName, &pucKeyValue);
    if (BS_OK != ulRet)
    {
        return ulRet;
    }

    if (sscanf(pucKeyValue, "%llu", &uiKeyValue) < 1)
    {
        RETURN(BS_NOT_SUPPORT);
    }
 
    *puiKeyValue = uiKeyValue;
    
    return BS_OK;
}

BS_STATUS MKV_GetKeyValueAsUint
(
    IN MKV_MARK_S *pstRoot,
    IN MKV_X_PARA_S *pstMarks,
    IN CHAR *pucKeyName,
    OUT UINT *puiKeyValue
)
{
    CHAR *pucKeyValue;
    UINT ulRet;
    UINT uiKeyValue;

    if ((NULL == pstMarks) || (pucKeyName == NULL) || (puiKeyValue == NULL))
    {
        RETURN(BS_NULL_PARA);
    }

    ulRet = MKV_GetKeyValueAsString(pstRoot, pstMarks, pucKeyName, &pucKeyValue);
    if (BS_OK != ulRet)
    {
        return ulRet;
    }

    if (sscanf(pucKeyValue, "%u", &uiKeyValue) < 1)
    {
        RETURN(BS_NOT_SUPPORT);
    }
    
    *puiKeyValue = uiKeyValue;
    
    return BS_OK;
}

BS_STATUS MKV_GetKeyValueAsInt
(
    IN MKV_MARK_S *pstRoot,
    IN MKV_X_PARA_S *pstMarks,
    IN CHAR *pucKeyName,
    OUT INT *plKeyValue
)
{
    CHAR *pucKeyValue;
    UINT ulRet;
    INT lKeyValue;

    if ((NULL == pstMarks) || (pucKeyName == NULL) || (plKeyValue == NULL))
    {
        RETURN(BS_NULL_PARA);
    }

    ulRet = MKV_GetKeyValueAsString(pstRoot, pstMarks, pucKeyName, &pucKeyValue);
    if (BS_OK != ulRet)
    {
        return ulRet;
    }

    if (sscanf(pucKeyValue, "%d", &lKeyValue) < 1)
    {
        RETURN(BS_NOT_SUPPORT);
    }
    
    *plKeyValue = lKeyValue;
    
    return BS_OK;
}

BOOL_T MKV_IsKeyExist(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks, IN CHAR *pszKeyName)
{
    CHAR *pszValue;

    if (BS_OK != MKV_GetKeyValueAsString(pstRoot, pstMarks, pszKeyName, &pszValue))
    {
        return FALSE;
    }

    return TRUE;
}


UINT MKV_GetMarkNumInMark(IN MKV_MARK_S *pstMark)
{
    if (NULL == pstMark)
    {
        return 0;
    }

    return DLL_COUNT(&pstMark->stSectionDllHead);
}

UINT MKV_GetSectionNum(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks)
{
    MKV_MARK_S *pstMark;
    
    pstMark = MKV_GetMark(pstRoot, pstMarks);
    if (NULL == pstMark)
    {
        return 0;
    }

    return MKV_GetMarkNumInMark(pstMark);
}


char * MKV_GetMarkDuplicate(IN MKV_MARK_S *pstRoot, IN MKV_X_PARA_S *pstMarks)
{
    MKV_MARK_S *pstMark;
    MKV_MARK_S *node, *tmp;
    
    pstMark = MKV_GetMark(pstRoot, pstMarks);
    if (NULL == pstMark) {
        return NULL;
    }

    DLL_SCAN(&pstMark->stSectionDllHead, node) {
        tmp = node;
        while((tmp = DLL_NEXT(&pstMark->stSectionDllHead, tmp))) {
            if (strcmp(node->pucMarkName, tmp->pucMarkName) == 0) {
                return node->pucMarkName;
            }
        }
    }

    return NULL;
}


UINT MKV_GetKeyNumOfMark(IN MKV_MARK_S *pstMark)
{
    if (NULL == pstMark)
    {
        return 0;
    }

    return DLL_COUNT(&pstMark->stKeyValueDllHead);
}

VOID MKV_WalkMarkInMark(IN MKV_MARK_S *pstRoot, IN PF_MKV_MARK_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    MKV_MARK_S *pstNode, *pstNodeTmp;
    
    DLL_SAFE_SCAN(&pstRoot->stSectionDllHead, pstNode, pstNodeTmp) {
        if (pfFunc(pstRoot, pstNode, hUserHandle) < 0) {
            break;
        }
    }
}

VOID MKV_WalkKeyInMark(IN MKV_MARK_S *pstRoot, IN PF_MKV_KEY_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    MKV_KEY_S *pstKey, *pstKeyTmp;
    
    DLL_SAFE_SCAN(&pstRoot->stKeyValueDllHead, pstKey, pstKeyTmp) {
        if (pfFunc(pstRoot, pstKey, hUserHandle) < 0) {
            break;
        }
    }
}

VOID MKV_ScanProcess
(
    IN MKV_MARK_S *pstMarkRoot,
    IN MKV_MARK_PROCESS_S *pstFuncTbl,
    IN UINT uiFuncTblCount,
    IN VOID *pUserPointer
)
{
    MKV_MARK_S *pstMarkNode;
    UINT i;

    MKV_SCAN_MARK_START(pstMarkRoot, pstMarkNode)
    {
        for (i=0; i<uiFuncTblCount; i++)
        {
            if (strcmp(pstMarkNode->pucMarkName,
                pstFuncTbl[i].pszMarkName) == 0)
            {
                pstFuncTbl[i].pfFunc(pstMarkNode, pUserPointer);
                break;
            }
        }
    }MKV_SCAN_END();
}

