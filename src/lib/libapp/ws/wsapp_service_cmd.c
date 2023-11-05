/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-3
* Description: 虚服务
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

#include "wsapp_service.h"
#include "wsapp_cfglock.h"

typedef struct
{
    BOOL_T bNo;
    CHAR *pcGateWay;
    CHAR *pcVHost;
    CHAR *pcDomain;
}WSAPP_SERVICE_CMD_BIND_GATEWAY_INFO_S;


PLUG_API BS_STATUS WSAPP_ServiceCmd_EnterView(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    if (ulArgc < 2)
    {
        return BS_ERR;
    }

    WSAPP_CfgLock_WLock();
    if (NULL == WSAPP_Service_GetByName(argv[1]))
    {
        if (NULL == WSAPP_Service_Add(argv[1]))
        {
            EXEC_OutString("Create service failed.\r\n");
        }
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}


PLUG_API BS_STATUS WSAPP_ServiceCmd_WebCenterOpt(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;
    BS_STATUS eRet;

    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    if (argv[0][0] == 'n')
    {
        eRet = WSAPP_Service_ClrWebCenterOpt(pcGwName, argv[2]);
    }
    else
    {
        eRet = WSAPP_Service_SetWebCenterOpt(pcGwName, argv[1]);
    }
    WSAPP_CfgLock_WUnLock();

    if (eRet != BS_OK)
    {
        EXEC_OutString("Operation failed.\r\n");
    }

    return BS_OK;
}


PLUG_API BS_STATUS WSAPP_ServiceCmd_Description(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;

    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    if (BS_OK != WSAPP_Service_SetDescription(pcGwName, argv[1]))
    {
        EXEC_OutString("Operation failed.\r\n");
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

static VOID wsapp_servicecmd_BindGatewayInfo
(
    IN UINT ulArgc,
    IN CHAR **argv,
    OUT WSAPP_SERVICE_CMD_BIND_GATEWAY_INFO_S *pstInfo
)
{
    Mem_Zero(pstInfo, sizeof(WSAPP_SERVICE_CMD_BIND_GATEWAY_INFO_S));

    if (argv[0][0] == 'n')
    {
        pstInfo->bNo = TRUE;
        argv ++;
        ulArgc --;
    }

    pstInfo->pcGateWay = argv[2];
    if (ulArgc >= 5)
    {
        if (argv[3][0] == 'v')
        {
            pstInfo->pcVHost = argv[4];
        }
        else if (argv[3][0] == 'd')
        {
            pstInfo->pcDomain = argv[4];
        }
    }

    if (ulArgc >= 7)
    {
        if (argv[5][0] == 'v')
        {
            pstInfo->pcVHost = argv[6];
        }
        else if (argv[5][0] == 'd')
        {
            pstInfo->pcDomain = (CHAR*)argv[6];
        }
    }

    if (pstInfo->pcVHost == NULL)
    {
        pstInfo->pcVHost = "";
    }

    if (pstInfo->pcDomain == NULL)
    {
        pstInfo->pcDomain = "";
    }

    return;
}


PLUG_API BS_STATUS WSAPP_ServiceCmd_BindGateway(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    WSAPP_SERVICE_CMD_BIND_GATEWAY_INFO_S stInfo;
    BS_STATUS eRet;

    wsapp_servicecmd_BindGatewayInfo(ulArgc, argv, &stInfo);

    WSAPP_CfgLock_WLock();
    if (stInfo.bNo == FALSE)
    {
        eRet = WSAPP_Service_BindGateway(CMD_EXP_GetCurrentModeValue(pEnv), stInfo.pcGateWay, stInfo.pcVHost, stInfo.pcDomain);
    }
    else
    {
        eRet = WSAPP_Service_NoBindGateway(CMD_EXP_GetCurrentModeValue(pEnv), stInfo.pcGateWay, stInfo.pcVHost, stInfo.pcDomain);
    }
    WSAPP_CfgLock_WUnLock();

    return eRet;
}



PLUG_API BS_STATUS WSAPP_ServiceCmd_Enable(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    CHAR *pcServiceName;

    pcServiceName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    if (BS_OK != WSAPP_Service_Start(pcServiceName))
    {
        EXEC_OutString("Start failed.\r\n");
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}


PLUG_API BS_STATUS WSAPP_ServiceCmd_NoEnable(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    CHAR *pcService;

    pcService = CMD_EXP_GetCurrentModeValue(pEnv);
    
    WSAPP_CfgLock_WLock();
    WSAPP_Service_Stop(pcService);
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

BS_STATUS WSAPP_ServiceCmd_Save(IN HANDLE hFile)
{
    UINT uiCurId = 0;
    CHAR szTmp[512];
    CHAR *pcTmp;
    WSAPP_SERVICE_S *pstService;
    INT iLen;
    UINT i;

    while ((uiCurId = WSAPP_Service_GetNextID(uiCurId)) != 0)
    {
        pstService = WSAPP_Service_GetByID(uiCurId);

        pcTmp = WSAPP_Service_GetNameByID(uiCurId);

        if (CMD_EXP_IsSaving(hFile) && (pstService->uiFlag & WSAPP_SERVICE_FLAG_SAVE_HIDE))
        {
            continue;
        }

        if (0 != CmdExp_OutputMode(hFile, "service %s", pcTmp)) {
            continue;
        }

        if (WSAPP_Service_IsWebCenterOptHide(pstService))
        {
            CMD_EXP_OutputCmd(hFile, "webcenter hide");
        }

        if (WSAPP_Service_IsWebCenterOptReadonly(pstService))
        {
            CMD_EXP_OutputCmd(hFile, "webcenter readonly");
        }

        pcTmp = WSAPP_Service_GetDescription(pstService);
        if (! TXT_IS_EMPTY(pcTmp))
        {
            CMD_EXP_OutputCmd(hFile, "description %s ", pcTmp);
        }

        for (i=0; i<WSAPP_SERVICE_MAX_BIND_NUM; i++)
        {
            if (pstService->astBindGateWay[i].szBindGwName[0] != '\0')
            {
                iLen = snprintf(szTmp, sizeof(szTmp), "bind gateway %s", pstService->astBindGateWay[i].szBindGwName);
                if (pstService->astBindGateWay[i].szVHostName[0] != '\0')
                {
                    iLen += snprintf(szTmp + iLen, sizeof(szTmp) - iLen, " vhost %s", pstService->astBindGateWay[i].szVHostName);
                }
                if (pstService->astBindGateWay[i].szDomain[0] != '\0')
                {
                    iLen += snprintf(szTmp + iLen, sizeof(szTmp) - iLen, " domain %s", pstService->astBindGateWay[i].szDomain);
                }

                CMD_EXP_OutputCmd(hFile, "%s", szTmp);
            }
        }

        if (pstService->bStart == TRUE)
        {
            CMD_EXP_OutputCmd(hFile, "service enable");
        }
        
        CMD_EXP_OutputModeQuit(hFile);
    }

    return BS_OK;
}

