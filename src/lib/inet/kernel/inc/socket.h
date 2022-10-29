/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/

#ifndef __SOCKET_H_
#define __SOCKET_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define AF_LOCAL 1
#define AF_INET  2

enum __socket_type
{
  SOCK_STREAM = 1,
  SOCK_DGRAM = 2,
  SOCK_RAW = 3,
  // SOCK_SEQPACKET = 4,
};

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SOCKET_H_*/


