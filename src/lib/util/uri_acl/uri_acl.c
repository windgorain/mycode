/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "pcre.h"

#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/eth_utl.h"
#include "utl/lstr_utl.h"
#include "utl/nap_utl.h"
#include "utl/bit_opt.h"
#include "utl/uri_acl.h"

/*****************************************************************************
  Description: 是否是简单字符串
      Caution: 通配符包括"*"、"?"、"%" 
*****************************************************************************/
STATIC inline BOOL_T uri_acl_IsSimpStr(IN const CHAR *pcStrStart, IN const CHAR *pcStrEnd)
{
    BOOL_T bIsSimpStr = BOOL_TRUE;

    while (pcStrStart < pcStrEnd) {
        if ((*pcStrStart == '*') || (*pcStrStart == '?') || (*pcStrStart == '%')) {
            bIsSimpStr = BOOL_FALSE;
            break;
        }
        pcStrStart++;
    }
    
    return bIsSimpStr;
}

/*****************************************************************************
  Description: 解析输入的字符类型
*****************************************************************************/
STATIC inline URI_ACL_CHAR_TYPE_E uri_acl_GetCharType(IN CHAR cChar)
{
    URI_ACL_CHAR_TYPE_E enType;
    
    switch(cChar) {
        /* 通配符 */
        case '*':
        case '?':
        case '%': {
            enType = URI_ACL_CHAR_WILDCARD;
            break;
        }
        /* 需要转义的字符 */
        case '.': {
            enType = URI_ACL_CHAR_NEEDTRANSFER;
            break;
        }
        /* 普通字符 */
        default: {
            enType = URI_ACL_CHAR_NORMAL;
            break;
        }
    }
    
    return enType;
}

/*****************************************************************************
  Description: 转换通配符为正则的形式
*****************************************************************************/
STATIC inline CHAR *uri_acl_TransWildCard2Reg(IN CHAR cChar)
{
    CHAR *pcString = "";
    
    if (cChar == '*') {
        pcString = ".*";
    } else if (cChar == '?') {
        pcString = ".";
    } else { /* cChar == '%' */
        pcString = "[^./]*";
    }
    
    return pcString;
}

/*****************************************************************************
  Description: pcre 编译处理
*****************************************************************************/
STATIC VOID uri_acl_pcre_Compile(IN const CHAR *pcPattern, IN INT iOptions, OUT URI_ACL_PCRE_S *pstPcre)
{
    const CHAR* pcError = NULL;
    INT iErrOffset = 0;

    if ((NULL == pcPattern) || (0 == pcPattern[0])) {
        return;
    }

    pstPcre->pstReg = pcre_compile(pcPattern, iOptions, &pcError, &iErrOffset, NULL);
    if (NULL != pstPcre->pstReg) {
#ifndef PCRE_STUDY_EXTRA_NEEDED
    #define PCRE_STUDY_EXTRA_NEEDED 0
#endif
#if (PCRE_MAJOR >= 8)
        pstPcre->pstExtraData = pcre_study((pcre*)pstPcre->pstReg, PCRE_STUDY_EXTRA_NEEDED, &pcError);
#endif
    }
}

/*****************************************************************************
  Description: pcre 高级编译不区分大小写
*****************************************************************************/
VOID URI_ACL_KPCRE_Compile2(IN const CHAR* pcPattern, OUT URI_ACL_PCRE_S *pstPcre)
{
    uri_acl_pcre_Compile(pcPattern, PCRE_NEWLINE_ANY | PCRE_DOTALL | PCRE_CASELESS, pstPcre);
    return;
}

/*****************************************************************************
  Description: 正则释放
*****************************************************************************/
VOID URI_ACL_KPCRE_Free(IN URI_ACL_PCRE_S *pstPcre)
{
    if (NULL != pstPcre->pstReg)
    {
        pcre_free((pcre*)pstPcre->pstReg);
        pstPcre->pstReg = NULL;
    }

#if (PCRE_MAJOR >= 8)
    if (NULL != pstPcre->pstExtraData)
    {
        pcre_free_study((pcre_extra*)pstPcre->pstExtraData);
        pstPcre->pstExtraData = NULL;
    }
#endif

    return;
}

/*****************************************************************************
  Description: 正则查找
       Return: 匹配的个数
*****************************************************************************/
INT URI_ACL_KPCRE_Exec(IN const URI_ACL_PCRE_S *pstPcre, 
                       IN const UCHAR* pucStr, 
                       IN INT iLeng, 
                       IN INT iOffsetCount, 
                       OUT INT *piOffsets)
{
    return pcre_exec((pcre*)pstPcre->pstReg, 
                     (pcre_extra*)pstPcre->pstExtraData, 
                     (CHAR*)pucStr, iLeng, 0, PCRE_NEWLINE_ANY, 
                     piOffsets, iOffsetCount);
}

