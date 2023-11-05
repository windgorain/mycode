/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-4-19
* Description: 
* History:     
******************************************************************************/

#ifndef __TOOLHELP_OS_H_
#define __TOOLHELP_OS_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS ToolHelp_CreateModule(IN UINT ulPid, OUT UINT *pulToolHelpId);
BS_STATUS ToolHelp_GetFirstModule(IN UINT ulToolHelpId, OUT MODULEENTRY32 *pstMe32);
BS_STATUS ToolHelp_GetNextModule(IN UINT ulToolHelpId, OUT MODULEENTRY32 *pstMe32);
VOID ToolHelp_Delete(IN UINT ulToolHelpId);


#ifdef __cplusplus
    }
#endif 

#endif 


