
#include "bs.h"

#include "utl/hash_utl.h"
#include "utl/mutex_utl.h"
#include "utl/txt_utl.h"
#include "utl/dns_service.h"


#define RETCODE_FILE_NUM RETCODE_FILE_NUM_DNS_SERVICE

#define DNS_SERVICE_LABEL_HASH_BUCKET_DFT 16      

#define DNS_SERVICE_FUNC_INIT() \
    do {    \
        if (pstService == NULL)     \
        {       \
            BS_DBGASSERT(0);        \
            RETURN(BS_NULL_PARA);       \
        }       \
        if (FALSE == dns_service_CheckDomainName(pcDomainName))     \
        {       \
            RETURN(BS_BAD_PARA);        \
        }       \
        TXT_Strlcpy(szDomainName, pcDomainName, sizeof(szDomainName));      \
        uiLabelNum = DNS_DomainName2Labels(szDomainName, &stLabels);        \
        if (uiLabelNum < 2)          \
        {       \
            RETURN(BS_BAD_PARA);        \
        }       \
    }while (0)

typedef struct tagDNS_SERVICE_LABEL_S
{
    HASH_NODE_S stHashNode;
    UINT uiIP;  
    CHAR *pcLabel;
    struct tagDNS_SERVICE_LABEL_S *pstParent;
    HASH_S * hLabelHash; 
}DNS_SERVICE_LABEL_S;

typedef struct
{
    HASH_S * hLabelHash;
    BOOL_T bCreateMutex;
    MUTEX_S stMutex;
}DNS_SERVICE_S;


static VOID dns_service_FreeLabelNode(IN DNS_SERVICE_LABEL_S *pstLabel);



static UINT g_auiDnsServiceLevelHashBucket[] =
{
    16,1024
};

static UINT dns_service_GetLabelIndex(IN VOID *pHashNode)
{
    UINT uiIndex = 0;
    CHAR *pc;
    DNS_SERVICE_LABEL_S *pstHashNode = pHashNode;

    pc = pstHashNode->pcLabel;
    while (*pc != '\0')
    {
        uiIndex += *pc;
        pc++;
    }

    return uiIndex;
}

static inline VOID dns_service_Lock(IN DNS_SERVICE_S *pstService)
{
    if (pstService->bCreateMutex)
    {
        MUTEX_P(&pstService->stMutex);
    }
}

static inline VOID dns_service_UnLock(IN DNS_SERVICE_S *pstService)
{
    if (pstService->bCreateMutex)
    {
        MUTEX_V(&pstService->stMutex);
    }
}

static UINT dns_service_GetHashBucketNumByLevel(IN UINT uiLevel)
{
    if (uiLevel >= sizeof(g_auiDnsServiceLevelHashBucket)/sizeof(UINT))
    {
        return DNS_SERVICE_LABEL_HASH_BUCKET_DFT;
    }

    return g_auiDnsServiceLevelHashBucket[uiLevel];
}

static DNS_SERVICE_LABEL_S * dns_service_MallocLabelNode(IN CHAR *pcLabel, IN UINT uiLevel)
{
    DNS_SERVICE_LABEL_S *pstNode;
    UINT uiLen;

    uiLen = strlen(pcLabel);
    
    BS_DBGASSERT(uiLen != 0);
    BS_DBGASSERT(uiLen <= DNS_MAX_LABEL_LEN);

    pstNode = MEM_ZMalloc(sizeof(DNS_SERVICE_LABEL_S) + uiLen + 1);
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->pcLabel = (CHAR*)(pstNode + 1);
    TXT_Strlcpy(pstNode->pcLabel, pcLabel, uiLen + 1);

    pstNode->hLabelHash = HASH_CreateInstance(NULL, dns_service_GetHashBucketNumByLevel(uiLevel + 1), dns_service_GetLabelIndex);
    if (NULL == pstNode->hLabelHash)
    {
        MEM_Free(pstNode);
        return NULL;
    }

    return pstNode;
}

static VOID  dns_service_FreeEach(IN void * hHashId, IN VOID *pstNode, IN VOID * pUserHandle)
{
    DNS_SERVICE_LABEL_S *pstLabel = (DNS_SERVICE_LABEL_S*)pstNode;

    dns_service_FreeLabelNode(pstLabel);
}

static VOID dns_service_FreeLabelNode(IN DNS_SERVICE_LABEL_S *pstLabel)
{
    
    if (pstLabel->hLabelHash != NULL)
    {
        HASH_DelAll(pstLabel->hLabelHash, dns_service_FreeEach, NULL);
        HASH_DestoryInstance(pstLabel->hLabelHash);
    }

    MEM_Free(pstLabel);
}

