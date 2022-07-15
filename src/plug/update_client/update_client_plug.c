/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"

#include "utl/file_utl.h"
#include "utl/sys_utl.h"
#include "utl/http_aly.h"
#include "utl/cff_utl.h"
#include "utl/txt_utl.h"
#include "utl/process_utl.h"
#include "utl/http_get.h"
#include "utl/update_utl.h"

static VOID uc_UpdateCallBack(IN UPDATE_EVENT_E eEvent, IN VOID *pData, IN VOID *pUserHandle)
{
    switch (eEvent)
    {
        case UPDATE_EVENT_START:
        {
            printf(" Update start.\r\n");
            break;
        }

        case UPDATE_EVENT_UPDATE_FILE:
        {
            UPDATE_FILE_S *pstUpdFile = pData;

            printf(" Updating (%d/%d) %s...",
                pstUpdFile->uiCurrentUpdCount,
                pstUpdFile->uiTotleUpdCount,
                pstUpdFile->pcUpdFile);
            break;
        }

        case UPDATE_EVENT_UPDATE_FILE_RESULT:
        {
            UPDATE_FILE_RESULT_E eResult = HANDLE_UINT(pData);
            CHAR *pcResult;

            switch (eResult)
            {
                case UPDATE_FILE_RESULT_OK:
                case UPDATE_FILE_RESULT_IS_NEWEST:
                {
                    pcResult = "OK";
                    break;
                }

                default:
                {
                    pcResult = "Failed";
                    break;
                }
            }

            printf("%s.\r\n", pcResult);
            break;
        }

        case UPDATE_EVENT_END:
        {
            printf(" Update finish.\r\n");
            break;
        }
    }
}

static VOID uc_Start(IN CHAR *pcVerPath)
{
    CHAR szExecCmd[512] = "正在升级,请稍后 \"";

    if (UPDATE_RET_REBOOT == UPD_Update(pcVerPath, NULL, uc_UpdateCallBack, NULL))
    {
#ifdef IN_WINDOWS
        TXT_Strlcat(szExecCmd, SYS_GetSelfFileName(), sizeof(szExecCmd));
        TXT_Strlcat(szExecCmd, "\"", sizeof(szExecCmd));
        PROCESS_CreateByFile("delayrun.exe", szExecCmd, 0);
#endif
        SYSRUN_Exit(0);
    }
}

/* update %STRING<1-255> */
PLUG_API BS_STATUS UC_CmdStart(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        return BS_ERR;
    }

    uc_Start(argv[1]);

    return BS_OK;
}

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN

