/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2008-3-28
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#ifdef IN_WINDOWS
#include <tlhelp32.h> 


BS_STATUS ToolHelp_CreateModule(IN UINT ulPid, OUT UINT *pulToolHelpId)
{
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;

    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ulPid);
    if( hModuleSnap == INVALID_HANDLE_VALUE ) 
    { 
        return BS_ERR;
    }

    *pulToolHelpId = (UINT)hModuleSnap;

    return BS_OK;
}

BS_STATUS ToolHelp_GetFirstModule(IN UINT ulToolHelpId, OUT MODULEENTRY32 *pstMe32)
{
    if (TRUE != Module32First((HANDLE)ulToolHelpId, pstMe32))
    {
        return BS_NO_SUCH;
    }

    return BS_OK;
}

BS_STATUS ToolHelp_GetNextModule(IN UINT ulToolHelpId, OUT MODULEENTRY32 *pstMe32)
{
    if (TRUE != Module32Next((HANDLE)ulToolHelpId, pstMe32))
    {
        return BS_NO_SUCH;
    }

    return BS_OK;
}

VOID ToolHelp_Delete(IN UINT ulToolHelpId)
{
    CloseHandle((HANDLE)ulToolHelpId);
}

#endif

