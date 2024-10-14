/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "../h/xgw_ovsdb.h"


PLUG_API int XGW_CMD_SetOvsdbServer(int argc, char **argv)
{
    if (argc < 3) {
        RETURN(BS_ERR);
    }

    return XGW_OVSDB_SetServer(argv[2]);
}


PLUG_API int XGW_CMD_StartOvsdb(int argc, char **argv)
{
    return XGW_OVSDB_Start();
}

int XGW_CMD_OvsdbSave(void *file)
{
    char *server = XGW_OVSDB_GetServer();

    if (server && server[0]) {
        CMD_EXP_OutputCmd(file, "ovsdb server %s", server);
    }

    if (XGW_OVSDB_IsStarted()) {
        CMD_EXP_OutputCmd(file, "ovsdb start");
    }

    return 0;
}


