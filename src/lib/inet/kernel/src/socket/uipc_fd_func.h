/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-24
* Description: 
* History:     
******************************************************************************/

#ifndef __UIPC_FD_FUNC_H_
#define __UIPC_FD_FUNC_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS UIPC_FD_Init();
INT UIPC_FD_FAlloc(OUT UPIC_FD_S **ppstFp);
VOID UIPC_FD_FClose(IN UPIC_FD_S *pstFp);
VOID UIPC_FD_Attach(IN UPIC_FD_S *pstFp, IN VOID *pSocket);
VOID * UIPC_FD_GetFp(IN INT iFd);
VOID * UIPC_FD_GetSocket(IN UPIC_FD_S *pstFp);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__UIPC_FD_FUNC_H_*/


