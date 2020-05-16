/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-6-27
* Description: python vm. 用于运行python脚本
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"

BS_STATUS PYVM_Init()
{
    CHAR szFile[FILE_MAX_PATH_LEN + 1];
    
    if (BS_OK != PY_Init())
    {
        return BS_ERR;
    }

    snprintf(szFile, sizeof(szFile), "%s/pyvm_run.py", LOCAL_INFO_GetHostPath());

    FILE_PATH_TO_UNIX(szFile);

    PY_RunFile(szFile);

    return BS_OK;
}


