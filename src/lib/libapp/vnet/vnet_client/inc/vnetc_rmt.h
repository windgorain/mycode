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
#endif 

CHAR * VNETC_RMT_GetDomainName();
CHAR * VNETC_RMT_GetUserName();
CHAR * VNETC_RMT_GetUserPasswd();
int VNETC_RMT_SetUserConfig(U64 p1, U64 p2, U64 p3);

#ifdef __cplusplus
    }
#endif 

#endif 


