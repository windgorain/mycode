/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2008-1-14
* Description: 
* History:     
******************************************************************************/

#ifndef __HTTPPROXY_UTL_H_
#define __HTTPPROXY_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* 创建一个HTTP Proxy实例 */
BS_STATUS HTTP_PROXY_CreateInstance(IN CHAR * pszSsltcpType, IN CHAR *pszSslPolicyName, OUT HANDLE *phId);

/*  删除一个HTTP PROXY实例 */
BS_STATUS HTTP_PROXY_DeleteInstance(IN HANDLE hId);

/* 开始HTTP Proxy服务 */
BS_STATUS HTTP_PROXY_Run(IN HANDLE hId, IN CHAR *pszUrl, IN USHORT usPort, IN UINT ulBackLog);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__HTTPPROXY_UTL_H_*/