static INT  dns_service_LabelCmp(IN VOID * pstHashNode, IN VOID * pstHashNodeToFind)
{
    DNS_SERVICE_LABEL_S * pstNode = (DNS_SERVICE_LABEL_S*)pstHashNode;
    DNS_SERVICE_LABEL_S * pstNodeToFind = (DNS_SERVICE_LABEL_S*)pstHashNodeToFind;
    
    return stricmp(pstNode->pcLabel, pstNodeToFind->pcLabel);
}

static DNS_SERVICE_LABEL_S * dns_service_FindLabel(IN HASH_S * hLabelHash, IN CHAR *pcLabelName)
{
    DNS_SERVICE_LABEL_S stNodeToFind;

    stNodeToFind.pcLabel = pcLabelName;
    
    return (DNS_SERVICE_LABEL_S*) HASH_Find(hLabelHash, dns_service_LabelCmp, (HASH_NODE_S*)&stNodeToFind);
}


static DNS_SERVICE_LABEL_S * dns_service_CreateLabels
(
    IN DNS_SERVICE_LABEL_S *pstParent,
    IN HASH_S * hLabelHash,
    IN CHAR **ppcLabels,  
    IN UINT uiLabelPos,  
    IN UINT uiLevel      
)
{
    DNS_SERVICE_LABEL_S *pstNode, *pstNodeTmp;

    pstNode = dns_service_FindLabel(hLabelHash, ppcLabels[uiLabelPos]);
    if (NULL == pstNode)
    {
        pstNode = dns_service_MallocLabelNode(ppcLabels[uiLabelPos], uiLevel);
        if (NULL == pstNode)
        {
            return NULL;
        }

        pstNode->pstParent = pstParent;

        HASH_Add(hLabelHash, (HASH_NODE_S*)pstNode);
    }

    if (uiLabelPos == 0)
    {
        return pstNode;
    }

    pstNodeTmp = dns_service_CreateLabels(pstNode, pstNode->hLabelHash, ppcLabels, uiLabelPos - 1, uiLevel + 1);
    if (NULL == pstNodeTmp)
    {
        if (HASH_Count(pstNode->hLabelHash) == 0)
        {
            dns_service_FreeLabelNode(pstNode);
        }
    }

    return pstNodeTmp;
}

static BS_STATUS dns_service_AddName
(
    IN DNS_SERVICE_S *pstService,
    IN CHAR **ppcLabels,  
    IN UINT uiLabelNum,  
    IN UINT uiIP
)
{
    DNS_SERVICE_LABEL_S *pstLabel;

    pstLabel = dns_service_CreateLabels(NULL, pstService->hLabelHash, ppcLabels, uiLabelNum - 1, 0);
    if (NULL == pstLabel)
    {
        RETURN(BS_NO_MEMORY);
    }

    pstLabel->uiIP = uiIP;

    return BS_OK;
}

static BS_STATUS dns_service_DelLabels
(
    IN HASH_S * hLabelHash,
    IN CHAR **ppcLabels,  
    IN UINT uiLabelPos    
)
{
    DNS_SERVICE_LABEL_S *pstNode;

    pstNode = dns_service_FindLabel(hLabelHash, ppcLabels[uiLabelPos]);
    if (NULL == pstNode)
    {
        return BS_OK;
    }

    if (uiLabelPos > 0)
    {
        dns_service_DelLabels(pstNode->hLabelHash, ppcLabels, uiLabelPos - 1);
    }    

    if ((uiLabelPos == 0) || (HASH_Count(pstNode->hLabelHash) == 0))
    {
        HASH_Del(hLabelHash, (HASH_NODE_S*)pstNode);
        dns_service_FreeLabelNode(pstNode);
    }

    return BS_OK;
}

static inline BS_STATUS dns_service_DelName
(
    IN DNS_SERVICE_S *pstService,
    IN CHAR **ppcLabels,  
    IN UINT uiLabelNum    
)
{
    return dns_service_DelLabels(pstService->hLabelHash, ppcLabels, uiLabelNum - 1);
}


static DNS_SERVICE_LABEL_S * dns_service_FindLabels
(
    IN HASH_S * hLabelHash,
    IN CHAR **ppcLabels,
    IN UINT uiLabelPos
)
{
    DNS_SERVICE_LABEL_S *pstNode;

    pstNode = dns_service_FindLabel(hLabelHash, ppcLabels[uiLabelPos]);
    if (NULL == pstNode)
    {
        pstNode = dns_service_FindLabel(hLabelHash, "*");
        if (NULL != pstNode)
        {
            return pstNode;
        }
    }
    
    if ((NULL != pstNode) && (uiLabelPos > 0))
    {
        return dns_service_FindLabels(pstNode->hLabelHash, ppcLabels, uiLabelPos - 1);
    }

    return pstNode;
}

static BOOL_T dns_service_CheckDomainName(IN CHAR *pcDomainName)
{
    if ((pcDomainName == NULL) || (pcDomainName[0] == '\0'))
    {
        return FALSE;
    }

    if (strlen(pcDomainName) > DNS_MAX_DOMAIN_NAME_LEN)
    {
        return FALSE;
    }

    return TRUE;
}

