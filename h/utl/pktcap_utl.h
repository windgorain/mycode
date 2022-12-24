

#ifndef __PKTCAP_UTL_H_
#define __PKTCAP_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define PKTCAP_NDIS_NAME_MAX_LEN 127

#define PKTCAP_FLAG_PROMISCUOUS 0x1 /* 混杂模式 */

typedef HANDLE PKTCAP_NDIS_HANDLE; /* 被打开的网卡句柄 */
typedef HANDLE PKTCAP_NIDS_INFO_HANDLE; /* 系统中所有网卡的信息句柄 */

typedef struct
{
    struct timeval ts; /* 从1970年1月1日00::00开始的时间 */
    UINT uiPktRawLen; /* 报文原始长度 */
    UINT uiPktCaptureLen; /* 捕获的长度 */
}PKTCAP_PKT_INFO_S;

typedef VOID (*PF_PKT_CAP_PACKET_IN_FUNC)(IN UCHAR *pucPktData, IN PKTCAP_PKT_INFO_S *pstPktInfo, IN HANDLE hUserHandle);

#define PKTCAP_NDIS_MAX_ADDR_NUM 16 /* 最多支持获取多少个地址信息, 不够用的话将来再扩充 */

typedef struct
{
    UINT uiAddr;
    UINT uiMask;
}PKTCAP_NDIS_ADDR_S;

typedef struct
{
    PKTCAP_NDIS_ADDR_S astAddr[PKTCAP_NDIS_MAX_ADDR_NUM];    
}PKTCAP_NDIS_INFO_S;

/*  获取网卡信息 */
PKTCAP_NIDS_INFO_HANDLE PKTCAP_GetNdisInfoList();

PKTCAP_NIDS_INFO_HANDLE PKTCAP_FindNdisByNameInNdisInfoList(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfoList, IN CHAR *pcName);

PKTCAP_NIDS_INFO_HANDLE PKTCAP_GetNextNdisInfo(IN PKTCAP_NIDS_INFO_HANDLE hCurrentNdisInfo);

CHAR * PKTCAP_GetNdisInfoDesc(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo);

CHAR * PKTCAP_GetNdisInfoName(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo);

VOID PKTCAP_GetNdisInfoString(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo, IN UINT uiSize, OUT CHAR *pcString);

BOOL_T PKTCAP_IsNdisExist(IN CHAR *pcName);

/* 打印网卡信息 */
VOID PKTCAP_PrintfNdisInfo(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo);

#define PKTCAP_INVALID_INDEX 0xffffffff
/* 根据Name获取Index */
UINT PKTCAP_GetNdisIndexByName(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo, IN CHAR *pcNdisName);

/* 根据Index获取网卡名 */
CHAR * PKTCAP_GetNdisNameByIndex(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo, IN UINT uiIndex);

BS_STATUS PKTCAP_GetNdisInfoByName
(
    IN CHAR *pcNdisName,
    OUT PKTCAP_NDIS_INFO_S *pstInfo
);
BS_STATUS PKTCAP_GetNdisInfoByNameEx
(
    IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo,
    IN CHAR *pcNdisName,
    OUT PKTCAP_NDIS_INFO_S *pstInfo
);
/* 释放网卡信息 */
VOID PKTCAP_FreeNdisInfoList(IN PKTCAP_NIDS_INFO_HANDLE hNdisInfo);

/* 打开网卡 */
PKTCAP_NDIS_HANDLE PKTCAP_OpenNdis(IN CHAR *pcNdisName, IN UINT uiFlag, IN UINT uiTimeOutTime/* ms, 0表示永不超时 */);

INT PKTCAP_GetDataLinkTypeByName(IN CHAR *pcDevName);

INT PKTCAP_GetDataLinkType(IN PKTCAP_NDIS_HANDLE hNdisHandle);

BOOL_T PKTCAP_IsEthDataLink(IN INT iDataLink);

/* 关闭网卡 */
VOID PKTCAP_CloseNdis(IN PKTCAP_NDIS_HANDLE hNdisHandle);

/* 报文发送 */
BOOL_T PKTCAP_SendPacket
(
    IN PKTCAP_NDIS_HANDLE hNdisHandle,
    IN UCHAR *pucPkt,
    IN UINT uiPktLen
);

/* 过滤规则设置 */
BOOL_T PKTCAP_SetFilter(IN PKTCAP_NDIS_HANDLE hNdisHandle, IN CHAR* pcFilter);

/* 支持超时方式的抓包 */
INT PKTCAP_Dispatch
(
    IN PKTCAP_NDIS_HANDLE hNdisHandle,
    IN PF_PKT_CAP_PACKET_IN_FUNC pfFunc,
    IN HANDLE hUserHandle
);

/* 永久抓包 */
INT PKTCAP_Loop
(
    IN PKTCAP_NDIS_HANDLE hNdisHandle,
    IN PF_PKT_CAP_PACKET_IN_FUNC pfFunc,
    IN HANDLE hUserHandle
);

VOID PKTCAP_BreakLoop(IN PKTCAP_NDIS_HANDLE hNdisHandle);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__PKTCAP_UTL_H_*/

