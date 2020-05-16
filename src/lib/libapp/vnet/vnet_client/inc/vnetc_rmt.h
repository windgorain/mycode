/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-3-15
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_RMT_H_
#define __VNETC_RMT_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

CHAR * VNETC_RMT_GetDomainName();
CHAR * VNETC_RMT_GetUserName();
CHAR * VNETC_RMT_GetUserPasswd();
BS_STATUS VNETC_RMT_SetUserConfig(IN CHAR *pszDomainName, IN CHAR *pszUserName, IN CHAR *pszPasswd);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_RMT_H_*/


