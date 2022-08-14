/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-8-30
* Description: 系统调用
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"
#include "utl/mime_utl.h"
#include "comp/comp_kfapp.h"

static BS_STATUS _sysmdl_run(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcCmd;
    CHAR *pcHide;
    UINT uiMode = SW_SHOW;

    pcCmd = MIME_GetKeyValue(hMime, "cmd");
    if ((NULL == pcCmd) || (pcCmd[0] == '\0'))
    {
        JSON_SetFailed(pstParam->pstJson, "Param error");
        return BS_ERR;
    }

    pcHide = MIME_GetKeyValue(hMime, "hide");
    if ((pcHide != NULL) && (stricmp(pcHide, "true") == 0))
    {
        uiMode = SW_HIDE;
    }
    
    WinExec(pcCmd, uiMode);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

BS_STATUS SysMdl_Init()
{
    return COMP_KFAPP_RegFunc("syscaller.run", _sysmdl_run, NULL);
}


