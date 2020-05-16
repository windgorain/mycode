/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-13
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_MAC_FW_H_
#define __VNETC_MAC_FW_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETC_MAC_FW_Input (IN MBUF_S *pstMbuf);
VOID VNETC_MAC_FW_SetPermitBoradcast(IN BOOL_T bPermit);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_MAC_FW_H_*/


