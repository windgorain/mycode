
#include "bs.h"

#include "utl/hash_utl.h"
#include "utl/txt_utl.h"

#include "../../inc/vnet_conf.h"

#include "vnets_domain_inner.h"

#define _VNETS_DOMAIN_NIM_HASH_BUCKET_NUM 512

static HASH_HANDLE g_hVnetsDomainNIMHashHandle = NULL;

typedef struct
{
    HASH_NODE_S stHashNode;
    CHAR szDomainName[VNET_CONF_MAX_DOMAIN_NAME_LEN + 1];
    UINT uiDomainId;
}_VNETS_DOMAIN_NIM_S;

static UINT vnets_domainnim_HashIndex(IN VOID *pstHashNode)
{
    _VNETS_DOMAIN_NIM_S *pstNode = pstHashNode;
    UINT i = 0;
    CHAR *pcChar;

    pcChar = pstNode->szDomainName;

    while (*pcChar != '\0')
    {
        i += *pcChar;
        pcChar ++;
    }

    return i;
}

static INT vnets_domainnim_Cmp(IN VOID * pstHashNode, IN VOID * pstNodeToFind)
{
    _VNETS_DOMAIN_NIM_S *pstNode = (_VNETS_DOMAIN_NIM_S*)pstHashNode;
    _VNETS_DOMAIN_NIM_S *pstNodeFind = (_VNETS_DOMAIN_NIM_S*)pstNodeToFind;

    return stricmp(pstNode->szDomainName, pstNodeFind->szDomainName);
}

static _VNETS_DOMAIN_NIM_S * vnets_domainnim_Find(IN CHAR *pcDomainName)
{
    _VNETS_DOMAIN_NIM_S *pstNode;
    _VNETS_DOMAIN_NIM_S stNodeToFind;

    TXT_Strlcpy(stNodeToFind.szDomainName, pcDomainName, sizeof(stNodeToFind.szDomainName));

    pstNode = HASH_Find(g_hVnetsDomainNIMHashHandle, vnets_domainnim_Cmp, &stNodeToFind);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return pstNode;
}

BS_STATUS _VNETS_DomainNIM_Init()
{
    g_hVnetsDomainNIMHashHandle = HASH_CreateInstance(NULL, _VNETS_DOMAIN_NIM_HASH_BUCKET_NUM, vnets_domainnim_HashIndex);
    if (NULL == g_hVnetsDomainNIMHashHandle)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

BS_STATUS _VNETS_DomainNIM_Add(IN CHAR * pcDomainName, IN UINT uiDomainId)
{
    _VNETS_DOMAIN_NIM_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(_VNETS_DOMAIN_NIM_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    TXT_Strlcpy(pstNode->szDomainName, pcDomainName, sizeof(pstNode->szDomainName));
    pstNode->uiDomainId = uiDomainId;

    HASH_Add(g_hVnetsDomainNIMHashHandle, pstNode);

    return BS_OK;
}

BS_STATUS _VNETS_DomainNIM_Del(IN CHAR * pcDomainName)
{
    _VNETS_DOMAIN_NIM_S *pstNode;

    pstNode = vnets_domainnim_Find(pcDomainName);
    if (pstNode == NULL)
    {
        return BS_OK;
    }

    HASH_Del(g_hVnetsDomainNIMHashHandle, pstNode);

    MEM_Free(pstNode);

    return BS_OK;
}

UINT _VNETS_DomainNIM_GetId(IN CHAR * pcDomainName)
{
    _VNETS_DOMAIN_NIM_S *pstNode;

    pstNode = vnets_domainnim_Find(pcDomainName);
    if (pstNode == NULL)
    {
        return 0;
    }

    return pstNode->uiDomainId;
}
