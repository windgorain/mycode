/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-12
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mime_utl.h"
#include "utl/kf_utl.h"
#include "utl/ic_utl.h"
#include "utl/bit_opt.h"
#include "comp/comp_kfapp.h"

#define KFAPP_DBG_FLAG_RUN 0x1

static KF_HANDLE g_hKfAppHandle;
static COMP_KFAPP_S g_stKfAppComp;
static UINT g_uiKfAppDbgFlag = 0;

static BS_STATUS kfapp_RegFunc(IN CHAR *pcKey, IN PF_KFAPP_FUNC pfFunc, IN HANDLE hUserHandle)
{
    if ((NULL == pcKey) || (NULL == pfFunc))
    {
        BS_DBGASSERT(0);
        return BS_NULL_PARA;
    }

    return KF_AddFunc(g_hKfAppHandle, pcKey, (PF_KF_FUNC)pfFunc, hUserHandle);
}

static BS_STATUS kfapp_RunMime(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    return KF_RunMime(g_hKfAppHandle, hMime, pstParam);
}

static BS_STATUS kfapp_RunString(IN CHAR *pcString, IN KFAPP_PARAM_S *pstParam)
{
    BS_DBG_OUTPUT(g_uiKfAppDbgFlag, KFAPP_DBG_FLAG_RUN, ("[KFAPP] Run string: %s\r\n", pcString));

    return KF_RunString(g_hKfAppHandle, '&', pcString, pstParam);
}

static VOID kfapp_InitComp()
{
    g_stKfAppComp.pfRegFunc = kfapp_RegFunc;
    g_stKfAppComp.pfRunMime = kfapp_RunMime;
    g_stKfAppComp.pfRunString = kfapp_RunString;
    g_stKfAppComp.comp.comp_name = COMP_KFAPP_NAME;

    COMP_Reg(&g_stKfAppComp.comp);
}

BS_STATUS KFAPP_Init()
{
    g_hKfAppHandle = KF_Create("_do");

    kfapp_InitComp();

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

