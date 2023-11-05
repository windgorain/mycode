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
#include "utl/url_acl.h"
#include "utl/exec_utl.h"
#include "utl/mem_cap.h"
#include "utl/url_acl.h"

typedef struct {
    RULE_NODE_S stRuleListNode;
    URL_ACL_RULE_S stRule;
}_URLACL_RULE_DESC_S;

typedef struct {
    PF_URL_ACL_RULE_SCAN pfScan;
    void *pUseData;
}_URL_ACL_SCAN_DESC_S;

typedef struct {
    URL_ACL_MATCH_INFO_S *match_info;
    BOOL_T is_match;
    BS_ACTION_E action;
}_URLACL_MATCH_DESC_S;


static void _url_acl_pcre_compile(IN const CHAR *pcPattern, IN INT iOptions, OUT URL_ACL_PCRE_S *pstPcre)
{
    const CHAR* pcError = NULL;
    INT iErrOffset = 0;

    if ((NULL == pcPattern) || (0 == pcPattern[0]))
    {
        return;
    }

    pstPcre->pstReg = pcre_compile(pcPattern, iOptions, &pcError, &iErrOffset, NULL);
    if (NULL != pstPcre->pstReg)
    {

#if (PCRE_MAJOR >= 8)
        #ifndef PCRE_STUDY_EXTRA_NEEDED
        #define PCRE_STUDY_EXTRA_NEEDED 0
        #endif
        pstPcre->pstExtraData = pcre_study((pcre*)pstPcre->pstReg, PCRE_STUDY_EXTRA_NEEDED, &pcError);
#endif

    }
    return;
}

static void _url_acl_free_rule(IN URL_ACL_RULE_S *pstRule)
{
    DBGASSERT(NULL != pstRule);

    RE_Destroy(pstRule->stRuleCfg.uHostDomain.pstPcre);
    MEM_Free(pstRule->stRuleCfg.uHostDomain.pcPcreStr);

    return;
}


static BOOL_T _url_acl_match_usr_str_pattern(IN const UCHAR *pucUsrString, IN const UCHAR *pucPtnString)
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

 
static BOOL_T _url_acl_match_simp_str(const UCHAR *pucUsrString, const URL_ACL_PATTERN_S *pstPattern)
{
    BOOL_T is_match = BOOL_FALSE;
    INT iRet;
    const UCHAR *pucPatStr;

    pucPatStr = pstPattern->szPattern;

    
    iRet = strcmp((CHAR *)pucUsrString, (CHAR *)pucPatStr);
    if (0 == iRet) {
        is_match = BOOL_TRUE;
    } else if (0 < iRet) {
        is_match = _url_acl_match_usr_str_pattern(pucUsrString, pucPatStr);
    } else {
        is_match = BOOL_FALSE;
    }
    
    return is_match;
}

static void _url_acl_delete_rule(void *rule, void *ud)
{
    LIST_RULE_HANDLE acl = ud;

    _URLACL_RULE_DESC_S *pstUriAclRule = container_of(rule, _URLACL_RULE_DESC_S, stRuleListNode);
    if (pstUriAclRule) {
        _url_acl_free_rule(&pstUriAclRule->stRule);
    }

    MemCap_Free(acl->memcap, pstUriAclRule);
}

static BOOL_T _url_acl_RuleSacnOuter(void *pstRule, void *ud)
{
    UINT uiRuleID;
    _URL_ACL_SCAN_DESC_S *pScanDesc = ud;
    _URLACL_RULE_DESC_S *pstRuleDesc = container_of(pstRule, _URLACL_RULE_DESC_S, stRuleListNode);

    if (NULL != pScanDesc && NULL != pScanDesc->pfScan)
    {
        uiRuleID = Rule_GetIDByNode(&pstRuleDesc->stRuleListNode);
        return pScanDesc->pfScan(uiRuleID, &pstRuleDesc->stRule, pScanDesc->pUseData);
    }

    return BOOL_FALSE;
}

static BOOL_T _url_acl_match_rule(IN void *pstRule, void *ud)
{
    BOOL_T match = BOOL_FALSE;
    const URL_ACL_PATTERN_S *pstPattern;
    const UCHAR *pucStr;
    INT iRet;

    URL_ACL_MATCH_INFO_S *match_info = (URL_ACL_MATCH_INFO_S*)ud;
       
    pucStr = match_info->szDomain;
    pstPattern = &(((URL_ACL_RULE_S*)pstRule)->stRuleCfg.uHostDomain);

    
    if (URL_ACL_PATTERN_STRING == pstPattern->enType) {
        match = _url_acl_match_simp_str(pucStr, pstPattern);
    } else { 
        RE_MATCH_CAPTURE_S m_data;
        m_data.str = (char*) pucStr;
        m_data.str_len = strlen(m_data.str);
        
        iRet = RE_Match(pstPattern->pstPcre, NULL, &m_data);
        if (iRet && m_data.matchCaptureSize > 0) 
        {
            match = BOOL_TRUE;
        }
    }
        
    return match;
}


