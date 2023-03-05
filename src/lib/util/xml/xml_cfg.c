/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-6-5
* Description: XML的 标记、标记名和属性、内容，本程序暂只支持标记和属性. <a key=value />
*                  其中a为标记名,key=value为属性;  key为属性名,value为属性值
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_XMLC

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/stack_utl.h"
#include "utl/xml_cfg.h"
#include "utl/xml_parse.h"

static BS_STATUS xmlc_ToString(IN HSTRING hString, IN MKV_MARK_S *pstMark, IN UINT ulLevle) 
{
    MKV_KEY_S  *pstKey;
    MKV_MARK_S  *pstSecTmp;
    UINT i;
    CHAR *pcChar;
    BS_STATUS eRet = BS_OK;

    for (i=1; i<ulLevle; i++)
    {
        eRet |= STRING_CatFromBuf(hString, "   ");
    }

    eRet |= STRING_CatFromBuf(hString, "<");
    eRet |= STRING_CatFromBuf(hString, pstMark->pucMarkName);

    DLL_SCAN(&pstMark->stKeyValueDllHead, pstKey)
    {
        pcChar = "\"";
        if (strchr(pstKey->pucKeyValue, '\"') != NULL)
        {
            pcChar = "\'";
        }

        eRet |= STRING_CatFromBuf(hString, " ");
        eRet |= STRING_CatFromBuf(hString, pstKey->pucKeyName);
        eRet |= STRING_CatFromBuf(hString, "=");
        eRet |= STRING_CatFromBuf(hString, pcChar);
        eRet |= STRING_CatFromBuf(hString, pstKey->pucKeyValue);
        eRet |= STRING_CatFromBuf(hString, pcChar);
    }

    if (DLL_COUNT(&pstMark->stSectionDllHead) == 0)
    {
        eRet |= STRING_CatFromBuf(hString, " />\r\n");
    }
    else
    {
        eRet |= STRING_CatFromBuf(hString, ">\r\n");

        DLL_SCAN(&pstMark->stSectionDllHead, pstSecTmp)
        {
            eRet |= xmlc_ToString(hString, pstSecTmp, ulLevle + 1);
        }

        for (i=1; i<ulLevle; i++)
        {
            eRet |= STRING_CatFromBuf(hString, "   ");
        }
        eRet |= STRING_CatFromBuf(hString, "</");
        eRet |= STRING_CatFromBuf(hString, pstMark->pucMarkName);
        eRet |= STRING_CatFromBuf(hString, ">\r\n");
    }

    return eRet;
}

static VOID _XMLC_SaveMark(IN FILE *fp, IN MKV_MARK_S *pstMark, IN UINT ulLevle) 
{
    MKV_KEY_S  *pstKey;
    MKV_MARK_S  *pstSecTmp;
    UINT i;
    CHAR cChar;

    for (i=1; i<ulLevle; i++)
    {
        fprintf(fp, "   ");
    }
    fprintf(fp, "<%s", pstMark->pucMarkName);

    DLL_SCAN(&pstMark->stKeyValueDllHead, pstKey)
    {
        cChar = '\"';
        if (strchr(pstKey->pucKeyValue, cChar) != NULL)
        {
            cChar = '\'';
        }
        fprintf(fp, " %s=%c%s%c", pstKey->pucKeyName, cChar, pstKey->pucKeyValue, cChar);
    }

    if (DLL_COUNT(&pstMark->stSectionDllHead) == 0)
    {
        fprintf(fp, " />\r\n");
    }
    else
    {
        fprintf(fp, ">\r\n");

        DLL_SCAN(&pstMark->stSectionDllHead, pstSecTmp)
        {
            _XMLC_SaveMark(fp, pstSecTmp, ulLevle + 1);
        }

        for (i=1; i<ulLevle; i++)
        {
            fprintf(fp, "   ");
        }
        fprintf(fp, "</%s>\r\n", pstMark->pucMarkName);
    }

    return;
}

