/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sem_utl.h"
#include "utl/plug_utl.h"
#include "utl/plug_mgr.h"
#include "utl/file_utl.h"
#include "utl/cff_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/lasterr_utl.h"



static PLUG_MGR_S g_stPlugctMgr;

BS_STATUS PLUGCT_Init()
{
    char buf[256];

    PlugMgr_Init(&g_stPlugctMgr);

    return PlugMgr_LoadByCfgFile(&g_stPlugctMgr, SYSINFO_ExpandConfPath(buf, sizeof(buf), "plug_config.ini"));
}

static int _plugct_LoadManual(char *plug_name)
{
    char buf[256];
    return PlugMgr_LoadManual(&g_stPlugctMgr, SYSINFO_ExpandConfPath(buf, sizeof(buf), "plug_config.ini"), plug_name);
}


BS_STATUS PLUGCT_LoadPlug(IN UINT uiArgc, IN CHAR **ppcArgv)
{
    if (uiArgc < 3)
    {
        return BS_ERR;
    }

    _plugct_LoadManual(ppcArgv[2]);

    return BS_OK;
}

static void _plugct_ShowOne(PLUG_MGR_NODE_S *node)
{
    EXEC_OutInfo("%-16s %s \r\n", node->plug_name, node->filename);
}

PLUG_API int PLUGCT_ShowPlug(UINT argc, char **argv)
{
    PLUG_MGR_NODE_S *node = NULL;

    EXEC_OutInfo("plug_name        filename \r\n");
    EXEC_OutInfo("-------------------------------------------------\r\n");

    while ((node= PlugMgr_Next(&g_stPlugctMgr, node))) {
        _plugct_ShowOne(node);
    }

    return 0;
}

PLUG_API UINT PLUGCT_GetLoadStage()
{
    return PlugMgr_GetStage(&g_stPlugctMgr);
}

