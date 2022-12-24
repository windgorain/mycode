/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-13
* Description: ifnet, 接口管理
* History:     
******************************************************************************/
#ifndef __IF_UTL_H_
#define __IF_UTL_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define IF_MAX_NAME_LEN 63
#define IF_MAX_NAME_INDEX_LEN 15

#define IF_EVENT_CREATE 0x1
#define IF_EVENT_DELETE 0x2

#define IF_INVALID_INDEX 0

#define IF_TYPE_INDEX_LINKER '-' /* 接口类型和index之间的连接符 */

typedef UINT IF_INDEX;

typedef HANDLE IF_CONTAINER;

/* 接口状态, ucPhyStatus, ucLinkStatus, ucProtocolStatus用到 */
typedef enum
{
    IFNET_DOWN = 0,
    IFNET_UP    
}IF_STATUS_E;

typedef enum
{
    IFNET_CMD_PHY_SET_STATUS = 0,   /* UINT */
    IFNET_CMD_LINK_SET_STATUS,      /* UINT */
    IFNET_CMD_PROTO_SET_STATUS,     /* UINT */
    IFNET_CMD_GET_TYPE,             /* UINT* */
    IFNET_CMD_SET_VRF,              /* UINT */
    IFNET_CMD_GET_VRF,              /* UINT* */
    IFNET_CMD_SET_MAC,              /* MAC_ADDR_S* */
    IFNET_CMD_GET_MAC,              /* MAC_ADDR_S* */
    IFNET_CMD_GET_IFNAME,           /* CHAR szIfName[IF_MAX_NAME_LEN + 1] */
    IFNET_CMD_SET_SHUTDOWN,         /* BOOL_T */
    IFNET_CMD_GET_SHUTDOWN,         /* BOOL_T* */
    IFNET_CMD_IS_L3,                /* BOOL_T* 判断是否L3口 */

    IFNET_CMD_MAX
}IF_IOCTL_CMD_E;

typedef enum
{
    IF_PHY_IOCTL_CMD_SET_SHUTDOWN = 0,  /* BOOL_T */
    IF_PHY_IOCTL_CMD_GET_PRIVATE_INFO,  /* 获取私有信息, LSTR_S*, IN: Lstr_s.pcdata指向缓冲区, lstr_s.uiLen为缓冲区size. 输出为以\0结尾的字符串 */

    IF_PHY_IOCTL_CMD_MAX
}IF_PHY_IOCTL_CMD_E;

typedef enum {
    IF_TYPE_CTL_CMD_CREATE = 0,
    IF_TYPE_CTL_CMD_DELETE,

    IFCTL_CMD_MAX
}IF_TYPE_CTL_CMD_E;

typedef BS_STATUS (*PF_IF_TYPE_CTL)(IF_INDEX ifindex, int cmd/*IF_TYPE_CTL_CMD_XXX*/, void *data);
typedef BS_STATUS (*PF_IF_EVENT_FUNC)(IN IF_INDEX ifIndex, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle);
typedef BS_STATUS (*IF_PROTO_INPUT_FUNC)(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 网络序 */);
typedef BS_STATUS (*IF_LINK_INPUT_FUNC)(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf);
typedef BS_STATUS (*IF_LINK_OUTPUT_FUNC)(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 报文协议类型, 网络序 */);
typedef BS_STATUS (*IF_PHY_OUTPUT_FUNC)(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf);
typedef BS_STATUS (*PF_PHY_IOCTL_FUNC)(IN IF_INDEX ifIndex, IN UINT uiCmd/* IF_PHY_IOCTL_CMD_E */, INOUT VOID *pData);
/* 返回值:
   BS_PROCESSED: 已经拿走处理了报文,调用者无权继续处理报文了
   BS_OK: 成功, 调用者继续处理报文
   其他: 失败, 调用者继续处理报文 */
typedef BS_STATUS (*PF_IF_PKT_PORCESSER_FUNC)(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf);

typedef struct
{
    IF_PROTO_INPUT_FUNC pfProtoInput;
}IF_PROTO_PARAM_S;

typedef struct
{
    IF_LINK_INPUT_FUNC pfLinkInput;
    IF_LINK_OUTPUT_FUNC pfLinkOutput;
}IF_LINK_PARAM_S;

typedef struct
{
    IF_PHY_OUTPUT_FUNC pfPhyOutput;
    PF_PHY_IOCTL_FUNC pfPhyIoctl;
}IF_PHY_PARAM_S;

typedef struct
{
    CHAR *pcProtoType;
    CHAR *pcLinkType;
    CHAR *pcPhyType;
    PF_IF_TYPE_CTL pfTypeCtl;
    UINT uiFlag;  /* COMP_IF_TYPE_FLAG_XXX */
}IF_TYPE_PARAM_S;

IF_CONTAINER IF_CreateContainer(void *memcap);
VOID IF_DestroyContainer(IN IF_CONTAINER hContainer);

