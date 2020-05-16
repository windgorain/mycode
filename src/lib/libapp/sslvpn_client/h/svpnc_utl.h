/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-2
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPNC_UTL_H_
#define __SVPNC_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

CONN_HANDLE SVPNC_CreateServerConn();
CONN_HANDLE SVPNC_SynConnectServer();
INT SVPNC_ReadHttpBody(IN CONN_HANDLE hConn, IN UCHAR *pucData, IN UINT uiDataSize);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPNC_UTL_H_*/