/*****************************************************************************
  Description: 解析模式字符串
*****************************************************************************/
STATIC ULONG uri_acl_ProcPattern(IN const CHAR *pcStrCur, IN const CHAR *pcStrEnd, OUT URI_ACL_PATTERN_S *pstPattern)
{
    CHAR *pcRegString; 
    CHAR *pcPattern;
    UINT uiOffset;
    URI_ACL_CHAR_TYPE_E enCharType;  
    URI_ACL_PCRE_S *pstPcre = &(pstPattern->stPcre);

    pcPattern = MEM_ZMalloc(URI_ACL_PATTERN_BUF_MAX);
    if (NULL == pcPattern)
    {
        return ERROR_FAILED;
    }
    
    /* 通配符模式 */
    uiOffset = 0;
    while (pcStrCur < pcStrEnd)
    {
        /* 解析输入的字符类型 */
        enCharType = uri_acl_GetCharType(*pcStrCur);
        
        /* 通配符 */
        if (URI_ACL_CHAR_WILDCARD == enCharType)
        {            
            /* 转换为正则形式 */
            pcRegString = uri_acl_TransWildCard2Reg(*pcStrCur);
            uiOffset += (UINT)snprintf(pcPattern + uiOffset, URI_ACL_PATTERN_BUF_MAX - uiOffset, "%s", pcRegString);
        }        
        /* 需要转义的字符 */
        else if (URI_ACL_CHAR_NEEDTRANSFER == enCharType) 
        {
            uiOffset += (UINT)snprintf(pcPattern + uiOffset, URI_ACL_PATTERN_BUF_MAX - uiOffset, "\\%c", *pcStrCur);
        }  
        /* 普通字符 */
        else 
        {
            uiOffset += (UINT)snprintf(pcPattern + uiOffset, URI_ACL_PATTERN_BUF_MAX - uiOffset, "%c", *pcStrCur);
        }
        pcStrCur++;
    }

    pstPattern->pcPcreStr = pcPattern;
    
    URI_ACL_KPCRE_Compile2(pcPattern, pstPcre); 
        
    return ERROR_SUCCESS;    
}

STATIC URI_ACL_RULE_S * uri_acl_AllocRule()
{
    URI_ACL_RULE_S *pstRule;
    
    /* 分配URI_ACL_KNODE_S */
    pstRule = MEM_ZMalloc(sizeof(URI_ACL_RULE_S));

    return pstRule;
}

STATIC VOID uri_acl_FreeRule(IN URI_ACL_RULE_S *pstRule)
{
    DBGASSERT(NULL != pstRule);

    URI_ACL_KPCRE_Free(&pstRule->stPattern.stPcre);
    MEM_Free(pstRule->stPattern.pcPcreStr);
    /* 释放Rule */
    MEM_Free(pstRule);

    return;
}

/*****************************************************************************
  Description: 匹配字符串
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
      Caution: 1、 usr: http://1.2.3.1/path1/path2
                  rule: http://1.2.3.1/path1/
                    ----matched
               2、 usr: http://1.2.3.1/path1/
                  rule: http://1.2.3.1/path1
                    ----not-matched
               3、 usr: http://1.2.3.1/path313/ 或者
                        http://1.2.3.1/path313
                  rule: http://1.2.3.1/path
                    ----not-matched
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchUsrStrGtPtnStr(IN const UCHAR *pucUsrString, IN const UCHAR *pucPtnString)
{
    ULONG ulPtnStrLen;
    INT iRet;

    ulPtnStrLen = strlen((CHAR *)pucPtnString);

    iRet = strncmp((CHAR *)pucUsrString, (CHAR *)pucPtnString, ulPtnStrLen);
    if (0 != iRet)
    {
        return FALSE;
    }

    if (pucPtnString[ulPtnStrLen - 1] != '/')
    {
        return FALSE;
    }
    
    return TRUE;
}

/*****************************************************************************
  Description: 匹配字符串
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchSimpStr(IN const UCHAR *pucUsrString, IN const URI_ACL_PATTERN_S *pstPattern)
{
    BOOL_T bIsMatch = BOOL_FALSE;
    INT iRet;
    const UCHAR *pucPatStr;

    pucPatStr = pstPattern->szPattern;

    /* 要求path区分大小写 */
    iRet = strcmp((CHAR *)pucUsrString, (CHAR *)pucPatStr);
    if (0 == iRet)
    {
        bIsMatch = BOOL_TRUE;
    }
    else if (0 < iRet) /* pucUsrString长于pattern*/
    {   
        bIsMatch = uri_acl_MatchUsrStrGtPtnStr(pucUsrString, pucPatStr);
    }
    else /* pucUsrString短于pattern */
    {
        bIsMatch = BOOL_FALSE;
    }
    
    return bIsMatch;
}

