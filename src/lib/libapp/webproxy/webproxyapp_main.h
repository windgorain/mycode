/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-8
* Description: 
* History:     
******************************************************************************/

#ifndef __WEBPROXYAPP_MAIN_H_
#define __WEBPROXYAPP_MAIN_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

WS_DELIVER_RET_E WebProxyApp_Main_ReferIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);
WS_DELIVER_RET_E WebProxyApp_Main_AuthIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);
WS_DELIVER_RET_E WebProxyApp_Main_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WEBPROXYAPP_MAIN_H_*/


