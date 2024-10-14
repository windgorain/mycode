/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/ovsdb_utl.h"

static char g_ovsdb_agent_server[256];
static int g_ovsdb_agent_start = 0;

#ifdef IN_LINUX
static void _ovsdb_core_main(USER_HANDLE_S *pstUserHandle)
{
    g_ovsdb_agent_start = 1;
    OVSDB_Connect(g_ovsdb_agent_server);
    g_ovsdb_agent_start = 0;
}
#endif

int XGW_OVSDB_Start(void)
{
    if (g_ovsdb_agent_server[0] == '\0') {
        RETURN(BS_ERR);
    }

#ifndef IN_LINUX
    EXEC_OutString("Only support linux\r\n");
#else
    if (THREAD_ID_INVALID == THREAD_Create("xgw_ovsdb", NULL, _ovsdb_core_main, NULL)) {
        RETURN(BS_ERR);
    }
#endif

    return 0;
}

int XGW_OVSDB_SetServer(char *server)
{
    int ret = strlcpy(g_ovsdb_agent_server, server, sizeof(g_ovsdb_agent_server));
    if (ret >= sizeof(g_ovsdb_agent_server)) {
        RETURN(BS_OUT_OF_RANGE);
    }

    return 0;
}

char * XGW_OVSDB_GetServer(void)
{
    return g_ovsdb_agent_server;
}

int XGW_OVSDB_IsStarted(void)
{
    return g_ovsdb_agent_start;
}

