/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-1-9
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_NODE_FWD_H_
#define __VNETS_NODE_FWD_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef BS_STATUS (*PF_VNETS_NODE_PROTO_FUNC)(IN MBUF_S *pstMbuf);

BS_STATUS VNETS_NodeFwd_FwdTo(IN UINT uiDstNodeID, IN MBUF_S *pstMbuf);
BS_STATUS VNETS_NodeFwd_Output(IN UINT uiDstNodeID, IN MBUF_S *pstMBuf, IN USHORT usProto);
BS_STATUS VNETS_NodeFwd_OutputBySes(IN UINT uiSesID, IN MBUF_S *pstMBuf, IN USHORT usProto);

#ifdef __cplusplus
    }
#endif 

#endif 


