/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-5-20
* Description: 
* History:     
******************************************************************************/

#ifndef __IP_STRING_H_
#define __IP_STRING_H_

#include "utl/ip_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


/* 解析字符串 IP/Mask */
BS_STATUS IPString_ParseIpMask(IN CHAR *pcIpMaskString, OUT IP_MASK_S *pstIpMask);

/* 解析字符串IP/Mask列表,比如:1.1.1.1/255.0.0.0,2.1.1.1./255.0.0.0,
   返回值: IP个数
*/
UINT IPString_ParseIpMaskList(IN CHAR *pcIpMaskString, IN CHAR cSplitChar, IN UINT uiIpMaskMaxNum, OUT IP_MASK_S *pstIpMasks);
CHAR * IPString_IP2String(IN UINT ip/*net order*/, OUT CHAR *str);
CHAR * IPString_IPHeader2String(IN VOID *ippkt, OUT CHAR *info, IN UINT infosize);
CHAR * IPString_IPHeader2Hex(IN VOID *ippkt, OUT CHAR *info);
INT IPString_IpMask2String_OutIpPrefix(IN IP_MASK_S *pstIpMask, IN INT iStrLen, OUT CHAR *str);

int IPString_ParseIpPort(CHAR *ip_port_string, OUT IP_PORT_S *out);
int IPString_ParseIpPrefix(CHAR *pcIpPrefixString, OUT IP_PREFIX_S *pstIpPrefix);
int IPString_IpPrefixString2IpMask(CHAR *pcIpPrefixString, OUT IP_MASK_S *pstIpMask);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IP_STRING_H_*/


