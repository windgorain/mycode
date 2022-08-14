/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-2-9
* Description: 
* History:     
******************************************************************************/
#include <stdio.h>

#include "bs.h"
#include "bs/loadbs.h"
#include "bs/loadcmd.h"
#include "utl/daemon_utl.h"

int main(int argc, char* argv[])
{
    DAEMON_Init(1, 0);
    SYSINFO_SetConfDir("conf_dft");
    LoadBs_SetArgv(argc, argv);
    LoadBs_SetMainMode();
    LoadBs_Init();
    LoadBs_Main();
	SYSRUN_Exit(0);

	return 0;
}

