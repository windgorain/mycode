/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-9-23
* Description: 
* History:     
******************************************************************************/

#ifndef __VNET_VPN_LINK_H_
#define __VNET_VPN_LINK_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#include "utl/mbuf_utl.h"

#define _VNET_VPN_LINK_VER 0

typedef struct
{
    USHORT usVer;
    USHORT usProto;    /* 承载的上层协议 */
    USHORT usPktLen;
    USHORT usReserved;
}VNET_VPN_LINK_HEAD_S;

BS_STATUS VNET_VpnLink_Encrypt(IN MBUF_S *pstMbuf, IN BOOL_T bIsEnc);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNET_VPN_LINK_H_*/


