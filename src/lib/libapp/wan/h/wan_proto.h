/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-15
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_PROTO_H_
#define __WAN_PROTO_H_

#ifdef __cplusplus
    extern "C" {
#endif 

typedef BS_STATUS (*PF_WAN_PROTO_INPUT)(IN MBUF_S *pstMbuf);

BS_STATUS WAN_Proto_Init();
BS_STATUS WAN_Proto_RegProto(IN USHORT usProtoType,  IN PF_WAN_PROTO_INPUT pfFunc);
BS_STATUS WAN_Proto_Input(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType);

#ifdef __cplusplus
    }
#endif 

#endif 


