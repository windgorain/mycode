/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-12
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mime_utl.h"
#include "utl/kf_utl.h"
#include "utl/bit_opt.h"
#include "comp/comp_kfapp.h"

#define KFAPP_DBG_FLAG_RUN 0x1

static KF_HANDLE g_hKfAppHandle;
static UINT g_uiKfAppDbgFlag = 0;

PLUG_API BS_STATUS KFAPP_RegFunc(IN CHAR *pcKey, IN PF_KFAPP_FUNC pfFunc, IN HANDLE hUserHandle)
{
    if ((NULL == pcKey) || (NULL == pfFunc))
    {
        BS_DBGASSERT(0);
        return BS_NULL_PARA;
    }

    return KF_AddFunc(g_hKfAppHandle, pcKey, (PF_KF_FUNC)pfFunc, hUserHandle);
}

PLUG_API BS_STATUS KFAPP_RunMime(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    return KF_RunMime(g_hKfAppHandle, hMime, pstParam);
}

PLUG_API BS_STATUS KFAPP_RunString(IN CHAR *pcString, IN KFAPP_PARAM_S *pstParam)
{
    BS_DBG_OUTPUT(g_uiKfAppDbgFlag, KFAPP_DBG_FLAG_RUN, ("[KFAPP] Run string: %s\r\n", pcString));

    return KF_RunString(g_hKfAppHandle, '&', pcString, pstParam);
}

PLUG_API BS_STATUS KFAPP_RunRaw(char *pcString, KFAPP_PARAM_S *pstParam, char split)
{
    BS_DBG_OUTPUT(g_uiKfAppDbgFlag, KFAPP_DBG_FLAG_RUN, ("[KFAPP] Run string: %s\r\n", pcString));

    return KF_RunString(g_hKfAppHandle, split, pcString, pstParam);
}

PLUG_API BS_STATUS KFAPP_ParamInit(IN KFAPP_PARAM_S *pstParam)
{
    Mem_Zero(pstParam, sizeof(KFAPP_PARAM_S));

    pstParam->pstJson = cJSON_CreateObject();
    if (NULL == pstParam->pstJson)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

PLUG_API VOID KFAPP_ParamFini(IN KFAPP_PARAM_S *pstParam)
{
    if (NULL != pstParam->pstJson)
    {
        cJSON_Delete(pstParam->pstJson);
        pstParam->pstJson = NULL;
    }

    if (NULL != pstParam->pcString)
    {
        free(pstParam->pcString);
        pstParam->pcString = NULL;
    }

    pstParam->uiStringLen = 0;
}

PLUG_API CHAR * KFAPP_BuildParamString(IN KFAPP_PARAM_S *pstParam)
{
    if (NULL != pstParam->pcString)
    {
        free(pstParam->pcString);
    }

    if (pstParam->json_string) {
        pstParam->pcString = strdup(pstParam->json_string);
    } else {
        pstParam->pcString = cJSON_Print(pstParam->pstJson);
    }

    if (NULL != pstParam->pcString)
    {
        pstParam->uiStringLen = strlen(pstParam->pcString);
    }

    return pstParam->pcString;
}

BS_STATUS KFAPP_Init()
{
    g_hKfAppHandle = KF_Create("_do");
    return BS_OK;
}

/* debug kfapp */
PLUG_API BS_STATUS KFAPP_Debug(IN UINT ulArgc, IN CHAR **argv)
{
    BIT_SET(g_uiKfAppDbgFlag, KFAPP_DBG_FLAG_RUN);

    return BS_OK;
}

/* no debug kfapp */
PLUG_API BS_STATUS KFAPP_NoDebug(IN UINT ulArgc, IN CHAR **argv)
{
    BIT_CLR(g_uiKfAppDbgFlag, KFAPP_DBG_FLAG_RUN);

    return BS_OK;
}

