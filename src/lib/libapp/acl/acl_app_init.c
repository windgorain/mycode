/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/socket_utl.h"
#include "utl/ip_acl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/list_rule.h"
#include "utl/bit_opt.h"
#include "comp/comp_acl.h"

static IPACL_HANDLE g_hAclAppIpAcl;
static MUTEX_S g_stAclAppMutex;
static COMP_ACL_S g_stAclAppComp;

static BS_STATUS _aclappip_GetListIDByName(IN ACL_NAME_ID_S *pstNameID)
{
    UINT64 ulListID;
    
    MUTEX_P(&g_stAclAppMutex);
    ulListID = IPACL_GetListByName(g_hAclAppIpAcl, pstNameID->pcAclListName);
    MUTEX_V(&g_stAclAppMutex);

    pstNameID->ulAclListID = ulListID;

    if (ulListID == ACL_INVALID_LIST_ID)
    {
        return BS_NOT_FOUND;
    }

    return BS_OK;
}

static BS_ACTION_E _aclappip_Match(IN UINT64 ulListID, IN IPACL_MATCH_INFO_S *pstMatchInfo)
{
    BS_ACTION_E eRet;
    
    MUTEX_P(&g_stAclAppMutex);
    eRet = IPACL_Match(g_hAclAppIpAcl, ulListID, pstMatchInfo);
    MUTEX_V(&g_stAclAppMutex);

    return eRet;
}

/* 增加List引用计数 */
static BS_STATUS _aclappip_AddListRef(IN CHAR *pcListName)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stAclAppMutex);
    eRet = IPACL_AddListRef(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName));
    MUTEX_V(&g_stAclAppMutex);

    return eRet;
}

static BS_STATUS _aclappip_DelListRef(IN CHAR *pcListName)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stAclAppMutex);
    eRet = IPACL_DelListRef(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName));
    MUTEX_V(&g_stAclAppMutex);

    return eRet;
}

static BS_STATUS _aclappip_DelListRefByID(IN UINT64 *pulListID)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stAclAppMutex);
    eRet = IPACL_DelListRef(g_hAclAppIpAcl, *pulListID);
    MUTEX_V(&g_stAclAppMutex);

    return eRet;
}

static BS_STATUS _aclapp_Ioctl(IN COMP_ACL_IOCTL_E enCmd, IN VOID *pData)
{
    BS_STATUS eRet = BS_OK;

    switch (enCmd)
    {
        case COMP_ACL_IOCTL_ADD_LIST_REF:
        {
            eRet = _aclappip_AddListRef(pData);
            break;
        }
        case COMP_ACL_IOCTL_DEL_LIST_REF:
        {
            eRet = _aclappip_DelListRef(pData);
            break;
        }
        case COMP_ACL_IOCTL_DEL_LIST_REF_BY_ID:
        {
            _aclappip_DelListRefByID(pData);
            break;
        }
        case COMP_ACL_IOCTL_GET_LIST_ID:
        {
            eRet = _aclappip_GetListIDByName(pData);
            break;
        }
        default:
        {
            eRet = BS_NOT_SUPPORT;
            break;
        }
    }

    return eRet;
}

static VOID _aclapp_InitComp()
{
    g_stAclAppComp.pfIoctl = _aclapp_Ioctl;
    g_stAclAppComp.pfIpAclMatch = _aclappip_Match;
    g_stAclAppComp.comp.comp_name = COMP_ACL_NAME;

    COMP_Reg(&g_stAclAppComp.comp);
}

static BS_STATUS _aclapp_EnterListView(IN CHAR *pcListName)
{
    UINT uiListID;

    uiListID = IPACL_GetListByName(g_hAclAppIpAcl, pcListName);
    if (uiListID == 0)
    {
        uiListID = IPACL_AddList(g_hAclAppIpAcl, pcListName);
    }

    if (uiListID == 0)
    {
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS _aclapp_EnterRuleView(IN CHAR *pcListName, IN UINT uiRuleID)
{
    UINT64 ulListID;
    IPACL_RULE_CFG_S stRuleCfg = {0};

    ulListID = IPACL_GetListByName(g_hAclAppIpAcl, pcListName);
    if (0 == ulListID)
    {
        return BS_ERR;
    }

    if (NULL != IPACL_GetRule(g_hAclAppIpAcl, ulListID, uiRuleID))
    {
        return BS_OK;
    }

    stRuleCfg.enAction = BS_ACTION_DENY;

    return IPACL_AddRule(g_hAclAppIpAcl, ulListID, uiRuleID, &stRuleCfg);
}

static BS_STATUS _aclapp_CmdAction(IN CHAR *pcListName, IN UINT uiRuleID, IN BS_ACTION_E eAction)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.enAction = eAction;

    return BS_OK;
}

static BS_STATUS _aclapp_CmdEnable(IN CHAR *pcListName, IN UINT uiRuleID, IN BOOL_T bEnable)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.bEnable = bEnable;

    return BS_OK;
}

