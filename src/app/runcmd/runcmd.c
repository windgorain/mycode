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

int main(int argc, char* argv[])
{
    SYSINFO_SetConfDir("conf_dft");
    LoadBs_SetArgv(argc, argv);
    LoadBs_Init();
	Load_Cmd(1);
	SYSRUN_Exit(0);

	return 0;
}