static VOID uri_acl_DeleteRule(IN void *pstRule, IN VOID *pUserHandle)
{
    URI_ACL_RULE_S *pstUriAclRule = container_of(pstRule, URI_ACL_RULE_S, stListRuleNode);

    uri_acl_FreeRule(pstUriAclRule);
}

LIST_RULE_HANDLE URI_ACL_Create(void *memcap)
{
    return ListRule_Create(memcap);
}

VOID URI_ACL_Destroy(IN LIST_RULE_HANDLE hCtx)
{
    ListRule_Destroy(hCtx, uri_acl_DeleteRule, NULL);

    return;
}

UINT URI_ACL_AddList(IN LIST_RULE_HANDLE hCtx, IN CHAR *pcListName)
{
    return ListRule_AddList(hCtx, pcListName, 0);
}

VOID URI_ACL_DelList(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID)
{
    ListRule_DelListID(hCtx, uiListID, uri_acl_DeleteRule, NULL);
}

BS_STATUS URI_ACL_AddListRef(IN LIST_RULE_HANDLE hURIAcl, IN UINT uiListID)
{
    return ListRule_IncListRef(hURIAcl, uiListID);
}

BS_STATUS URI_ACL_DelListRef(IN LIST_RULE_HANDLE hURIAcl, IN UINT uiListID)
{
    return ListRule_DecListRef(hURIAcl, uiListID);
}

UINT URI_ACL_FindListByName(IN LIST_RULE_HANDLE hCtx, IN CHAR *pcListName)
{
    return ListRule_GetListIDByName(hCtx, pcListName);
}

LIST_RULE_LIST_S* URI_ACL_GetListByName(IN LIST_RULE_HANDLE hURIHandle, IN CHAR *pcListName)
{
    return ListRule_GetListByName(hURIHandle, pcListName);
}

LIST_RULE_LIST_S* URI_ACL_GetListByID(IN LIST_RULE_HANDLE hURIHandle, UINT list_id)
{
    return ListRule_GetListByID(hURIHandle, list_id);
}

UINT URI_ACL_GetNextRuleID(IN LIST_RULE_HANDLE hURIAcl, IN UINT uiListID, IN UINT uiCurrentRuleID)
{
    RULE_NODE_S *pstNode;

    pstNode = ListRule_GetNextRule(hURIAcl, uiListID, uiCurrentRuleID);
    if (NULL == pstNode)
    {
        return 0;
    }

    return pstNode->uiRuleID;
}

UINT URI_ACL_ConflictRule(IN LIST_RULE_LIST_S* rule_list, IN CHAR* pcRule, IN UINT action)
{
    RULE_NODE_S *pstRuleNode;
    URI_ACL_RULE_S* pstRule;
    UINT ID = RULE_ID_INVALID;

    while(NULL != (pstRuleNode = RuleList_GetNextByID(&rule_list->stRuleList, ID))) {
        pstRule = container_of(pstRuleNode, URI_ACL_RULE_S, stListRuleNode);
        if(strcmp((CHAR*)pstRule->stPattern.szPattern, pcRule) == 0) {
            return pstRule->stListRuleNode.uiRuleID;
        }
        ID = pstRule->stListRuleNode.uiRuleID;
    }

    return RULE_ID_INVALID;
}


STATIC ULONG uri_acl_ParseURI(IN const CHAR *pcURI, 
                                 IN ULONG ulURILen,
                                 INOUT URI_ACL_PATTERN_S *pattern)
{
    const CHAR *pcStart = pcURI;
    const CHAR *pcEnd = pcStart + ulURILen; 
  
    BOOL_T bIsSimpStr; 
    ULONG ulRet = ERROR_SUCCESS;
    
    bIsSimpStr = uri_acl_IsSimpStr(pcStart, pcEnd); 
    /* 简单字符串方式，精确匹配 */
    if (BOOL_TRUE == bIsSimpStr)
    {
        pattern->enType = URI_ACL_PATTERN_STRING;
        memcpy(pattern->szPattern, pcStart, ulURILen);
        pattern->szPattern[ulURILen] = '\0';
    }
    /* 通配符模式 */
    else
    {        
        pattern->enType = URI_ACL_PATTERN_PCRE;        
        ulRet = uri_acl_ProcPattern(pcStart, pcEnd, pattern);          
    }                   
    
    return ulRet;
}

