/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-5
* Description: 
* History:     
******************************************************************************/

#ifndef __IP_LIST_H_
#define __IP_LIST_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT uiBeginIp; /* 主机序 */
    UINT uiEndIp;  /* 主机序 */
}IPLIST_NODE_S;

typedef struct
{
    DLL_HEAD_S stList;
}IPLIST_S;

static inline VOID IPList_Init(IN IPLIST_S *pstList)
{
    DLL_INIT(&pstList->stList);
}

VOID IPList_Finit(IN IPLIST_S *pstList);
BS_STATUS IPList_ParseString(IN IPLIST_S *pstList, IN LSTR_S *pstIpStrList);
/* 判断一个IP地址段是否和IpList 有重叠(部分重叠也算) */
BOOL_T IPLIst_IsOverlap(IN IPLIST_S *pstList, IN UINT uiStartIP/* 主机序 */, IN UINT uiEndIP/* 主机序 */);
BOOL_T IPList_IsIPInTheList(IN IPLIST_S *pstList, IN UINT uiIP/* 主机序 */);
BS_STATUS IPList_AddRange(IN IPLIST_S *pstList, IN UINT uiBeginIP, IN UINT uiEndIP);
BS_STATUS IPList_DelRange(IN IPLIST_S *pstList, IN UINT uiBeginIP/* 主机序 */, IN UINT uiEndIP/* 主机序 */);
BS_STATUS IPList_ModifyRange
(
    IN IPLIST_S *pstList,
    IN UINT uiOldBeginIP/* 主机序 */,
    IN UINT uiOldEndIP/* 主机序 */,
    IN UINT uiBeginIP/* 主机序 */,
    IN UINT uiEndIP/* 主机序 */
);
/* 将两个List串成一个 */
VOID IPLIst_Cat(INOUT IPLIST_S *pstListDst, INOUT IPLIST_S *pstListSrc);
/* 对地址段进行排序 */
VOID IPList_Sort(IN IPLIST_S *pstList);
/* 将有重叠部分的地址段合并 */
VOID IPList_Compress(IN IPLIST_S *pstList);
/* 将可以连续的部分进行连续化 */
VOID IPList_Continue(IN IPLIST_S *pstList);
/* 按照网段方式将连续地址进行分割, 比如将:
  1.1.1.1-1.1.1.5分割为 1.1.1.1/255, 1.1.1.2/254, 1.1.1.4/254三个网段*/
BS_STATUS IPList_Range2SubNetList
(
    INOUT IPLIST_S *pstList,
    IN UINT uiBeginIP/* 主机序 */,
    IN UINT uiEndIP/* 主机序 */
);
/* 按照网段方式将IpList进行分割, 比如将:
  1.1.1.1-1.1.1.5分割为 1.1.1.1/255, 1.1.1.2/254, 1.1.1.4/254三个网段*/
BS_STATUS IPLIst_SplitBySubNet(INOUT IPLIST_S *pstList);
BS_STATUS IPLIst_ToString(IN IPLIST_S *pstList, OUT CHAR *pcString, IN UINT uiSize);

#define IPLIST_SCAN_BEGIN(_pstList, _uiBeginIP, _uiEndIP)   do { \
    IPLIST_NODE_S *_pstNode;    \
    DLL_SCAN(&(_pstList)->stList, _pstNode)  \
    {   \
        (_uiBeginIP) = _pstNode->uiBeginIp; \
        (_uiEndIP) = _pstNode->uiEndIp;

#define IPLIST_SCAN_END()   }}while(0)

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IP_LIST_H_*/