static VOID _XMLC_Close(IN XMLC_HEAD_S *pstXmlHead)
{
    MKV_MARK_S *pstSecNode = NULL, *pstSecNodeTmp = NULL;

    DLL_SAFE_SCAN (&pstXmlHead->stSecRoot.stSectionDllHead, pstSecNode, pstSecNodeTmp)
    {
        MKV_DelMarkInMark(&pstXmlHead->stSecRoot, pstSecNode);
    }

    if (NULL != pstXmlHead->pucFileName)
    {
        MEM_Free(pstXmlHead->pucFileName);
    }

    free(pstXmlHead);
}

static BS_STATUS _XMLC_ProcessMark(IN XML_PARSE_S *pstXmlParse, IN USER_HANDLE_S *pstUserHandle)
{
    HANDLE hStack = pstUserHandle->ahUserHandle[0];
    MKV_MARK_S  *pstSecCurrent = NULL;
    XMLC_HEAD_S *pstHead = pstUserHandle->ahUserHandle[1];

    if (BS_OK != HSTACK_GetTop(hStack, (HANDLE *)&pstSecCurrent))
    {
        RETURN(BS_ERR);
    }

    *(pstXmlParse->pszStr1 + pstXmlParse->uiStr1Len) = '\0';

    switch (pstXmlParse->eType)
    {
        case XML_TYPE_MARK_NAME:
            if (pstXmlParse->uiStr1Len > 0)
            {
                pstSecCurrent = MKV_AddMark2Mark(pstSecCurrent, pstXmlParse->pszStr1, FALSE);
                if (NULL == pstSecCurrent)
                {
                    RETURN(BS_ERR);
                }
                if (BS_OK != HSTACK_Push(hStack, pstSecCurrent))
                {
                    RETURN(BS_ERR);
                }
            }
            break;

        case XML_TYPE_MARK_KEYVALUE:
            *(pstXmlParse->pszStr2 + pstXmlParse->uiStr2Len) = '\0';
            MKV_AddNewKey2Mark(pstSecCurrent, pstXmlParse->pszStr1, pstXmlParse->pszStr2, FALSE, pstHead->bIsSort);
            break;

        case XML_TYPE_END_MARK:
            HSTACK_Pop(hStack, (HANDLE*)&pstSecCurrent);
            break;

        default:
            break;
    }

	return BS_OK;
}

static BS_STATUS _XMLC_Parse(IN XMLC_HEAD_S *pstXmlHead)
{
    UINT uiOffset = 0;
    HANDLE hStack;
    USER_HANDLE_S stUserHandle;

    hStack = HSTACK_Create(0);
    if (NULL == hStack)
    {
        RETURN(BS_ERR);
    }

    /* 检测是否UTF8 */
    if (pstXmlHead->uiFileSize >= 3)
    {
        if ((pstXmlHead->pucFileContent[0] == (CHAR)0xef)
            && (pstXmlHead->pucFileContent[1] == (CHAR)0xbb)
            && (pstXmlHead->pucFileContent[2] == (CHAR)0xbf))
        {
            pstXmlHead->bIsUtf8 = TRUE;
            uiOffset = 3;
        }
    }

    if (BS_OK != HSTACK_Push(hStack, &(pstXmlHead->stSecRoot)))
    {
        HSTACK_Destory(hStack);
        RETURN(BS_ERR);
    }

    stUserHandle.ahUserHandle[0] = hStack;
    stUserHandle.ahUserHandle[1] = pstXmlHead;

    if (BS_OK != XML_Parse(pstXmlHead->pucFileContent + uiOffset, (PF_XML_PARSE_FUNC)_XMLC_ProcessMark, &stUserHandle))
    {
        HSTACK_Destory(hStack);
        RETURN(BS_ERR);
    }

    return BS_OK;
}

HSTRING XMLC_ToString(IN HANDLE hXmlcHandle)
{
    HSTRING hString;
    XMLC_HEAD_S *pstXmlHead = (XMLC_HEAD_S *)hXmlcHandle;
    MKV_MARK_S  *pstMark;

    hString = STRING_Create();
    if (NULL == hString)
    {
        return NULL;
    }

    DLL_SCAN(&pstXmlHead->stSecRoot.stSectionDllHead, pstMark)
    {
        xmlc_ToString(hString, pstMark, 1);
    }

    return hString;
}

