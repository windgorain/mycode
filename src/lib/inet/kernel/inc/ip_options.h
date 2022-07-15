/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-25
* Description: 
* History:     
******************************************************************************/

#ifndef __IP_OPTIONS_H_
#define __IP_OPTIONS_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

VOID IP_SaveSrcOption (IN MBUF_S *pstMBuf, OUT UCHAR *ucOldIPHeader);
VOID IP_StrIpOptions ( INOUT MBUF_S *pstMBuf );


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IP_OPTIONS_H_*/


