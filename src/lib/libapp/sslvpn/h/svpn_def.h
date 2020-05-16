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
#endif /* __cplusplus */

#define SVPN_SERVICE_NAME_LEN 63   /* 最大Service name名长度 */
#define SVPN_CONTEXT_NAME_LEN 63   /* 最大context name名长度 */
#define SVPN_MAX_USER_NAME_LEN ULM_MAX_USER_NAME_LEN  /* 最大用户名长度 */
#define SVPN_MAX_PASSWORD_LEN 63  /* 最大用户名长度 */
#define SVPN_MAX_RES_NAME_LEN 63  /* 最大资源名长度 */

#define SVPN_ONLINE_COOKIE_KEY     "svpnuid"

#define SVPN_PROPERTY_SPLIT ',' /* 属性中的分隔符 */
#define SVPN_PROPERTY_SPLIT_STRING "," /* 属性中的分隔符 */
#define SVPN_PROPERTY_LINE_SPLIT '>' /* 属性中的分隔符 */

/* 定义用户类型 */
#define SVPN_USER_TYPE_ANONYMOUS  0x1
#define SVPN_USER_TYPE_USER       0x2
#define SVPN_USER_TYPE_ADMIN      0x4
#define SVPN_USER_TYPE_ALL    0xffffffff

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPN_DEF_H_*/


