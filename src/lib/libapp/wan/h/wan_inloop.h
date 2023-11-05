/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-4-2
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_INLOOP_H_
#define __WAN_INLOOP_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS WAN_InLoop_Init();
UINT WAN_InLoop_GetIfIndex();
BS_STATUS WAN_InLoop_LinkOutput (IN UINT uiIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType);

#ifdef __cplusplus
    }
#endif 

#endif 


