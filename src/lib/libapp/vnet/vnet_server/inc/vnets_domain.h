
#ifndef __VNETS_DOMAIN_H_
#define __VNETS_DOMAIN_H_

#include "../../inc/vnet_conf.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* Domain事件 */
#define VNET_DOMAIN_EVENT_AFTER_CREATE  1
#define VNET_DOMAIN_EVENT_PRE_DESTROY   2

/* 域类型 */
typedef enum
{
    VNETS_DOMAIN_TYPE_NORMAL = 0,    /* 普通域, 只能以字母、数字和双字节字符开头 */
    VNETS_DOMAIN_TYPE_PERSONAL,      /* 用户个人域, 以VNET_DOMAIN_TYPE_CHAR_PERSONAL开头 */
    VNETS_DOMAIN_TYPE_INNER,         /* 预定义内置域, 以VNET_DOMAIN_TYPE_CHAR_INNER开头 */

    VNETS_DOMAIN_TYPE_MAX
}VNETS_DOMAIN_TYPE_E;

typedef enum
{
    VNETS_DOMAIN_PROPERTY_MACTBL = 0,
    VNETS_DOMAIN_WAN_VF_ID,

    VNET_DOMAIN_PROPERTY_MAX  /* 在这个属性前面添加其他属性 */
}VNETS_DOMAIN_PROPERTY_E;

typedef struct
{
    UINT uiDomainID;
    CHAR szDomainName[VNET_CONF_MAX_DOMAIN_NAME_LEN + 1];
    VNETS_DOMAIN_TYPE_E eType;
    HANDLE ahProperty[VNET_DOMAIN_PROPERTY_MAX];
}VNETS_DOMAIN_RECORD_S;


typedef BS_STATUS (*PF_VNET_DOMAIN_EVENT_FUNC)(IN VNETS_DOMAIN_RECORD_S *pstParam, IN UINT uiEvent);
typedef VOID (*PF_VNET_DOMAIN_TIMER_FUNC)(IN VNETS_DOMAIN_RECORD_S *pstParam);

BS_STATUS VNETS_Domain_Init();
UINT VNETS_Domain_Add
(
    IN CHAR *pcDomainName,
    IN UINT uiNodeID
);
VOID VNETS_Domain_Del(IN UINT uiDomainId);
VOID VNETS_Domain_RebootByName(IN CHAR *pcDomainName);
VOID VNETS_Domain_DelNode(IN UINT uiDomainId, IN UINT uiSesId);
BOOL_T VNETS_Domain_IsExist(IN UINT uiDomainID);
BS_STATUS VNETS_Domain_SetProperty
(
    IN UINT uiDomainId,
    IN VNETS_DOMAIN_PROPERTY_E ePropertyIndex,
    IN HANDLE hValue
);
BS_STATUS VNETS_Domain_GetProperty
(
    IN UINT uiDomainId,
    IN VNETS_DOMAIN_PROPERTY_E ePropertyIndex,
    OUT HANDLE *phValue
);

UINT VNETS_Domain_GetDomainID(IN CHAR *pcDomainName);
BS_STATUS VNETS_Domain_GetDomainName
(
    IN UINT uiDomainId,
    OUT CHAR szDomainName[VNET_CONF_MAX_DOMAIN_NAME_LEN + 1]
);
UINT VNETS_InnerDomain_GetPubDomainId();

UINT VNETS_Domain_GetSesCount(IN UINT uiDomainID);

UINT VNETS_Domain_GetNextNode
(
    IN CHAR *pcDomainName,
    IN UINT uiCurrentNodeId/* 如果为0,则表示得到第一个SES ID */
);

typedef BS_WALK_RET_E (*PF_VNETS_DOMAIN_WALK_FUNC)(IN VNETS_DOMAIN_RECORD_S *pstParam, IN HANDLE hUserHandle);

VOID VNETS_Domain_WalkDomain(IN PF_VNETS_DOMAIN_WALK_FUNC pfFunc, IN HANDLE hUserHandle);

typedef BS_WALK_RET_E (*PF_VNETS_DOMAIN_NODE_WALK_FUNC)(IN UINT uiNodeID, IN HANDLE hUserHandle);

VOID VNETS_Domain_WalkNode
(
    IN UINT uiDomainID,
    IN PF_VNETS_DOMAIN_NODE_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
);

UINT VNETS_DomainCmd_GetDomainIdByEnv(IN VOID *pEnv);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_DOMAIN_H_*/