static BS_STATUS _aclapp_CmdDIP(IN CHAR *pcListName, IN UINT uiRuleID, IN UINT uiIP, IN UINT uiWildcard)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.stDIP.uiIP = uiIP;
    pstRule->stRuleCfg.stDIP.uiWildcard = uiWildcard;
    pstRule->stRuleCfg.uiKeyMask |= IPACL_KEY_DIP;

    return BS_OK;
}

static BS_STATUS _aclapp_CmdSIP(IN CHAR *pcListName, IN UINT uiRuleID, IN UINT uiIP, IN UINT uiWildcard)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.stSIP.uiIP = uiIP;
    pstRule->stRuleCfg.stSIP.uiWildcard = uiWildcard;
    pstRule->stRuleCfg.uiKeyMask |= IPACL_KEY_SIP;

    return BS_OK;
}

static BS_STATUS _aclapp_CmdNoSIP(IN CHAR *pcListName, IN UINT uiRuleID)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.stSIP.uiIP = 0;
    pstRule->stRuleCfg.stSIP.uiWildcard = 0;
    BIT_CLR(pstRule->stRuleCfg.uiKeyMask, IPACL_KEY_SIP);

    return BS_OK;
}

static BS_STATUS _aclapp_CmdNoDIP(IN CHAR *pcListName, IN UINT uiRuleID)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.stDIP.uiIP = 0;
    pstRule->stRuleCfg.stDIP.uiWildcard = 0;
    BIT_CLR(pstRule->stRuleCfg.uiKeyMask, IPACL_KEY_DIP);

    return BS_OK;
}





static BS_STATUS _aclapp_CmdSPort
(
    IN CHAR *pcListName,
    IN UINT uiRuleID,
    IN USHORT usPortBegin,
    IN USHORT usPortEnd
)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.stSPort.usBegin = usPortBegin;
    pstRule->stRuleCfg.stSPort.usEnd = usPortEnd;
    pstRule->stRuleCfg.uiKeyMask |= IPACL_KEY_SPORT;

    return BS_OK;
}

static BS_STATUS _aclapp_CmdDPort
(
    IN CHAR *pcListName,
    IN UINT uiRuleID,
    IN USHORT usPortBegin,
    IN USHORT usPortEnd
)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.stDPort.usBegin = usPortBegin;
    pstRule->stRuleCfg.stDPort.usEnd = usPortEnd;
    pstRule->stRuleCfg.uiKeyMask |= IPACL_KEY_DPORT;

    return BS_OK;
}

static BS_STATUS _aclapp_CmdNoSPort
(
    IN CHAR *pcListName,
    IN UINT uiRuleID
)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.stSPort.usBegin = 0;
    pstRule->stRuleCfg.stSPort.usEnd = 0;
    BIT_CLR(pstRule->stRuleCfg.uiKeyMask, IPACL_KEY_SPORT);

    return BS_OK;
}

static BS_STATUS _aclapp_CmdNoDPort
(
    IN CHAR *pcListName,
    IN UINT uiRuleID
)
{
    IPACL_RULE_S *pstRule;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
    if (NULL == pstRule)
    {
        return BS_NOT_FOUND;
    }

    pstRule->stRuleCfg.stDPort.usBegin = 0;
    pstRule->stRuleCfg.stDPort.usEnd = 0;
    BIT_CLR(pstRule->stRuleCfg.uiKeyMask, IPACL_KEY_DPORT);

    return BS_OK;
}

