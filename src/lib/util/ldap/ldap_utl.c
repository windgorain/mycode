/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-1-10
* Description: 对LDAP接口的封装
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/kv_utl.h"
#include "utl/ldap_utl.h"

#ifdef IN_WINDOWS
#include "winldap.h"
#endif

typedef enum
{
    LDAPUTL_STATE_BIND_NULL = 0,
    LDAPUTL_STATE_SEARCH_DOMAIN,
    LDAPUTL_STATE_BIND_USER,
    LDAPUTL_STATE_SEARCH_SUBTREE,
    LDAPUTL_STATE_FINISH,
    
}LDAPUTL_STATE_E;

typedef struct
{
    CHAR *pcServerAddress;
    USHORT usServerPort;   /* 主机序 */
    CHAR *pcUserName;
    CHAR *pcPassword;

    LDAP *pstLdapSession;
    CHAR *pcDomainDn;
    LDAPUTL_STATE_E eState;
    LONG lMsgID;
}LDAP_NODE_S;

/* 将dc=hit,dc=com转换为"hit.com" */
static BS_STATUS _ldaputl_DomainDn2Domain(IN CHAR *pcDomainDN, OUT CHAR *pcDomain)
{
    KV_HANDLE hKv;
    LSTR_S stInput;
    KV_S *pstKvNode = NULL;

    pcDomain[0] = '\0';

    hKv = KV_Create(KV_FLAG_PERMIT_DUP_KEY);
    if (NULL == hKv)
    {
        return BS_NO_MEMORY;
    }

    stInput.pcData = pcDomainDN;
    stInput.uiLen = strlen(pcDomainDN);

    if (BS_OK != KV_Parse(hKv, &stInput, ',', '='))
    {
        KV_Destory(hKv);
        return BS_ERR;
    }

    while ((pstKvNode = KV_GetNext(hKv, pstKvNode)) != NULL)
    {
        if (stricmp(pstKvNode->pcKey, "dc") != 0)
        {
            continue;
        }

        if (pcDomain[0] != '\0')
        {
            strcat(pcDomain, ".");
        }

        strcat(pcDomain, pstKvNode->pcValue);
    }

    KV_Destory(hKv);

    return BS_OK;
}

static BS_STATUS _ldaputl_ProcessBindNull(IN LDAP_NODE_S *pstNode)
{
    LDAPMessage * plmsgSearchResponse;
    LONG lRet;
    
    lRet = ldap_result(pstNode->pstLdapSession, pstNode->lMsgID, LDAP_MSG_ALL, NULL, &plmsgSearchResponse);
    if (lRet < 0)
    {
        return BS_ERR;
    }

    /* 0表示还在查询 */
    if (lRet == 0)
    {
        return BS_CONTINUE;
    }

    pstNode->lMsgID = -1;

    pstNode->lMsgID = ldap_search(pstNode->pstLdapSession, NULL, LDAP_SCOPE_BASE, NULL, NULL, FALSE);
    if (pstNode->lMsgID < 0)
    {
		return BS_ERR;
    }

    pstNode->eState = LDAPUTL_STATE_SEARCH_DOMAIN;

    return BS_CONTINUE;
}

static BS_STATUS _ldaputl_ProcessSearchDomain(IN LDAP_NODE_S *pstNode)
{
    LDAPMessage * plmsgSearchResponse;
    LONG lRet;
    CHAR **ppcDomainDN;
    CHAR szDomain[256];
    CHAR szTmp[256];
    
    lRet = ldap_result(pstNode->pstLdapSession, pstNode->lMsgID, LDAP_MSG_ALL, NULL, &plmsgSearchResponse);
    if (lRet < 0)
    {
        return BS_ERR;
    }

    /* 0表示还在查询 */
    if (lRet == 0)
    {
        return BS_CONTINUE;
    }

    pstNode->lMsgID = -1;

    ppcDomainDN = ldap_get_values(pstNode->pstLdapSession, plmsgSearchResponse, "defaultNamingContext");
    if ((NULL == ppcDomainDN) || (*ppcDomainDN == NULL))
    {
    	return BS_ERR;
    }

    pstNode->pcDomainDn = TXT_Strdup(*ppcDomainDN);

    ldap_value_free(ppcDomainDN);
    
    if (NULL == pstNode->pcDomainDn)
    {
        return BS_ERR;
    }

    _ldaputl_DomainDn2Domain(pstNode->pcDomainDn, szDomain);
    snprintf(szTmp, sizeof(szTmp), "%s@%s", pstNode->pcUserName, szDomain);

	pstNode->lMsgID = ldap_simple_bind(pstNode->pstLdapSession, szTmp, pstNode->pcPassword);
    if (pstNode->lMsgID < 0)
	{
		return BS_ERR;
	}

    pstNode->eState = LDAPUTL_STATE_BIND_USER;

    return BS_CONTINUE;
}

