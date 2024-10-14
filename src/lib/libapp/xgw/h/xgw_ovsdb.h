/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#ifndef _XGW_OVSDB_H
#define _XGW_OVSDB_H

#ifdef __cplusplus
extern "C" {
#endif

int XGW_OVSDB_Start(void);
int XGW_OVSDB_SetServer(char *server);
char * XGW_OVSDB_GetServer(void);
int XGW_OVSDB_IsStarted(void);
int XGW_CMD_OvsdbSave(void *file);


#ifdef __cplusplus
}
#endif
#endif 
