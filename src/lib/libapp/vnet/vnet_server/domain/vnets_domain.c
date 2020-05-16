
#include "bs.h"

#include "utl/hash_utl.h"
#include "utl/mutex_utl.h"
#include "utl/vclock_utl.h"

#include "../../inc/vnet_domain.h"

#include "../inc/vnets_dc.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_mac_tbl.h"
#include "../inc/vnets_master.h"
#include "../inc/vnets_enter_domain.h"

#include "vnets_domain_inner.h"

typedef struct
{
    CHAR cTypeFlag;
    VNETS_DOMAIN_TYPE_E eType;
}VNETS_DOMAIN_TYPE_FLAG_MAP_S;

static VNETS_DOMAIN_TYPE_FLAG_MAP_S g_astVnetsDomainTypeFlagMap[] =
{
    {VNET_DOMAIN_TYPE_CHAR_PERSONAL, VNETS_DOMAIN_TYPE_PERSONAL},
    {VNET_DOMAIN_TYPE_CHAR_INNER, VNETS_DOMAIN_TYPE_INNER}
};

static MUTEX_S g_stVnetsDomainMutex;

static UINT vnets_domain_Add
(
    IN CHAR *pcDomainName,
    IN VNETS_DOMAIN_TYPE_E eType,
    IN UINT uiNodeID
)
{
    UINT uiDomainId;

    uiDomainId = _VNETS_DomainNIM_GetId(pcDomainName);
    if (uiDomainId != 0)
    {
        _VNETS_DomainRecord_AddNode(uiDomainId, uiNodeID);
        return uiDomainId;
    }

    /* 申请一个可用ID */
    uiDomainId = _VNETS_DomainId_Get();
    if (0 == uiDomainId)
    {
        return 0;
    }

    /* 添加名字到ID的映射 */
    if (BS_OK != _VNETS_DomainNIM_Add(pcDomainName, uiDomainId))
    {
        _VNETS_DomainId_FreeID(uiDomainId);
        return 0;
    }

    /* 添加一个域记录 */
    if (BS_OK != _VNETS_DomainRecord_Add(uiDomainId, pcDomainName, eType, uiNodeID))
    {
        _VNETS_DomainNIM_Del(pcDomainName);
        _VNETS_DomainId_FreeID(uiDomainId);
        return 0;
    }

    return uiDomainId;
}

static VNETS_DOMAIN_TYPE_E vnets_domain_GetDomainTypeByName(IN CHAR *pcDomainName)
{
    VNETS_DOMAIN_TYPE_E eType = VNETS_DOMAIN_TYPE_NORMAL;
    UINT i;
    CHAR cFlag = pcDomainName[0];

    for (i=0; i<sizeof(g_astVnetsDomainTypeFlagMap)/sizeof(VNETS_DOMAIN_TYPE_FLAG_MAP_S); i++)
    {
        if (cFlag == g_astVnetsDomainTypeFlagMap[i].cTypeFlag)
        {
            eType = g_astVnetsDomainTypeFlagMap[i].eType;
            break;
        }
    }

    return eType;
}

BS_STATUS VNETS_Domain_Init()
{
    _VNETS_DomainId_Init();
    _VNETS_DomainNIM_Init();
    _VNETS_DomainRecord_Init();
    _VNETS_DomainCmd_Init();

    MUTEX_Init(&g_stVnetsDomainMutex);

    return BS_OK;
}

UINT VNETS_Domain_Add
(
    IN CHAR *pcDomainName,
    IN UINT uiNodeID
)
{
    UINT uiDomainId;
    VNETS_DOMAIN_TYPE_E eType;

    eType = vnets_domain_GetDomainTypeByName(pcDomainName);

    if (eType == VNETS_DOMAIN_TYPE_NORMAL)
    {
        if (FALSE == VNETS_DC_IsDomainExist(pcDomainName))
        {
            return 0;
        }
    }

    MUTEX_P(&g_stVnetsDomainMutex);
    uiDomainId = vnets_domain_Add(pcDomainName, eType, uiNodeID);
    MUTEX_V(&g_stVnetsDomainMutex);

    return uiDomainId;
}

static VOID vnets_domain_Del(IN UINT uiDomainId)
{
    CHAR szDomainName[VNET_CONF_MAX_DOMAIN_NAME_LEN + 1];
    if (BS_OK != _VNETS_DomainRecord_GetDomainName(uiDomainId, szDomainName))
    {
        return;
    }

    _VNETS_DomainId_FreeID(uiDomainId);
    _VNETS_DomainNIM_Del(szDomainName);
    _VNETS_DomainRecord_Del(uiDomainId);
}

static VOID vnets_domain_Reboot(IN CHAR *pcDomainName)
{
    UINT uiDomainId;
    
    uiDomainId = _VNETS_DomainNIM_GetId(pcDomainName);
    if (uiDomainId == 0)
    {
        return;
    }

    VNETS_EnterDomain_RebootDomain(uiDomainId);

    vnets_domain_Del(uiDomainId);
}

