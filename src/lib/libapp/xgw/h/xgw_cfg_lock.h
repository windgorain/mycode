/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#ifndef _XGW_CFG_LOCK_H
#define _XGW_CFG_LOCK_H

#ifdef __cplusplus
extern "C" {
#endif

int XGW_CFGLOCK_Init(void);
void XGW_CFGLOCK_Lock(void);
void XGW_CFGLOCK_Unlock(void);

#ifdef __cplusplus
}
#endif
#endif 