static BS_STATUS _aclappip_NoAclList(IN CHAR *pcListName)
{
    UINT64 ulListID;

    ulListID = IPACL_GetListByName(g_hAclAppIpAcl, pcListName);
    if (0 == ulListID)
    {
        return BS_OK;
    }

    if (IPACL_ListGetRef(g_hAclAppIpAcl, ulListID) > 0)
    {
        return BS_REF_NOT_ZERO;
    }

    IPACL_DelList(g_hAclAppIpAcl, ulListID);

    return BS_OK;
}

static BS_STATUS _aclappip_MoveRule(IN CHAR *pcListName, IN UINT uiOldRuleID, IN UINT uiNewRuleID)
{
    return IPACL_MoveRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiOldRuleID, uiNewRuleID);
}

static VOID _aclappip_NoRule(IN CHAR *pcListName, IN UINT uiRuleID)
{
    IPACL_DelRule(g_hAclAppIpAcl, IPACL_GetListByName(g_hAclAppIpAcl, pcListName), uiRuleID);
}

BS_STATUS AclApp_Init()
{
    g_hAclAppIpAcl = IPACL_Create();
    if (NULL == g_hAclAppIpAcl)
    {
        return BS_NO_MEMORY;
    }

    MUTEX_Init(&g_stAclAppMutex);

    _aclapp_InitComp();

    return BS_OK;
}


/* CMD */

/* ip-acl %STRING */
PLUG_API BS_STATUS AclAppIP_EnterListView(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stAclAppMutex);
    eRet = _aclapp_EnterListView(ppcArgv[1]);
    MUTEX_V(&g_stAclAppMutex);
    
    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* rule %INT */
PLUG_API BS_STATUS AclAppIP_EnterRuleView(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiRuleID = 0;
    CHAR *pcRuleID = ppcArgv[1];
    BS_STATUS eRet;

    TXT_Atoui(pcRuleID, &uiRuleID);

    if (uiRuleID == 0)
    {
        return BS_ERR;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);

    MUTEX_P(&g_stAclAppMutex);
    eRet = _aclapp_EnterRuleView(pcListName, uiRuleID);
    MUTEX_V(&g_stAclAppMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }
    
    return BS_OK;
}

/* no ip-acl %STRING */
PLUG_API BS_STATUS AclAppIP_CmdNoList(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stAclAppMutex);
    eRet = _aclappip_NoAclList(ppcArgv[2]);
    MUTEX_V(&g_stAclAppMutex);

    if (eRet == BS_REF_NOT_ZERO)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}

/* move rule %INT to %INT */
PLUG_API BS_STATUS AclAppIP_CmdMoveRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiOldRuleID;
    UINT uiNewRuleID;
    BS_STATUS eRet;

    uiOldRuleID = TXT_Str2Ui(ppcArgv[2]);
    uiNewRuleID = TXT_Str2Ui(ppcArgv[4]);

    if ((uiOldRuleID == 0) || (uiNewRuleID == 0))
    {
        return BS_ERR;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);

    MUTEX_P(&g_stAclAppMutex);
    eRet = _aclappip_MoveRule(pcListName, uiOldRuleID, uiNewRuleID);
    MUTEX_V(&g_stAclAppMutex);

    if (eRet != BS_OK)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}

/* no rule %INT */
PLUG_API BS_STATUS AclAppIP_CmdNoRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiRuleID = 0;
    CHAR *pcRuleID = ppcArgv[2];

    TXT_Atoui(pcRuleID, &uiRuleID);

    if (uiRuleID == 0)
    {
        return BS_ERR;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);

    MUTEX_P(&g_stAclAppMutex);
    _aclappip_NoRule(pcListName, uiRuleID);
    MUTEX_V(&g_stAclAppMutex);

    return BS_OK;
}

