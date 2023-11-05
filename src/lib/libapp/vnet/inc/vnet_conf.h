/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-9-20
* Description: 
* History:     
******************************************************************************/

#ifndef __VNET_CONF_H_
#define __VNET_CONF_H_

#include "utl/passwd_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define VNET_CONF_MAX_DOMAIN_NAME_LEN 63  
#define VNET_CONF_MAX_USER_NAME_LEN 31
#define VNET_CONF_MAX_ALIAS_LEN 31
#define VNET_CONF_MAX_DES_LEN 1023
#define VNET_CONF_MAX_USER_PASSWD_LEN 31
#define VNET_CONF_MAX_USER_ENC_PASSWD_LEN PW_BASE64_CIPHER_LEN(VNET_CONF_MAX_USER_PASSWD_LEN)

#define VNET_CONF_DFT_UDP_PORT 443

#ifdef __cplusplus
    }
#endif 

#endif 


