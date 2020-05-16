/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-22
* Description: Simple Data Container. 只有三级: Section, Key,Value.  比DC少了TableName
* History:     
******************************************************************************/
#include "bs.h"
            
#include "utl/sdc_utl.h"
    
#include "sdc_inner.h"


VOID SDC_Destroy(IN SDC_HANDLE hSdc)
{
    _SDC_S *pstSdc = hSdc;

    pstSdc->pstFuncTbl->pfDestroy(hSdc);
}

BS_STATUS SDC_SetProp(IN SDC_HANDLE hSdc, IN CHAR *pcTag, IN CHAR *pcProp, IN CHAR *pcValue)
{
    _SDC_S *pstSdc = hSdc;
    BS_STATUS eRet;

    if ((NULL == hSdc) || (pcTag == NULL) || (pcProp == NULL) || (pcValue == NULL))
    {
        return BS_NULL_PARA;
    }

    eRet = pstSdc->pstFuncTbl->pfSetProp(hSdc, pcTag, pcProp, pcValue);

    return eRet;
}

CHAR * SDC_GetProp(IN SDC_HANDLE hSdc, IN CHAR *pcTag, IN CHAR *pcProp)
{
    _SDC_S *pstSdc = hSdc;

    if ((NULL == hSdc) || (pcTag == NULL) || (pcProp == NULL))
    {
        return NULL;
    }

    return pstSdc->pstFuncTbl->pfGetProp(hSdc, pcTag, pcProp);
}

BS_STATUS SDC_AddTag(IN SDC_HANDLE hSdc, IN CHAR *pcTag)
{
    _SDC_S *pstSdc = hSdc;

    if ((NULL == hSdc) || (pcTag == NULL))
    {
        return BS_NULL_PARA;
    }

    return pstSdc->pstFuncTbl->pfAddTag(hSdc, pcTag);
}

BOOL_T SDC_IsTagExist(IN SDC_HANDLE hSdc, IN CHAR *pcTag)
{
    _SDC_S *pstSdc = hSdc;

    if ((NULL == hSdc) || (pcTag == NULL))
    {
        return FALSE;
    }

    return pstSdc->pstFuncTbl->pfIsTagExist(hSdc, pcTag);
}

VOID SDC_DeleteTag(IN SDC_HANDLE hSdc, IN CHAR *pcTag)
{
    _SDC_S *pstSdc = hSdc;

    if ((NULL == hSdc) || (pcTag == NULL))
    {
        return;
    }

    pstSdc->pstFuncTbl->pfDeleteTag(hSdc, pcTag);
}

CHAR * SDC_GetNextTag(IN SDC_HANDLE hSdc, IN CHAR *pcCurrentTag)
{
    _SDC_S *pstSdc = hSdc;
    return pstSdc->pstFuncTbl->pfGetNextTag(hSdc, pcCurrentTag);
}

UINT SDC_GetTagNum(IN SDC_HANDLE hSdc)
{
    _SDC_S *pstSdc = hSdc;
    return pstSdc->pstFuncTbl->pfGetTagNum(hSdc);
}

