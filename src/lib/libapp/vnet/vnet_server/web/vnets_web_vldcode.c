/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-19
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "vnets_web_vldcode.h"
#include "vnets_web_inner.h"


#define VNETS_WEB_VLDCODE_MAX_NUM (1024*8)

static HANDLE g_hVnetsWebVldCode;

BS_STATUS VNETS_WebVldCode_Init()
{
    g_hVnetsWebVldCode = VLDCODE_CreateInstance(VNETS_WEB_VLDCODE_MAX_NUM);
    if (NULL == g_hVnetsWebVldCode)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

VLDBMP_S * VNETS_WebVldCode_GenImg(OUT UINT *puiClientID)
{
    UINT uiClientID = 0;
    VLDBMP_S *pstBmp;

    pstBmp = VLDCODE_GenVldBmp(g_hVnetsWebVldCode, &uiClientID);
    if (NULL == pstBmp)
    {
        return NULL;
    }

    *puiClientID = uiClientID;

    return pstBmp;
}

VLDCODE_RET_E VNETS_WebVldCode_Check(IN UINT uiClientId, IN CHAR *pcCode)
{
    return VLDCODE_Check(g_hVnetsWebVldCode, uiClientId, pcCode);
}


