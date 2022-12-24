/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-4
* Description: 
* History:     
******************************************************************************/

#ifndef __IPPOOL_UTL_H_
#define __IPPOOL_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE IPPOOL_HANDLE;

typedef BS_WALK_RET_E (*PF_IPPOOL_WALK_FUNC)(IN UINT uiIp/* 主机序 */, IN HANDLE hUserHandle);

IPPOOL_HANDLE IPPOOL_Create();

VOID IPPOOL_Destory(IN IPPOOL_HANDLE hIpPoolHandle);

/* 添加一个地址池 */
BS_STATUS IPPOOL_AddRange
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN UINT uiBeginIP/* 主机序, 含有此IP */,
    IN UINT uiEndIP/* 主机序, 含有此IP */
);

BS_STATUS IPPOOL_DelRange
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN UINT uiBeginIP/* 主机序*/,
    IN UINT uiEndIP/* 主机序*/
);

BS_STATUS IPPOOL_ModifyRange
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN UINT uiOldBeginIP/* 主机序*/,
    IN UINT uiOldEndIP/* 主机序*/,
    IN UINT uiBeginIP/* 主机序*/,
    IN UINT uiEndIP/* 主机序*/
);

/* 判断是否和已经存在的地址池有重叠 */
BOOL_T IPPOOL_IsOverlap
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN UINT uiBeginIP/* 主机序*/,
    IN UINT uiEndIP/* 主机序*/
);

/* 将IP设置为禁止分配 */
VOID IPPOOL_Deny(IN IPPOOL_HANDLE hIpPoolHandle, IN UINT uiDenyIp /* 主机序 */);

VOID IPPOOL_Permit(IN IPPOOL_HANDLE hIpPoolHandle, IN UINT uiIP /* 主机序 */);

VOID IPPOOL_PermitAll(IN IPPOOL_HANDLE hIpPoolHandle);

/* 返回地址, 主机序 */
UINT IPPOOL_AllocIP (IN IPPOOL_HANDLE hIpPoolHandle, IN UINT uiRequestIp/* 优先分配这个IP,如果不行,则自动分配一个. 主机序 */);

/* 申请特定IP */
BS_STATUS IPPOOL_AllocSpecIP(IN IPPOOL_HANDLE hIpPoolHandle, IN UINT uiSpecIp/* 主机序 */);

/* 释放地址回地址池, 主机序 */
VOID IPPOOL_FreeIP (IN IPPOOL_HANDLE hIpPoolHandle, IN UINT ulIp);

VOID IPPOOL_WalkBusy
(
    IN IPPOOL_HANDLE hIpPoolHandle,
    IN PF_IPPOOL_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IPPOOL_UTL_H_*/


