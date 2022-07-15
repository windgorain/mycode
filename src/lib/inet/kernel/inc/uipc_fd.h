/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-20
* Description: 
* History:     
******************************************************************************/

#ifndef __UIPC_FD_H_
#define __UIPC_FD_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* f_flags */
#define O_NONBLOCK      00004  /* Non Blocking Open */

typedef struct
{
    VOID *pCtrl;
    INT iFd;
    UINT uiTID;
    UINT f_flags;
    VOID *pSocket;
}UPIC_FD_S;


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__UIPC_FD_H_*/


