/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-3-23
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "comp/comp_kfapp.h"

static int g_kfbs_inited = 0;

static inline BS_STATUS kfbs_Init()
{
    if (g_kfbs_inited) {
        return 0;
    }

    g_kfbs_inited = 1;

    return COMP_KFAPP_Init();
}

PLUG_API VOID KFBS_Do(IN CHAR *pcCmd, OUT CHAR *pcResult, IN UINT uiResultSize)
{
    KFAPP_PARAM_S stParam;

    if (0 != kfbs_Init()) {
        BS_DBGASSERT(0);
        return;
    }

    TXT_Strlcpy(pcResult, "{}", uiResultSize);

    if (BS_OK != COMP_KFAPP_ParamInit(&stParam)) {
        return;
    }

    COMP_KFAPP_RunString(pcCmd, &stParam);

    if (NULL != COMP_KFAPP_BuildParamString(&stParam)) {
        TXT_Strlcpy(pcResult, stParam.pcString, uiResultSize);
    }

    COMP_KFAPP_ParamFini(&stParam);

    return;
}