/* 申请一个User Data Index */
UINT IF_AllocUserDataIndex(IN IF_CONTAINER hContainer);

BS_STATUS IF_RegEvent(IN IF_CONTAINER hContainer, IN PF_IF_EVENT_FUNC pfEventFunc, IN USER_HANDLE_S *pstUserHandle);

/* 设置网络层参数，如IP/IPX */
BS_STATUS IF_SetProtoType(IN IF_CONTAINER hContainer, IN CHAR *pcProtoType, IN IF_PROTO_PARAM_S *pstParam); /* 设置网络层参数 */
IF_PROTO_PARAM_S * IF_GetProtoType(IN IF_CONTAINER hContainer, IN CHAR *pcProtoType);
BS_STATUS IF_SetLinkType(IN IF_CONTAINER hContainer, IN CHAR *pcLinkType, IN IF_LINK_PARAM_S *pstParam);  /* 设置链路层参数 */
IF_LINK_PARAM_S * IF_GetLinkType(IN IF_CONTAINER hContainer, IN CHAR *pcLinkType);
BS_STATUS IF_SetPhyType(IN IF_CONTAINER hContainer, IN CHAR *pcPhyType, IN IF_PHY_PARAM_S *pstParam);  /* 设置物理层参数 */
IF_PHY_PARAM_S * IF_GetPhyType(IN IF_CONTAINER hContainer, IN CHAR *pcPhyType);
/* 返回TypeID, 0表示失败 */
UINT IF_AddIfType
(
    IN IF_CONTAINER hContainer,
    IN CHAR *pcIfType,
    IN IF_TYPE_PARAM_S *pstParam
);
CHAR * IF_GetTypeNameByType(IN IF_CONTAINER hContainer, IN UINT uiType);
UINT IF_GetIfTypeFlag(IN IF_CONTAINER hContainer, IN CHAR *pcIfType);
IF_INDEX IF_CreateIf(IN IF_CONTAINER hContainer, IN CHAR *pcIfType);
IF_INDEX IF_CreateIfByName(IN IF_CONTAINER hContainer, IN CHAR *pcIfType, IN CHAR *pcIfName);
VOID IF_DeleteIf(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex);
BS_STATUS IF_Ioctl(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex, IN IF_IOCTL_CMD_E enCmd, IN HANDLE hData);
BS_STATUS IF_PhyIoctl(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex, IN IF_PHY_IOCTL_CMD_E enCmd, IN HANDLE hData);
IF_INDEX IF_GetIfIndex(IN IF_CONTAINER hContainer, IN CHAR *pcIfName);
CHAR * IF_GetPhyIndexStringByIfName(IN CHAR *pcName);
#define IF_INVALID_PHY_INDEX 0xffffffff
UINT IF_GetPhyIndexByIfName(char *ifname);
BS_STATUS IF_GetTypeNameByIfName(IN CHAR *pcIfName, OUT CHAR *pcTypeName);
IF_PHY_OUTPUT_FUNC IF_GetPhyOutPut (IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex);
IF_LINK_INPUT_FUNC IF_GetLinkInput (IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex);
IF_LINK_OUTPUT_FUNC IF_GetLinkOutput (IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex);
IF_PROTO_INPUT_FUNC IF_GetProtoInput(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex);
BS_STATUS IF_SetUserData(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex, IN UINT uiUserIndex, IN HANDLE hData);
BS_STATUS IF_GetUserData(IN IF_CONTAINER hContainer, IN IF_INDEX ifIndex, IN UINT uiUserIndex, OUT HANDLE *phData);
UINT IF_GetNext(IN IF_CONTAINER hContainer, IN UINT uiCurrentIfIndex/* 0表示从头开始 */);


#define IF_PKT_PROCESSER_PRI_DFT 10000

typedef enum
{
    IF_PKT_PROCESSER_TYPE_LINKINPUT = 0,
    IF_PKT_PROCESSER_TYPE_LINKOUTPUT,
    IF_PKT_PROCESSER_TYPE_PROTOINPUT,
    IF_PKT_PROCESSER_TYPE_PHYOUTPUT,

    IF_PKT_PROCESSER_TYPE_MAX
}IF_PKT_PROCESSER_TYPE_E;

BS_STATUS IF_RegPktProcesser(
    IN IF_CONTAINER hContainer,
    IN IF_INDEX ifIndex,
    IN IF_PKT_PROCESSER_TYPE_E enType,
    IN UINT uiPri,  /* 优先级,数字越小优先级越高 */
    IN PF_IF_PKT_PORCESSER_FUNC pfFunc
);
VOID * IF_GetPktProcesserList(IN IF_CONTAINER hContainer, IN IF_PKT_PROCESSER_TYPE_E enType, IN IF_INDEX ifIndex);
BS_STATUS IF_NotifyPktProcesser(IN VOID *pProcesserList, IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IF_UTL_H_*/


