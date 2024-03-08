/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-19
* Description: my conf file type
*  # : 表示后面跟的是注释
*  每行一个独立的描述，格式为: key: property1;property2=xxx;property3
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/mkv_utl.h"
#include "utl/mcf_utl.h"

#define MYCONF_CHAR_REMARK '#'  /* 注释符号 */
#define MYCONF_CHAR_KEY_PRO_SPLIT ':'  /* 分割Key和属性 */
#define MYCONF_CHAR_PRO_SPLIT ';'  /* 分割属性 */
#define MYCONF_CHAR_PRO_VALUE_SPLIT '='  /* 分割属性和Value */

static VOID mcf_ProcLine(IN MCF_HEAD_S *pstHead, IN BOOL_T bSort, IN CHAR *pcLine)
{
    CHAR *pcKey;
    CHAR *pcProp;
    CHAR *pcTmp = NULL;
    CHAR *pcValue;
    CHAR *pcSplit;
    MKV_MARK_S  *pstSecCurrent = NULL;

    if ((pcLine[0] == '\0')
        || (pcLine[0] == MYCONF_CHAR_REMARK)
        || (pcLine[0] == MYCONF_CHAR_KEY_PRO_SPLIT))
    {
        return;
    }

    pcKey = pcLine;

    pcSplit = strchr(pcKey, MYCONF_CHAR_KEY_PRO_SPLIT);
    if (NULL != pcSplit)
    {
        *pcSplit = '\0';
        pcTmp = pcSplit + 1;
    }

    pstSecCurrent = MKV_AddMark2Mark(pstSecCurrent, pcKey, FALSE);
    if (NULL == pstSecCurrent)
    {
        return;
    }

    if (pcTmp == NULL)
    {
        return;
    }

    TXT_SCAN_ELEMENT_BEGIN(pcTmp, MYCONF_CHAR_PRO_SPLIT, pcProp)
    {
        pcValue = "";
        pcSplit = strchr(pcProp, '=');
        if (NULL != pcSplit)
        {
            *pcSplit = '\0';
            pcValue = pcSplit + 1;
        }

        TXT_StrimAndMove(pcProp);
        TXT_StrimAndMove(pcValue);

        MKV_AddNewKey2Mark(pstSecCurrent, pcProp, pcValue, TRUE, bSort);
    }TXT_SCAN_ELEMENT_END();
}


MCF_HANDLE MCF_Open
(
    IN CHAR * pucFileName,
    IN BOOL_T bIsCreateIfNotExist,
    IN BOOL_T bSort,
    IN BOOL_T bReadOnly
)
{
    FILE       *fp = NULL;
    S64       filesize;
    UINT      ulFileNameLen;
    UINT      ulLineLen;
    MCF_HEAD_S *pstHead = NULL;
    CHAR       *pucLineHead = NULL;
    CHAR       *pszOpenFlag = NULL;

    UINT      ulOffset = 0;

    if (NULL == pucFileName)
    {
        return NULL;
    }

    if (bIsCreateIfNotExist == TRUE)
    {
        pszOpenFlag = "ab+";
    }
    else
    {
        pszOpenFlag = "rb";
    }

    filesize = FILE_GetSize(pucFileName);
    if (filesize < 0) {
        filesize = 0;
    }

    fp = FILE_Open(pucFileName, bIsCreateIfNotExist, pszOpenFlag);
    if (NULL == fp)
    {
        return NULL;
    }

    ulFileNameLen = strlen(pucFileName);
    
    pstHead = malloc(sizeof(MCF_HEAD_S) + ulFileNameLen + filesize + 2);
    if (NULL == pstHead)
    {
        fclose(fp);
        return NULL;
    }
    memset(pstHead, 0, (sizeof(MCF_HEAD_S) + ulFileNameLen + filesize + 2));
    DLL_INIT(&pstHead->stSecRoot.stSectionDllHead);
    DLL_INIT(&pstHead->stSecRoot.stKeyValueDllHead);

    pstHead->bIsSort = bSort;
    pstHead->bReadOnly = bReadOnly;
    pstHead->uiMemSize = sizeof(MCF_HEAD_S) + ulFileNameLen + filesize + 2;
    pstHead->pucFileName = (CHAR*)(pstHead) + sizeof(MCF_HEAD_S);
    pstHead->pucFileContent = (CHAR*)(pstHead) + sizeof(MCF_HEAD_S) + ulFileNameLen + 1;
    TXT_Strlcpy(pstHead->pucFileName, pucFileName, strlen(pucFileName) + 1);

    if (filesize != 0) {
        if (fread(pstHead->pucFileContent, 1, filesize, fp) != filesize) {
            fclose(fp);
            return NULL;
        }
    }
    
    fclose(fp);

    /* 检测是否UTF8 */
    if (filesize >= 3)
    {
        if ((pstHead->pucFileContent[0] == (CHAR)0xef)
            && (pstHead->pucFileContent[1] == (CHAR)0xbb)
            && (pstHead->pucFileContent[2] == (CHAR)0xbf))
        {
            pstHead->bIsUtf8 = TRUE;
            ulOffset = 3;
        }
    }

    pstHead->pucFileContent[filesize] = '\0';
    TXT_StrimAndMove(pstHead->pucFileContent);

    TXT_SCAN_LINE_BEGIN(pstHead->pucFileContent + ulOffset, pucLineHead, ulLineLen)
    {
        pucLineHead[ulLineLen] = '\0';
        TXT_StrimAndMove(pucLineHead);

        mcf_ProcLine(pstHead, bSort, pucLineHead);

        
    }TXT_SCAN_LINE_END();


    if (bSort)
    {
        MKV_SortMark(&pstHead->stSecRoot);
    }

    return (HANDLE)pstHead;
}

/*
找不到Prop,返回NULL; 找到Prop,但是无Value,返回""; 

*/
CHAR * MCF_GetProp(IN MCF_HANDLE hMcfHandle, IN CHAR *pcKey, IN CHAR *pcProp)
{
    CHAR *pcValue = NULL;
    MKV_X_PARA_S stTreParam;
    MCF_HEAD_S *pstHead = hMcfHandle;

    stTreParam.apszMarkName[0] = pcKey;
    stTreParam.ulLevle = 1;

    MKV_GetKeyValueAsString(&pstHead->stSecRoot, &stTreParam, pcProp, &pcValue);

    return pcValue;
}