static BS_STATUS dns_service_GetInfoByName
(
    IN DNS_SERVICE_S *pstService,
    IN CHAR **ppcLabels,  
    IN UINT uiLabelNum,   
    OUT DNS_SERVICE_INFO_S *pstInfo
)
{
    DNS_SERVICE_LABEL_S *pstFind;
    DNS_SERVICE_LABEL_S *pstTmp;
    BOOL_T bIsNeedDot = FALSE;

    pstFind = dns_service_FindLabels(pstService->hLabelHash, ppcLabels, uiLabelNum - 1);
    if (NULL == pstFind)
    {
        return BS_NOT_FOUND;
    }

    pstInfo->uiIP = pstFind->uiIP;

    pstInfo->szAuthorName[0] = '\0';

    pstTmp = pstFind;
    while (pstTmp != NULL)
    {
        if (pstTmp->pcLabel[0] != '*')
        {
            if (bIsNeedDot)
            {
                TXT_Strlcat(pstInfo->szAuthorName, ".", DNS_MAX_DOMAIN_NAME_LEN + 1);
            }
            TXT_Strlcat(pstInfo->szAuthorName, pstTmp->pcLabel, DNS_MAX_DOMAIN_NAME_LEN + 1);
            bIsNeedDot = TRUE;
        }

        pstTmp = pstTmp->pstParent;
    }

    return BS_OK;
}

DNS_SERVICE_HANDLE DNS_Service_Create(IN BOOL_T bCreateMutex)
{
    DNS_SERVICE_S *pstService;

    pstService = MEM_ZMalloc(sizeof(DNS_SERVICE_S));
    if (NULL == pstService)
    {
        return NULL;
    }

    pstService->bCreateMutex = bCreateMutex;
    if (bCreateMutex)
    {
        MUTEX_Init(&pstService->stMutex);
    }

    pstService->hLabelHash = HASH_CreateInstance(NULL, g_auiDnsServiceLevelHashBucket[0], dns_service_GetLabelIndex);
    if (NULL == pstService->hLabelHash)
    {
        MUTEX_Final(&pstService->stMutex);
        MEM_Free(pstService);
        return NULL;
    }

    return pstService;
}

VOID DNS_Service_Destory(IN DNS_SERVICE_HANDLE hDnsService)
{
    DNS_SERVICE_S *pstService = hDnsService;

    if (pstService == NULL)
    {
        return;
    }

    HASH_DelAll(pstService->hLabelHash, dns_service_FreeEach, NULL);
    HASH_DestoryInstance(pstService->hLabelHash);
    if (pstService->bCreateMutex)
    {
        MUTEX_Final(&pstService->stMutex);
    }

    MEM_Free(pstService);

    return;
}

BS_STATUS DNS_Service_AddName
(
    IN DNS_SERVICE_HANDLE hService,
    IN CHAR *pcDomainName,
    IN UINT uiIP
)
{
    DNS_SERVICE_S *pstService = hService;
    BS_STATUS eRet;
    CHAR szDomainName[DNS_MAX_DOMAIN_NAME_LEN + 1];
    DNS_LABELS_S stLabels;
    UINT uiLabelNum;

    DNS_SERVICE_FUNC_INIT();

    dns_service_Lock(pstService);
    eRet = dns_service_AddName(pstService, stLabels.apcLabels, uiLabelNum, uiIP);
    dns_service_UnLock(pstService);

    return eRet;
}

BS_STATUS DNS_Service_DelName(IN DNS_SERVICE_HANDLE hService, IN CHAR *pcDomainName)
{
    DNS_SERVICE_S *pstService = hService;
    BS_STATUS eRet;
    CHAR szDomainName[DNS_MAX_DOMAIN_NAME_LEN + 1];
    DNS_LABELS_S stLabels;
    UINT uiLabelNum;

    DNS_SERVICE_FUNC_INIT();

    dns_service_Lock(pstService);
    eRet = dns_service_DelName(pstService, stLabels.apcLabels, uiLabelNum);
    dns_service_UnLock(pstService);

    return eRet;
}

BS_STATUS DNS_Service_GetInfoByName
(
    IN DNS_SERVICE_HANDLE hService,
    IN CHAR *pcDomainName,
    OUT DNS_SERVICE_INFO_S *pstInfo
)
{
    DNS_SERVICE_S *pstService = hService;
    BS_STATUS eRet;
    CHAR szDomainName[DNS_MAX_DOMAIN_NAME_LEN + 1];
    DNS_LABELS_S stLabels;
    UINT uiLabelNum;

    DNS_SERVICE_FUNC_INIT();

    dns_service_Lock(pstService);
    eRet = dns_service_GetInfoByName(pstService, stLabels.apcLabels, uiLabelNum, pstInfo);
    dns_service_UnLock(pstService);

    return eRet;
}