BS_STATUS URI_ACL_AddRule(IN LIST_RULE_HANDLE hCtx, IN LIST_RULE_LIST_S* rule_list, IN UINT uiRuleID, IN CHAR *pcRule, IN CHAR* Action)
{
    URI_ACL_RULE_S *pstRule;
    UINT action;
    UINT conflict_id;

    if (strlen(pcRule) > URI_ACL_RULE_MAX_LEN)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    if(Action[0] == 'p') {
        action = BS_ACTION_PERMIT; 
    }else {
        action = BS_ACTION_DENY;
    }
    
    if(RuleList_GetRule(&rule_list->stRuleList, uiRuleID)) {
        RETURNI(BS_ALREADY_EXIST, "rule %d exists!\r\n", uiRuleID);
    }

    conflict_id = URI_ACL_ConflictRule(rule_list, pcRule, action);
    if(conflict_id != RULE_ID_INVALID){
        RETURNI(BS_CONFLICT, "confilict with rule %d!\r\n",conflict_id);
    }

    /* 创建rule */
    pstRule = uri_acl_AllocRule();
    if (NULL == pstRule)
    {
        return BS_NO_MEMORY;
    }
    
    
    TXT_Strlcpy((CHAR*)pstRule->stPattern.szPattern, pcRule, sizeof(pstRule->stPattern.szPattern));
    uri_acl_ParseURI(pcRule, strlen(pcRule), &pstRule->stPattern);
    pstRule->action = action;
    RuleList_Add(&rule_list->stRuleList, uiRuleID, &pstRule->stListRuleNode);
    
    return BS_OK;
}

BS_STATUS URI_ACL_AddRuleToListID(IN LIST_RULE_HANDLE hCtx, UINT uiListID, IN UINT uiRuleID, IN CHAR *pcRule, IN CHAR* Action)
{
    LIST_RULE_LIST_S *list = URI_ACL_GetListByID(hCtx, uiListID);
    if (! list) {
        RETURN(BS_ERR);
    }

    return URI_ACL_AddRule(hCtx, list, uiRuleID, pcRule, Action);
}

VOID URI_ACL_DelRule(LIST_RULE_LIST_S* pstList, IN UINT uiRuleID)
{
    RULE_NODE_S *pstListRuleNode;
    pstListRuleNode = RuleList_Del(&pstList->stRuleList, uiRuleID);
    uri_acl_DeleteRule(pstListRuleNode, NULL);
}

URI_ACL_RULE_S * URI_ACL_GetRule(IN LIST_RULE_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiRuleID)
{

    URI_ACL_RULE_S *pstRule;
    
    pstRule = (VOID*) ListRule_GetRule(hIpAcl, uiListID, uiRuleID);
    if (NULL == pstRule)
    {
        return NULL;
    }

    return pstRule;

}

STATIC BOOL_T uri_acl_MatchRule(IN const URI_ACL_MATCH_INFO_S *pstMatchInfo, IN const URI_ACL_RULE_S *pstRule)
{
    BOOL_T bIsMatch = BOOL_FALSE;
    const URI_ACL_PATTERN_S *pstPattern;
    const UCHAR *pucStr;
    INT iRet;
    INT aiOvector[2] = {-1,-1};
       
    pucStr = pstMatchInfo->szDomain;
    pstPattern = &(pstRule->stPattern);

    /* 简单字符串 */
    if (URI_ACL_PATTERN_STRING == pstPattern->enType)
    {
        bIsMatch = uri_acl_MatchSimpStr(pucStr, pstPattern);
    }
    /* 正则 */
    else
    {
        iRet = URI_ACL_KPCRE_Exec(&(pstPattern->stPcre), 
                                  pucStr, 
                                  (INT)strlen((CHAR *)pucStr), 
                                  2, aiOvector);
        /* 能够匹配且从起始处完全匹配 */
        if ((iRet >= 0) && (0 == aiOvector[0]))
        {
            bIsMatch = BOOL_TRUE;
        }    
    }
        
    return bIsMatch;
}

BS_STATUS URI_ACL_Match
(
    IN LIST_RULE_HANDLE hCtx,
    IN UINT uiListID,
    IN URI_ACL_MATCH_INFO_S *pstMatchInfo, 
    OUT BS_ACTION_E *penAction
)
{
    RULE_NODE_S *pstListRuleNode;
    URI_ACL_RULE_S *pstRule;
    BOOL_T bIsMatch = BOOL_TRUE;
    BS_STATUS eRet = BS_NOT_FOUND;
    UINT uiRuleID = RULE_ID_INVALID;

    *penAction = BS_ACTION_DENY;

    while ((pstListRuleNode = ListRule_GetNextRule(hCtx, uiListID, uiRuleID)) != NULL)
    {
        uiRuleID = pstListRuleNode->uiRuleID;
        pstRule = container_of(pstListRuleNode, URI_ACL_RULE_S, stListRuleNode);
        /* 按rule匹配 */
        bIsMatch = uri_acl_MatchRule(pstMatchInfo, pstRule);
        if (BOOL_TRUE == bIsMatch)
        {
            *penAction = pstRule->action;
            printf("match rule %d action %d\r\n", uiRuleID, pstRule->action);
            eRet = ERROR_SUCCESS;
            break;
        }   
    }

    return eRet;
}