BS_STATUS XMLC_SaveTo(IN HANDLE hXmlcHandle, IN CHAR *pcFileName)
{
    XMLC_HEAD_S *pstXmlHead = (XMLC_HEAD_S *)hXmlcHandle;
    FILE  *fp = NULL;
    MKV_MARK_S  *pstMark;

    if (NULL == pstXmlHead)
    {
        RETURN(BS_NULL_PARA);
    }

    fp = FILE_Open(pcFileName, FALSE, "wb+");
    if (NULL == fp)
    {
        RETURN(BS_CAN_NOT_OPEN);
    }

    if (pstXmlHead->bIsUtf8 == TRUE)
    {
        fprintf(fp, "\xEF\xBB\xBF");
    }

    DLL_SCAN(&pstXmlHead->stSecRoot.stSectionDllHead, pstMark)
    {
        _XMLC_SaveMark(fp, pstMark, 1);
    }

    fclose(fp);

    return BS_OK;
}

BS_STATUS XMLC_Save(IN HANDLE hXmlcHandle)
{
    XMLC_HEAD_S *pstXmlHead = (XMLC_HEAD_S *)hXmlcHandle;

    if (NULL == pstXmlHead)
    {
        RETURN(BS_NULL_PARA);
    }

    if (pstXmlHead->bReadOnly == TRUE)
    {
        RETURN(BS_NO_PERMIT);
    }

    if (pstXmlHead->pucFileName == NULL)
    {
        RETURN(BS_NOT_SUPPORT);
    }

    return XMLC_SaveTo(hXmlcHandle, pstXmlHead->pucFileName);
}

VOID XMLC_Close(IN HANDLE hXmlcHandle)
{
    if (NULL == hXmlcHandle)
    {
        return;
    }

    XMLC_Save(hXmlcHandle);

    _XMLC_Close(hXmlcHandle);
}

HANDLE XMLC_Open
(
    IN CHAR * pucFileName,
    IN BOOL_T bIsCreateIfNotExist,
    IN BOOL_T bSort,
    IN BOOL_T bReadOnly
)
{
    FILE       *fp = NULL;
    UINT64    ulFileSize = 0;
    UINT      ulRet;
    XMLC_HEAD_S *pstXmlHead = NULL;
    CHAR       *pszOpenFlag = NULL;
    CHAR      *pcFileName = NULL;
    ULONG     ulMemLen;
    
    if (NULL != pucFileName)
    {
        if (bIsCreateIfNotExist == TRUE)
        {
            pszOpenFlag = "ab+";
        }
        else
        {
            pszOpenFlag = "rb";
        }

        pcFileName = TXT_Strdup(pucFileName);
        if (NULL == pcFileName)
        {
            return NULL;
        }

        ulRet = FILE_GetSize(pucFileName, &ulFileSize);
        if (BS_OK != ulRet)
        {
            ulFileSize = 0;
        }

        fp = FILE_Open(pucFileName, bIsCreateIfNotExist, pszOpenFlag);
        if (NULL == fp)
        {
            MEM_Free(pcFileName);
            return NULL;
        }
    }

    ulMemLen = sizeof(XMLC_HEAD_S) + (ULONG)ulFileSize + 1;
    pstXmlHead = malloc(ulMemLen);
    if (NULL == pstXmlHead)
    {
        fclose(fp);
        if (NULL != pcFileName)
        {
            MEM_Free(pcFileName);
        }
        return NULL;
    }
    memset(pstXmlHead, 0, ulMemLen);
    DLL_INIT(&pstXmlHead->stSecRoot.stSectionDllHead);
    DLL_INIT(&pstXmlHead->stSecRoot.stKeyValueDllHead);

    pstXmlHead->bIsSort = bSort;
    pstXmlHead->bReadOnly = bReadOnly;
    pstXmlHead->uiFileSize = (UINT)ulFileSize;
    pstXmlHead->ulMemSize = ulMemLen;
    pstXmlHead->pucFileName = pcFileName;
    pstXmlHead->pucFileContent = (CHAR*)(pstXmlHead) + sizeof(XMLC_HEAD_S);

    if (fp != NULL)
    {
        if (ulFileSize != 0)
        {
            if (fread(pstXmlHead->pucFileContent, 1, (UINT)ulFileSize, fp) != ulFileSize) {
                fclose(fp);
                _XMLC_Close(pstXmlHead);
                return NULL;
            }
        }
        fclose(fp);
    }

    pstXmlHead->pucFileContent[ulFileSize] = '\0';

    if (BS_OK != _XMLC_Parse(pstXmlHead))
    {
        _XMLC_Close(pstXmlHead);
        return NULL;
    }

    if (bSort)
    {
        MKV_SortMark(&pstXmlHead->stSecRoot);
    }

    return (HANDLE)pstXmlHead;
}

