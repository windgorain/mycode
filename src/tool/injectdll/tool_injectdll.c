/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-4-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/injectdll.h"
#include "utl/process_utl.h"

static CHAR *g_pszHelpString = "Usage: injectdll executable_file_name dll_file_name";

int Tool_Injectdll(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("%s", g_pszHelpString);
		return -1;
	}

    LoadBs_SetArgv(argc, argv);
    LoadBs_Init();

	INJDLL_CreateProcessAndInjectDll(argv[1], argv[2]);

	return 0;
}

