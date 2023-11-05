/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-6
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_DEF_H_
#define __SVPN_DEF_H_

#include "utl/ulm_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define SVPN_SERVICE_NAME_LEN 63   
#define SVPN_CONTEXT_NAME_LEN 63   
#define SVPN_MAX_USER_NAME_LEN ULM_MAX_USER_NAME_LEN  
#define SVPN_MAX_PASSWORD_LEN 63  
#define SVPN_MAX_RES_NAME_LEN 63  

#define SVPN_ONLINE_COOKIE_KEY     "svpnuid"

#define SVPN_PROPERTY_SPLIT ',' 
#define SVPN_PROPERTY_SPLIT_STRING "," 
#define SVPN_PROPERTY_LINE_SPLIT '>' 


#define SVPN_USER_TYPE_ANONYMOUS  0x1
#define SVPN_USER_TYPE_USER       0x2
#define SVPN_USER_TYPE_ADMIN      0x4
#define SVPN_USER_TYPE_ALL    0xffffffff

#ifdef __cplusplus
    }
#endif 

#endif 