BS_STATUS XMLC_DelKeyInMark(IN MKV_MARK_S *pstMark, IN MKV_KEY_S *pstKey)
{    
    return MKV_DelKeyInMark(pstMark, pstKey);
}

BS_STATUS XMLC_DelKey(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections, IN CHAR *pszKey)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;

    return MKV_DelKey(&pstHead->stSecRoot, pstSections, pszKey);
}

BS_STATUS XMLC_DelAllKeyOfMark(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;

    return MKV_DelAllKeyOfMark(&pstHead->stSecRoot, pstSections);
}

BS_STATUS XMLC_DelMark(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;

    return MKV_DelMark(&pstHead->stSecRoot, pstSections);
}

/* 覆盖已经重复的Mark */
BS_STATUS XMLC_AddMark(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;

    return MKV_AddMark(&pstHead->stSecRoot, pstSections, pstHead->bIsSort);
}

/* 不覆盖已经重复的Mark, 而是生成一个新的 */
MKV_MARK_S * XMLC_AddMark2Mark(IN HANDLE hXmlcHandle, IN MKV_MARK_S *pstMark, IN CHAR *pcMark)
{
    return MKV_AddMark2Mark(pstMark, pcMark, TRUE);
}

MKV_MARK_S * XMLC_FindMarkInMark(IN HANDLE hXmlcHandle, IN MKV_MARK_S *pstMark, IN CHAR *pcMark)
{
    return MKV_FindMarkInMark(pstMark, pcMark);
}

MKV_MARK_S * XMLC_GetMark(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections)
{
    XMLC_HEAD_S *pstXmlHead = (XMLC_HEAD_S*)hXmlcHandle;

    return MKV_GetMark(&pstXmlHead->stSecRoot, pstSections);
}

BOOL_T XMLC_IsMarkExist(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections)
{
    if (NULL == XMLC_GetMark(hXmlcHandle, pstSections))
    {
        return FALSE;
    }

    return TRUE;
}

CHAR * XMLC_GetNextMarkInMark(IN HANDLE hXmlcHandle, IN MKV_MARK_S *pstMarkRoot, IN CHAR *pcCurSecName)
{
    return MKV_GetNextMarkInMark(pstMarkRoot, pcCurSecName);
}

CHAR * XMLC_GetNextMark(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections, IN CHAR *pcCurSecName)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S*)hXmlcHandle;

    return MKV_GetNextMark(&pstHead->stSecRoot, pstSections, pcCurSecName);
}

CHAR * XMLC_GetMarkByIndex(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections, IN UINT uiIndex/* 从0开始计算 */)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S*)hXmlcHandle;

    return MKV_GetMarkByIndex(&pstHead->stSecRoot, pstSections, uiIndex);
}

BS_STATUS XMLC_GetNextKeyInMark(IN MKV_MARK_S *pstMarkRoot, INOUT CHAR **ppszKeyName)
{
    return MKV_GetNextKeyInMark(pstMarkRoot, ppszKeyName);
}

