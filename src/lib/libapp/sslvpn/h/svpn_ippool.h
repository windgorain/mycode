/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-7-17
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_IPPOOL_H_
#define __SVPN_IPPOOL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS SVPN_IPPOOL_Init();
BS_STATUS SVPN_IPPOOL_AddRange
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiStartIP/* 主机序 */,
    IN UINT uiEndIP/* 主机序 */
);
VOID SVPN_IPPOOL_DelRange
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiStartIP/* 主机序 */,
    IN UINT uiEndIP/* 主机序 */
);

BS_STATUS SVPN_IPPOOL_ModifyRange
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiOldStartIP/* 主机序 */,
    IN UINT uiOldEndIP/* 主机序 */,
    IN UINT uiStartIP/* 主机序 */,
    IN UINT uiEndIP/* 主机序 */
);

/* 申请一个IP，返回主机序IP */
UINT SVPN_IPPOOL_AllocIP(IN SVPN_CONTEXT_HANDLE hSvpnContext);

VOID SVPN_IPPOOL_FreeIP(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiIP/* 主机序 */);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPN_IPPOOL_H_*/