static UINT vnets_domain_GetNextNode
(
    IN CHAR *pcDomainName,
    IN UINT uiCurrentNodeId/* 如果为0,则表示得到第一个 */
)
{
    UINT uiDomainId;
    
    uiDomainId = _VNETS_DomainNIM_GetId(pcDomainName);
    if (uiDomainId == 0)
    {
        return 0;
    }

    return _VNETS_DomainRecord_GetNextNode(uiDomainId, uiCurrentNodeId);
}

VOID VNETS_Domain_Del(IN UINT uiDomainId)
{
    MUTEX_P(&g_stVnetsDomainMutex);
    vnets_domain_Del(uiDomainId);
    MUTEX_V(&g_stVnetsDomainMutex);
}

VOID VNETS_Domain_RebootByName(IN CHAR *pcDomainName)
{
    MUTEX_P(&g_stVnetsDomainMutex);
    vnets_domain_Reboot(pcDomainName);
    MUTEX_V(&g_stVnetsDomainMutex);
}

VOID VNETS_Domain_DelNode(IN UINT uiDomainId, IN UINT uiNodeID)
{
    MUTEX_P(&g_stVnetsDomainMutex);
    _VNETS_DomainRecord_DelNode(uiDomainId, uiNodeID);
    MUTEX_V(&g_stVnetsDomainMutex);
}

BOOL_T VNETS_Domain_IsExist(IN UINT uiDomainID)
{
    BOOL_T bIsExit;
    
    MUTEX_P(&g_stVnetsDomainMutex);
    bIsExit = _VNETS_DomainRecord_IsExist(uiDomainID);
    MUTEX_V(&g_stVnetsDomainMutex);

    return bIsExit;
}

BS_STATUS VNETS_Domain_SetProperty
(
    IN UINT uiDomainId,
    IN VNETS_DOMAIN_PROPERTY_E ePropertyIndex,
    IN HANDLE hValue
)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stVnetsDomainMutex);
    eRet = _VNETS_DomainRecord_SetProperty(uiDomainId, ePropertyIndex, hValue);
    MUTEX_V(&g_stVnetsDomainMutex);

    return eRet;
}

BS_STATUS VNETS_Domain_GetProperty
(
    IN UINT uiDomainId,
    IN VNETS_DOMAIN_PROPERTY_E ePropertyIndex,
    OUT HANDLE *phValue
)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stVnetsDomainMutex);
    eRet = _VNETS_DomainRecord_GetProperty(uiDomainId, ePropertyIndex, phValue);
    MUTEX_V(&g_stVnetsDomainMutex);

    return eRet;
}

UINT VNETS_Domain_GetDomainID(IN CHAR *pcDomainName)
{
    UINT uiDomainId;
    
    MUTEX_P(&g_stVnetsDomainMutex);
    uiDomainId = _VNETS_DomainNIM_GetId(pcDomainName);
    MUTEX_V(&g_stVnetsDomainMutex);

    return uiDomainId;
}

BS_STATUS VNETS_Domain_GetDomainName
(
    IN UINT uiDomainId,
    OUT CHAR szDomainName[VNET_CONF_MAX_DOMAIN_NAME_LEN + 1]
)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stVnetsDomainMutex);
    eRet = _VNETS_DomainRecord_GetDomainName(uiDomainId, szDomainName);
    MUTEX_V(&g_stVnetsDomainMutex);

    return eRet;
}

VOID VNETS_Domain_WalkDomain(IN PF_VNETS_DOMAIN_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    MUTEX_P(&g_stVnetsDomainMutex);
    _VNETS_DomainRecord_WalkDomain(pfFunc, hUserHandle);
    MUTEX_V(&g_stVnetsDomainMutex);
}

VOID VNETS_Domain_WalkNode
(
    IN UINT uiDomainID,
    IN PF_VNETS_DOMAIN_NODE_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    MUTEX_P(&g_stVnetsDomainMutex);
    _VNETS_DomainRecord_WalkNode(uiDomainID, pfFunc, hUserHandle);
    MUTEX_V(&g_stVnetsDomainMutex);
}

UINT VNETS_Domain_GetSesCount(IN UINT uiDomainID)
{
    UINT uiCount;
    
    MUTEX_P(&g_stVnetsDomainMutex);
    uiCount = _VNETS_DomainRecord_GetSesCount(uiDomainID);
    MUTEX_V(&g_stVnetsDomainMutex);

    return uiCount;
}

UINT VNETS_Domain_GetNextNode
(
    IN CHAR *pcDomainName,
    IN UINT uiCurrentNodeId/* 如果为0,则表示得到第一个SES ID */
)
{
    UINT uiNodeID = 0;
    
    MUTEX_P(&g_stVnetsDomainMutex);
    uiNodeID = vnets_domain_GetNextNode(pcDomainName, uiCurrentNodeId);
    MUTEX_V(&g_stVnetsDomainMutex);

    return uiNodeID;
}

