/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2008-1-13
* Description: 
* History:     
******************************************************************************/

/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_PROCESS

#include "bs.h"

#include "pbs/process_pbs.h"
    
#include "utl/txt_utl.h"


#define SUB_PROCESS_FLAG "-processParam"


BS_STATUS PROCESS_RunSubProcess(IN UINT ulArgc, IN CHAR *pszArgv[])
{
    PROCESS_PARAM_S stProcessParam = {0};
    UINT i;
    HANDLE hTmp;

    for (i=0; i<sizeof(stProcessParam.aulParam)/sizeof(UINT); i++)
    {
        TXT_Atoui(pszArgv[3 + i], &stProcessParam.aulParam[i]);
    }

    hTmp = FUNCTBL_Call(pszArgv[2], 1, &stProcessParam);

    return HANDLE_UINT(hTmp);
}