/* action {permit | deny} */
PLUG_API BS_STATUS AclAppIP_CmdAction(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiRuleID;
    CHAR *pcListName;
    CHAR *pcRuleID;
    BS_STATUS eRet;
    BS_ACTION_E eAction;

    pcRuleID = CMD_EXP_GetCurrentModeValue(pEnv);
    pcListName = CMD_EXP_GetUpModeValue(pEnv, 1);

    TXT_Atoui(pcRuleID, &uiRuleID);
    if (uiRuleID == 0)
    {
        return BS_ERR;
    }

    eAction = BS_ACTION_DENY;
    if (ppcArgv[1][0] == 'p')
    {
        eAction = BS_ACTION_PERMIT;
    }

    MUTEX_P(&g_stAclAppMutex);
    eRet = _aclapp_CmdAction(pcListName, uiRuleID, eAction);
    MUTEX_V(&g_stAclAppMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* [no] enable */
PLUG_API BS_STATUS AclAppIP_CmdEnable(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiRuleID;
    CHAR *pcListName;
    CHAR *pcRuleID;
    BS_STATUS eRet;
    BOOL_T bEnable;

    pcRuleID = CMD_EXP_GetCurrentModeValue(pEnv);
    pcListName = CMD_EXP_GetUpModeValue(pEnv, 1);

    TXT_Atoui(pcRuleID, &uiRuleID);
    if (uiRuleID == 0)
    {
        return BS_ERR;
    }

    bEnable = FALSE;
    if (ppcArgv[0][0] == 'e')
    {
        bEnable = TRUE;
    }

    MUTEX_P(&g_stAclAppMutex);
    eRet = _aclapp_CmdEnable(pcListName, uiRuleID, bEnable);
    MUTEX_V(&g_stAclAppMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* ip {source|destination} %IP {0|%IP} */
PLUG_API BS_STATUS AclAppIP_CmdIP(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiRuleID;
    CHAR *pcListName;
    CHAR *pcRuleID;
    CHAR *pcIP = ppcArgv[2];
    CHAR *pcWildcard = ppcArgv[3];
    BS_STATUS eRet;
    UINT uiIP;
    UINT uiWildcard;

    pcRuleID = CMD_EXP_GetCurrentModeValue(pEnv);
    pcListName = CMD_EXP_GetUpModeValue(pEnv, 1);

    TXT_Atoui(pcRuleID, &uiRuleID);
    if (uiRuleID == 0)
    {
        return BS_ERR;
    }

    uiIP = Socket_Ipsz2IpHost(pcIP);
    uiWildcard = Socket_Ipsz2IpHost(pcWildcard);

    MUTEX_P(&g_stAclAppMutex);
    if (ppcArgv[1][0] == 's')
    {
        eRet = _aclapp_CmdSIP(pcListName, uiRuleID, uiIP, uiWildcard);
    }
    else
    {
        eRet = _aclapp_CmdDIP(pcListName, uiRuleID, uiIP, uiWildcard);
    }
    MUTEX_V(&g_stAclAppMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* no ip {source|destination} */
PLUG_API BS_STATUS AclAppIP_CmdNoIP(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiRuleID;
    CHAR *pcListName;
    CHAR *pcRuleID;
    BS_STATUS eRet;

    pcRuleID = CMD_EXP_GetCurrentModeValue(pEnv);
    pcListName = CMD_EXP_GetUpModeValue(pEnv, 1);

    TXT_Atoui(pcRuleID, &uiRuleID);
    if (uiRuleID == 0)
    {
        return BS_ERR;
    }

    MUTEX_P(&g_stAclAppMutex);
    if (ppcArgv[2][0] == 's')
    {
        eRet = _aclapp_CmdNoSIP(pcListName, uiRuleID);
    }
    else
    {
        eRet = _aclapp_CmdNoDIP(pcListName, uiRuleID);
    }
    MUTEX_V(&g_stAclAppMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* port {source|destination} range %INT %INT */
PLUG_API BS_STATUS AclAppIP_CmdPortRange(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiRuleID;
    CHAR *pcListName;
    CHAR *pcRuleID;
    BS_STATUS eRet;
    USHORT usPortBegin;
    USHORT usPortEnd;

    pcRuleID = CMD_EXP_GetCurrentModeValue(pEnv);
    pcListName = CMD_EXP_GetUpModeValue(pEnv, 1);

    TXT_Atoui(pcRuleID, &uiRuleID);
    if (uiRuleID == 0)
    {
        return BS_ERR;
    }

    usPortBegin = TXT_Str2Ui(ppcArgv[3]);
    usPortEnd = TXT_Str2Ui(ppcArgv[4]);

    MUTEX_P(&g_stAclAppMutex);
    if (ppcArgv[1][0] == 's')
    {
        eRet = _aclapp_CmdSPort(pcListName, uiRuleID, usPortBegin, usPortEnd);
    }
    else
    {
        eRet = _aclapp_CmdDPort(pcListName, uiRuleID, usPortBegin, usPortEnd);
    }
    MUTEX_V(&g_stAclAppMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* no port {source|destination} */
PLUG_API BS_STATUS AclAppIP_CmdNoPort(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiRuleID;
    CHAR *pcListName;
    CHAR *pcRuleID;
    BS_STATUS eRet;

    pcRuleID = CMD_EXP_GetCurrentModeValue(pEnv);
    pcListName = CMD_EXP_GetUpModeValue(pEnv, 1);

    TXT_Atoui(pcRuleID, &uiRuleID);
    if (uiRuleID == 0)
    {
        return BS_ERR;
    }

    MUTEX_P(&g_stAclAppMutex);
    if (ppcArgv[1][0] == 's')
    {
        eRet = _aclapp_CmdNoSPort(pcListName, uiRuleID);
    }
    else
    {
        eRet = _aclapp_CmdNoDPort(pcListName, uiRuleID);
    }
    MUTEX_V(&g_stAclAppMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

static VOID _aclapp_SaveCmdInRule(IN HANDLE hFile, IN UINT64 ulListID, IN UINT uiRuleID)
{
    IPACL_RULE_S *pstRule;
    UINT uiIP;
    UINT uiWildcard;

    pstRule = IPACL_GetRule(g_hAclAppIpAcl, ulListID, uiRuleID);
    if (NULL == pstRule)
    {
        return;
    }

    if (pstRule->stRuleCfg.uiKeyMask & IPACL_KEY_SIP)
    {
        uiIP = htonl(pstRule->stRuleCfg.stSIP.uiIP);
        uiWildcard = htonl(pstRule->stRuleCfg.stSIP.uiWildcard);

        CMD_EXP_OutputCmd(hFile, "ip source %pI4 %pI4", &uiIP, &uiWildcard);
    }

    if (pstRule->stRuleCfg.uiKeyMask & IPACL_KEY_DIP)
    {
        uiIP = htonl(pstRule->stRuleCfg.stDIP.uiIP);
        uiWildcard = htonl(pstRule->stRuleCfg.stDIP.uiWildcard);

        CMD_EXP_OutputCmd(hFile, "ip destination %pI4 %pI4", &uiIP, &uiWildcard);
    }

    if (pstRule->stRuleCfg.uiKeyMask & IPACL_KEY_SPORT)
    {
        CMD_EXP_OutputCmd(hFile, "port source range %d %d",
            pstRule->stRuleCfg.stSPort.usBegin, pstRule->stRuleCfg.stSPort.usEnd);
    }

    if (pstRule->stRuleCfg.uiKeyMask & IPACL_KEY_DPORT)
    {
        CMD_EXP_OutputCmd(hFile, "port destination range %d %d",
            pstRule->stRuleCfg.stDPort.usBegin, pstRule->stRuleCfg.stDPort.usEnd);
    }

    if (pstRule->stRuleCfg.enAction == BS_ACTION_DENY)
    {
        CMD_EXP_OutputCmd(hFile, "action deny");
    }
    else
    {
        CMD_EXP_OutputCmd(hFile, "action permit");
    }

    if (pstRule->stRuleCfg.bEnable)
    {
        CMD_EXP_OutputCmd(hFile, "enable");
    }
}

static VOID aclapp_SaveCmdInList(IN HANDLE hFile, IN UINT64 ulListID)
{
    UINT uiRuleID = RULE_ID_INVALID;

    while ((uiRuleID = IPACL_GetNextRuleID(g_hAclAppIpAcl, ulListID, uiRuleID)) != 0)
    {
        CMD_EXP_OutputMode(hFile, "rule %d", uiRuleID);
        _aclapp_SaveCmdInRule(hFile, ulListID, uiRuleID);
        CMD_EXP_OutputModeQuit(hFile);
    }
}

PLUG_API BS_STATUS AclApp_Save(IN HANDLE hFile)
{
    UINT64 ulListID = 0;
    CHAR *pcListName;

    MUTEX_P(&g_stAclAppMutex);

    while ((ulListID = IPACL_GetNextList(g_hAclAppIpAcl, ulListID)) != 0)
    {
        pcListName = IPACL_GetListNameByID(g_hAclAppIpAcl, ulListID);
        CMD_EXP_OutputMode(hFile, "ip-acl %s", pcListName);
        aclapp_SaveCmdInList(hFile, ulListID);
        CMD_EXP_OutputModeQuit(hFile);
    }

    MUTEX_V(&g_stAclAppMutex);

    return BS_OK;
}