static BS_STATUS _ldaputl_ProcessBindUser(IN LDAP_NODE_S *pstNode)
{
    LDAPMessage * plmsgSearchResponse;
    LONG lRet;
    CHAR *apcAttrs[] = {"memberOf", NULL};
    
    lRet = ldap_result(pstNode->pstLdapSession, pstNode->lMsgID, LDAP_MSG_ALL, NULL, &plmsgSearchResponse);
    if (lRet < 0)
    {
        return BS_ERR;
    }

    /* 0表示还在查询 */
    if (lRet == 0)
    {
        return BS_CONTINUE;
    }

    pstNode->lMsgID = -1;

    pstNode->lMsgID = ldap_search(pstNode->pstLdapSession,     // session handle  
                    pstNode->pcDomainDn,   // locaation to start search, NULL specifies top level  
                    LDAP_SCOPE_SUBTREE,    // search only the root entry (rootDSE)  
                    "(&(sAMAccountName=user1)(objectCategory=person))",   // search for all objects (only one for the rootDSE)  
                    apcAttrs,   // no attributes specified, return all attributes  
                    FALSE);  // return attributes types and values  
    if (pstNode->lMsgID < 0)
	{
		return BS_ERR;
	}

    pstNode->eState = LDAPUTL_STATE_SEARCH_SUBTREE;

    return BS_CONTINUE;
}

static BS_STATUS _ldaputl_ProcessSearchSubtree(IN LDAP_NODE_S *pstNode)
{
    LDAPMessage * plmsgSearchResponse;
    LONG lRet;

    lRet = ldap_result(pstNode->pstLdapSession, pstNode->lMsgID, LDAP_MSG_ALL, NULL, &plmsgSearchResponse);
    if (lRet < 0)
    {
        return BS_ERR;
    }

    /* 0表示还在查询 */
    if (lRet == 0)
    {
        return BS_CONTINUE;
    }

    pstNode->lMsgID = -1;

    pstNode->eState = LDAPUTL_STATE_FINISH;

    return BS_OK;
}

LDAPUTL_HANDLE LDAPUTL_Create
(
    IN CHAR *pcServerAddress,
    IN USHORT usServerPort,
    IN CHAR *pcUserName,
    IN CHAR *pcPassword
)
{
    LDAP_NODE_S *pstLdapNode;

    pstLdapNode = MEM_ZMalloc(sizeof(LDAP_NODE_S));
    if (NULL == pstLdapNode)
    {
        return NULL;
    }

    pstLdapNode->pcServerAddress = TXT_Strdup(pcServerAddress);
    pstLdapNode->pcUserName = TXT_Strdup(pcUserName);
    pstLdapNode->pcPassword = TXT_Strdup(pcPassword);
    pstLdapNode->usServerPort = usServerPort;
    pstLdapNode->lMsgID = -1;
    
    if ((NULL == pstLdapNode->pcServerAddress) || (NULL == pstLdapNode->pcUserName) || (NULL == pstLdapNode->pcPassword))
    {
        LDAPUTL_Destroy(pstLdapNode);
        return NULL;
    }

    return pstLdapNode;
}

