/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-16
* Description: 
* History:     
******************************************************************************/

#ifndef __NAT_LINK_H_
#define __NAT_LINK_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS NAT_Link_Init();
BS_STATUS NAT_Link_Input (IN UINT ulIfIndex, IN MBUF_S *pstMbuf);
BS_STATUS NAT_Link_OutPut(IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* net order */);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__NAT_LINK_H_*/