BS_STATUS XMLC_GetNextKey(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections, INOUT CHAR **ppszKeyName)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S*)hXmlcHandle;

    return MKV_GetNextKey(&pstHead->stSecRoot, pstSections, ppszKeyName);
}

BS_STATUS XMLC_SetKeyValueAsString
(
    IN HANDLE hXmlcHandle,
    IN MKV_X_PARA_S *pstSections,
    IN CHAR *pszKeyName,
    IN CHAR *pszValue
)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;
    return MKV_SetKeyValueAsString(&pstHead->stSecRoot, pstSections, pszKeyName, pszValue, pstHead->bIsSort);
}

BS_STATUS XMLC_SetKeyValueAsUlong
(
    IN HANDLE hXmlcHandle, 
    IN MKV_X_PARA_S *pstSections,
    IN CHAR *pucKeyName,
    IN UINT ulKeyValue
)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;
    return MKV_SetKeyValueAsUint(&pstHead->stSecRoot, pstSections, pucKeyName, ulKeyValue, pstHead->bIsSort);
}

BS_STATUS XMLC_GetKeyValueAsString(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections, IN CHAR *pszKeyName, OUT CHAR **ppszKeyValue)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;
    return MKV_GetKeyValueAsString(&pstHead->stSecRoot, pstSections, pszKeyName, ppszKeyValue);
}

BS_STATUS XMLC_GetKeyValueAsUint
(
    IN HANDLE hXmlcHandle,
    IN MKV_X_PARA_S *pstSections,
    IN CHAR *pucKeyName,
    OUT UINT *pulKeyValue
)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;

    return MKV_GetKeyValueAsUint(&pstHead->stSecRoot, pstSections, pucKeyName, pulKeyValue);
}

BS_STATUS XMLC_GetKeyValueAsInt(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections, IN CHAR *pucKeyName, OUT INT *plKeyValue)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;
    return MKV_GetKeyValueAsInt(&pstHead->stSecRoot, pstSections, pucKeyName, plKeyValue);
}

BOOL_T XMLC_IsKeyExist(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections, IN CHAR *pszKeyName)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;
    return MKV_IsKeyExist(&pstHead->stSecRoot, pstSections, pszKeyName);
}

/* 返回section的个数 */
UINT XMLC_GetMarkNumInMark(IN MKV_MARK_S *pstMark)
{
    return MKV_GetMarkNumInMark(pstMark);
}

UINT XMLC_GetMarkNum(IN HANDLE hXmlcHandle, IN MKV_X_PARA_S *pstSections)
{
    XMLC_HEAD_S *pstHead = (XMLC_HEAD_S *)hXmlcHandle;
    return MKV_GetSectionNum(&pstHead->stSecRoot, pstSections);
}

/* 返回section中属性的个数 */
UINT XMLC_GetKeyNumOfMark(IN MKV_MARK_S *pstMark)
{
    return MKV_GetKeyNumOfMark(pstMark);
}

VOID XMLC_WalkMarkInMark(IN MKV_MARK_S *pstMarkRoot, IN PF_MKV_MARK_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    MKV_WalkMarkInMark(pstMarkRoot, pfFunc, hUserHandle);
}

VOID XMLC_WalkKeyInMark(IN MKV_MARK_S *pstMarkRoot, IN PF_MKV_KEY_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    MKV_WalkKeyInMark(pstMarkRoot, pfFunc, hUserHandle);
}

BS_STATUS XMLC_SetKeyValueInMark(IN MKV_MARK_S *pstMark, IN CHAR *pcKey, IN CHAR *pcValue)
{
    if (NULL == MKV_SetKeyValueInMark(pstMark, pcKey, pcValue, TRUE, TRUE))
    {
        return BS_ERR;
    }

    return BS_OK;
}

CHAR * XMLC_GetKeyValueInMark(IN MKV_MARK_S *pstMark, IN CHAR *pcKey)
{
    MKV_KEY_S *pstKeyValue;
    
    pstKeyValue = MKV_FindKeyInMark(pstMark, pcKey);
    if (NULL == pstKeyValue)
    {
        return NULL;
    }

    return pstKeyValue->pucKeyValue;
}

