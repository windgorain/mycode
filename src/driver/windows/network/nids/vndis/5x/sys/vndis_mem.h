/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description: 
* History:     
******************************************************************************/

#ifndef __VNDIS_MEM_H_
#define __VNDIS_MEM_H_

#ifdef __cplusplus
    extern "C" {
#endif 

VOID * VNDIS_MEM_Malloc(IN UINT uiLen);
VOID VNDIS_MEM_Free(IN VOID *pMem, IN UINT uiLen);
VOID VNDIS_MEM_Copy(IN VOID *pDst, IN VOID *pSrc, IN UINT uiLen);

#ifdef __cplusplus
    }
#endif 

#endif 


