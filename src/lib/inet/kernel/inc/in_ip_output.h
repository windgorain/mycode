/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-12-21
* Description: 
* History:     
******************************************************************************/

#ifndef __IN_IP_OUTPUT_H_
#define __IN_IP_OUTPUT_H_

#ifdef __cplusplus
    extern "C" {
#endif 


BS_STATUS IN_IP_Output
(
    IN MBUF_S *pstMBuf,
    IN MBUF_S *pstMOpt,
    IN LONG lFlags,
    IN IPMOPTIONS_S *pstIpMo
);


#ifdef __cplusplus
    }
#endif 

#endif 


