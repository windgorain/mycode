/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/

#ifndef __UDP_DEF_H_
#define __UDP_DEF_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


typedef struct
{
    INPCB_INFO_S udbinfo;
    INPCB_HEAD_S stAllPcbList;
    int udp_blackhole;
    int udp_cksum;
}UDP_CTRL_S;

#define V_udbinfo (g_stUdpUsrreq.udbinfo)
#define V_udp_cksum (g_stUdpUsrreq.udp_cksum)

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__UDP_DEF_H_*/