VOID LDAPUTL_Destroy(IN LDAPUTL_HANDLE hLdapNode)
{
    LDAP_NODE_S *pstLdapNode = hLdapNode;

    if (pstLdapNode->pcServerAddress)
    {
        MEM_Free(pstLdapNode->pcServerAddress);
    }

    if (pstLdapNode->pcUserName)
    {
        MEM_Free(pstLdapNode->pcUserName);
    }

    if (pstLdapNode->pcPassword)
    {
        MEM_Free(pstLdapNode->pcPassword);
    }

    if (pstLdapNode->pcDomainDn)
    {
        MEM_Free(pstLdapNode->pcDomainDn);
    }

    if (pstLdapNode->lMsgID >= 0)
    {
        ldap_abandon(pstLdapNode->pstLdapSession, pstLdapNode->lMsgID);
    }

    if (pstLdapNode->pstLdapSession)
    {
        ldap_unbind(pstLdapNode->pstLdapSession);
    }

    MEM_Free(pstLdapNode);
}

BS_STATUS LDAPUTL_StartAuth(IN LDAPUTL_HANDLE hLdapNode)
{
    LDAP_NODE_S *pstLdapNode = hLdapNode;
    int version;
    INT iTimeout;

    pstLdapNode->pstLdapSession = ldap_init(pstLdapNode->pcServerAddress, pstLdapNode->usServerPort);
    if (NULL == pstLdapNode->pstLdapSession)
    {
        return BS_ERR;
    }

    version = LDAP_VERSION3;
  	ldap_set_option(pstLdapNode->pstLdapSession, LDAP_OPT_PROTOCOL_VERSION, &version);

    iTimeout = 15;
	ldap_set_option(pstLdapNode->pstLdapSession, LDAP_OPT_TIMELIMIT, &iTimeout);

#ifdef IN_UNIXLIKE
    {
        struct timeval tv;
    	tv.tv_sec = 0;
    	tv.tv_usec = 0;
    	ldap_set_option(pstLdapNode->pstLdapSession, LDAP_OPT_NETWORK_TIMEOUT, &tv);
    }
#endif

    /* 第一次绑定,匿名连接 */
	pstLdapNode->lMsgID = ldap_simple_bind(pstLdapNode->pstLdapSession, NULL, NULL);
    if (pstLdapNode->lMsgID < 0)
	{
		return BS_ERR;
	}

    pstLdapNode->eState = LDAPUTL_STATE_BIND_NULL;

    return BS_OK;
}

INT LDAPUTL_GetSocketID(IN LDAPUTL_HANDLE hLdapNode)
{
    LDAP_NODE_S *pstLdapNode = hLdapNode;
    INT iSocketID;

#ifdef IN_UNIXLIKE
    {
        Sockbuf *sockbuf;

        if (0 > ldap_get_option(pstLdapNode->pstLdapSession, LDAP_OPT_SOCKBUF, (void **)&sockbuf ))
        {
            return -1;
        }

    	if (0 >= ber_sockbuf_ctrl(sockbuf, LBER_SB_OPT_GET_FD, &iSocketID))
        {
            return -1;
        }
    }
#endif

#ifdef IN_WINDOWS
    iSocketID = pstLdapNode->pstLdapSession->ld_sb.sb_sd;
#endif

    return iSocketID;
}

/*
   返回值: BS_OK:成功. BS_CONTINUE:还没完成,需要继续. 其他:出错
*/
BS_STATUS LDAPUTL_Run(IN LDAPUTL_HANDLE hLdapNode)
{
    LDAP_NODE_S *pstLdapNode = hLdapNode;
    BS_STATUS eRet;

    switch (pstLdapNode->eState)
    {
        case LDAPUTL_STATE_BIND_NULL:
        {
            eRet = _ldaputl_ProcessBindNull(pstLdapNode);
            break;
        }
        case LDAPUTL_STATE_SEARCH_DOMAIN:
        {
            eRet = _ldaputl_ProcessSearchDomain(pstLdapNode);
            break;
        }

        case LDAPUTL_STATE_BIND_USER:
        {
            eRet = _ldaputl_ProcessBindUser(pstLdapNode);
            break;
        }

        case LDAPUTL_STATE_SEARCH_SUBTREE:
        {
            eRet = _ldaputl_ProcessSearchSubtree(pstLdapNode);
            break;
        }

        case LDAPUTL_STATE_FINISH:
        {
            eRet = BS_OK;
            break;
        }

        default:
        {
            BS_DBGASSERT(0);
            eRet = BS_ERR;
            break;
        }
    }

	return eRet;
}


