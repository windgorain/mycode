/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-19
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_WEB_VLDCODE_H_
#define __VNETS_WEB_VLDCODE_H_

#include "utl/vldcode_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

VLDBMP_S * VNETS_WebVldCode_GenImg(OUT UINT *puiClientID);
VLDCODE_RET_E VNETS_WebVldCode_Check(IN UINT uiClientId, IN CHAR *pcCode);

#ifdef __cplusplus
    }
#endif 

#endif 


