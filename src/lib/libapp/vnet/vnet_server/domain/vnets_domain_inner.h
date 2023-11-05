
#ifndef __VNETS_DOMAIN_INNER_H_
#define __VNETS_DOMAIN_INNER_H_

#include "../inc/vnets_domain.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#if 1
BS_STATUS _VNETS_DomainId_Init();
UINT _VNETS_DomainId_Get();
VOID _VNETS_DomainId_FreeID(IN UINT uiId);
#endif

#if 1
BS_STATUS _VNETS_DomainNIM_Init();
BS_STATUS _VNETS_DomainNIM_Add(IN CHAR * pcDomainName, IN UINT uiDomainId);
BS_STATUS _VNETS_DomainNIM_Del(IN CHAR * pcDomainName);
UINT _VNETS_DomainNIM_GetId(IN CHAR * pcDomainName);
#endif

#if 1
BS_STATUS _VNETS_DomainRecord_Init();
BS_STATUS _VNETS_DomainRecord_Add
(
    IN UINT uiDomainID,
    IN CHAR *pcDomainName,
    IN VNETS_DOMAIN_TYPE_E eType,
    IN UINT uiNodeID
);
VOID _VNETS_DomainRecord_Del(IN UINT uiDomainID);
VOID _VNETS_DomainRecord_AddNode
(
    IN UINT uiDomainID,
    IN UINT uiNodeID
);
VOID _VNETS_DomainRecord_DelNode(IN UINT uiDomainID, IN UINT uiNodeID);
BOOL_T _VNETS_DomainRecord_IsExist(IN UINT uiDomainID);
BS_STATUS _VNETS_DomainRecord_SetProperty
(
    IN UINT uiDomainId,
    IN VNETS_DOMAIN_PROPERTY_E ePropertyIndex,
    IN HANDLE hValue
);
BS_STATUS _VNETS_DomainRecord_GetProperty
(
    IN UINT uiDomainId,
    IN VNETS_DOMAIN_PROPERTY_E ePropertyIndex,
    OUT HANDLE *phValue
);
BS_STATUS _VNETS_DomainRecord_GetAllProperty
(
    IN UINT uiDomainID,
    OUT HANDLE *phValue
);
BS_STATUS _VNETS_DomainRecord_GetDomainName
(
    IN UINT uiDomainId,
    OUT CHAR szDomainName[VNET_CONF_MAX_DOMAIN_NAME_LEN + 1]
);
UINT _VNETS_DomainRecord_GetSesCount(IN UINT uiDomainId);
UINT _VNETS_DomainRecord_GetNextNode(IN UINT uiDomainId, IN UINT uiCurrentNodeId);
VOID _VNETS_DomainRecord_WalkDomain(IN PF_VNETS_DOMAIN_WALK_FUNC pfFunc, IN HANDLE hUserHandle);
VOID _VNETS_DomainRecord_WalkNode
(
    IN UINT uiDomainId,
    IN PF_VNETS_DOMAIN_NODE_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
);
#endif

#if 1
BS_STATUS _VNETS_DomainEvent_Issu(IN VNETS_DOMAIN_RECORD_S *pstParam, IN UINT uiEvent);
#endif

#if 1
typedef struct
{
    DLL_HEAD_S stListHead;
}_VNETS_DOMAIN_NODE_TBL_S;
BS_STATUS _VNETS_DomainNode_Init(IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl);
VOID _VNETS_DomainNode_Uninit(IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl);
BS_STATUS _VNETS_DomainNode_Add
(
    IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl,
    IN UINT uiNodeID
);
VOID _VNETS_DomainNode_Del
(
    IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl,
    IN UINT uiNodeID
);
UINT _VNETS_DomainNode_GetCount(IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl);
UINT _VNETS_DomainNode_GetNext
(
    IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl,
    IN UINT uiCurrentNodeID
);
VOID _VNETS_DomainNode_Walk
(
    IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl,
    IN PF_VNETS_DOMAIN_NODE_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
);

#endif

#if 1
BS_STATUS _VNETS_DomainCmd_Init();
#endif

#ifdef __cplusplus
    }
#endif 

#endif 



