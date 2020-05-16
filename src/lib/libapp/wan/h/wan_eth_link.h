/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-22
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_ETH_LINK_H_
#define __WAN_ETH_LINK_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS WAN_ETH_LinkOutput (IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 主机序 */);
BS_STATUS WAN_ETH_LinkInput (IN UINT ulIfIndex, IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WAN_ETH_LINK_H_*/