VOID URL_ACL_KPCRE_Compile2(IN const CHAR* pcPattern, OUT URL_ACL_PCRE_S *pstPcre)
{
    _url_acl_pcre_compile(pcPattern, PCRE_NEWLINE_ANY | PCRE_DOTALL | PCRE_CASELESS, pstPcre);
    return;
}


VOID URL_ACL_KPCRE_Free(IN URL_ACL_PCRE_S *pstPcre)
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


INT URL_ACL_KPCRE_Exec(IN const URL_ACL_PCRE_S *pstPcre, 
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

URL_ACL_HANDLE URL_ACL_Create(void *memcap)
{
    return ListRule_Create(memcap);
}

void URL_ACL_Destroy(URL_ACL_HANDLE acl)
{
    ListRule_Destroy(acl, _url_acl_delete_rule, NULL);
    return;
}

URL_ACL_LIST_HANDLE URL_ACL_CreateList(URL_ACL_HANDLE acl, char *list_name)
{
    return ListRule_CreateList(acl, list_name, 0);
}

void URL_ACL_UpdateRule(URL_ACL_RULE_S *pstRule, URL_ACL_RULE_CFG_S *rule_config)
{
    pstRule->stRuleCfg = *rule_config;
}

void URL_ACL_DestroyList(URL_ACL_HANDLE acl, URL_ACL_LIST_HANDLE list)
{
    ListRule_DestroyList(acl, list, _url_acl_delete_rule, acl);
}

UINT URL_ACL_AddList(URL_ACL_HANDLE acl, char *list_name)
{
    return ListRule_AddList(acl, list_name, 0);
}

void URL_ACL_DelList(URL_ACL_HANDLE acl, UINT list_id)
{
    ListRule_DelListID(acl, list_id, _url_acl_delete_rule, NULL);
}

int URL_ACL_AddListRef(URL_ACL_HANDLE acl, UINT list_id)
{
    return ListRule_IncListRef(acl, list_id);
}

int URL_ACL_DelListRef(URL_ACL_HANDLE acl, UINT list_id)
{
    return ListRule_DecListRef(acl, list_id);
}

UINT URL_ACL_FindListByName(URL_ACL_HANDLE acl, char *list_name)
{
    return ListRule_GetListIDByName(acl, list_name);
}

UINT URL_ACL_GetListByName(URL_ACL_HANDLE acl, char *list_name)
{
    return ListRule_GetListIDByName(acl, list_name);
}

LIST_RULE_LIST_S* URL_ACL_GetListByID(URL_ACL_HANDLE acl, UINT list_id)
{
    return ListRule_GetListByID(acl, list_id);
}

UINT URL_ACL_GetNextRuleID(URL_ACL_HANDLE acl, UINT list_id, UINT curr_rule_id)
{
    RULE_NODE_S *pstNode;

    pstNode = ListRule_GetNextRule(acl, list_id, curr_rule_id);
    if (NULL == pstNode) {
        return 0;
    }

    return pstNode->uiRuleID;
}

UINT URL_ACL_ListGetRef(URL_ACL_HANDLE acl, UINT list_id)
{
    return ListRule_GetListRef(acl, list_id);
}

int URL_ACL_ReplaceList(URL_ACL_HANDLE acl, UINT list_id, URL_ACL_LIST_HANDLE new_list)
{
    return ListRule_ReplaceList(acl, list_id, _url_acl_delete_rule, acl, new_list);
}

void URL_ACL_ScanRule(URL_ACL_HANDLE acl, UINT list_id, PF_URL_ACL_RULE_SCAN pfFunc, void *ud)
{
    LIST_RULE_LIST_S* pstList;
    _URL_ACL_SCAN_DESC_S stScanDesc = {0};

    stScanDesc.pfScan = pfFunc;
    stScanDesc.pUseData = ud;

    pstList = ListRule_GetListByID(acl, list_id);
    if (NULL != pstList) {
        RuleList_ScanRule(&pstList->stRuleList, _url_acl_RuleSacnOuter, &stScanDesc);
    }

    return;
}

BS_ACTION_E URL_ACL_GetDefaultActionByID(URL_ACL_HANDLE acl, UINT list_id)
{
    return ListRule_GetDefaultActionByID(acl, list_id);
}

int URL_ACL_SetDefaultActionByID(URL_ACL_HANDLE acl, UINT list_id, BS_ACTION_E action)
{
    return ListRule_SetDefaultActionByID(acl, list_id, action);
}

UINT URL_ACL_GetNextListID(URL_ACL_HANDLE acl, UINT curr_list_id)
{
    return ListRule_GetNextListID(acl, curr_list_id);
}

char * URL_ACL_GetListNameByID(URL_ACL_HANDLE acl, UINT list_id)
{
    return ListRule_GetListNameByID(acl, list_id);
}

int URL_ACL_MoveRule(URL_ACL_HANDLE acl, UINT list_id, UINT old_rule_id, UINT new_rule_id)
{
    return ListRule_MoveRule(acl, list_id, old_rule_id, new_rule_id);
}

int URL_ACL_RebaseID(URL_ACL_HANDLE acl, UINT list_id, UINT step)
{
    return ListRule_ResetID(acl, list_id, step);
}

int URL_ACL_IncreaseID(URL_ACL_HANDLE acl, UINT list_id, UINT start, UINT end, UINT step)
{
    return ListRule_IncreaseID(acl, list_id, start, end, step);
}

UINT URL_ACL_ConflictRule(IN LIST_RULE_LIST_S* rule_list, IN CHAR* pcRule, IN UINT action)
{
    RULE_NODE_S *pstRuleNode;
    _URLACL_RULE_DESC_S *pstUriAclRule;
    URL_ACL_RULE_S* pstRule = NULL;
    UINT ID = RULE_ID_INVALID;

    while ( NULL != (pstRuleNode = RuleList_GetNextByID(&rule_list->stRuleList, ID))) {
        pstUriAclRule = container_of(pstRuleNode, _URLACL_RULE_DESC_S, stRuleListNode);
        if (pstUriAclRule) {
            pstRule = &(pstUriAclRule->stRule);
            if(strcmp((CHAR*)pstRule->stRuleCfg.uHostDomain.szPattern, pcRule) == 0) {
                return pstUriAclRule->stRuleListNode.uiRuleID;
            }
            ID = pstUriAclRule->stRuleListNode.uiRuleID;
        } else {
            break;
        }
    }

    return RULE_ID_INVALID;
}

void URL_ACL_Reset(URL_ACL_HANDLE acl)
{
    ListRule_Reset(acl, _url_acl_delete_rule, acl);
    return;
}

int URL_ACL_AddRuleToList(LIST_RULE_HANDLE acl, URL_ACL_LIST_HANDLE list,
        UINT rule_id, URL_ACL_RULE_CFG_S *rule_cfg)
{
    _URLACL_RULE_DESC_S *pstRule;
    BS_STATUS eRet;

    pstRule = MemCap_ZMalloc(acl->memcap, sizeof(_URLACL_RULE_DESC_S));
    if (NULL == pstRule) {
        return BS_NO_MEMORY;
    }
    pstRule->stRule.stRuleCfg = *rule_cfg;

    eRet = ListRule_AddRule2List(list, rule_id, &pstRule->stRuleListNode);
    if (BS_OK != eRet) {
        MemCap_Free(acl->memcap, pstRule);
        return eRet;
    }

    return BS_OK;
}

int URL_ACL_AddRule(URL_ACL_HANDLE acl, UINT list_id, UINT rule_id, URL_ACL_RULE_CFG_S *rule_cfg)
{
    _URLACL_RULE_DESC_S *pstRule;
    BS_STATUS eRet;

    pstRule = MemCap_ZMalloc(acl->memcap, sizeof(_URLACL_RULE_DESC_S));
    if (NULL == pstRule) {
        return BS_NO_MEMORY;
    }
    pstRule->stRule.stRuleCfg = *rule_cfg;

    eRet = ListRule_AddRule(acl, list_id, rule_id, &(pstRule->stRuleListNode));
    if (BS_OK != eRet) {
        MemCap_Free(acl->memcap, pstRule);
        return eRet;
    }

    return BS_OK;
}

void URL_ACL_DelRule(URL_ACL_HANDLE acl, UINT list_id, UINT rule_id)
{
    _URLACL_RULE_DESC_S *pstRule;
    
    pstRule = (void*)ListRule_DelRule(acl, list_id, rule_id);
    if (NULL == pstRule) {
        return;
    }

    MemCap_Free(acl->memcap, pstRule);

    return;
}

URL_ACL_RULE_S * URL_ACL_GetRule(LIST_RULE_HANDLE acl, UINT list_id, UINT rule_id)
{
    _URLACL_RULE_DESC_S *pstRule;
    
    pstRule = (void*) ListRule_GetRule(acl, list_id, rule_id);
    if (NULL == pstRule) {
        return NULL;
    }

    return &(pstRule->stRule);
}

BS_ACTION_E URL_ACL_Match(URL_ACL_HANDLE acl, UINT list_id, URL_ACL_MATCH_INFO_S *match_info)
{
    LIST_RULE_LIST_S* pstList;
    _URLACL_MATCH_DESC_S stMatchDesc;

    if (! acl) {
        return BS_ACTION_UNDEF;
    }

    Mem_Zero(&stMatchDesc, sizeof(_URLACL_MATCH_DESC_S));
    stMatchDesc.match_info = match_info;
    stMatchDesc.is_match = BOOL_FALSE;
    stMatchDesc.action = BS_ACTION_UNDEF;

    pstList = ListRule_GetListByID(acl, list_id);
    if (NULL != pstList) {
        
        RuleList_ScanRule(&pstList->stRuleList, _url_acl_match_rule, &stMatchDesc);

        if (!stMatchDesc.is_match) {
            stMatchDesc.action = pstList->default_action;
        }
    }

    return stMatchDesc.action;
}
