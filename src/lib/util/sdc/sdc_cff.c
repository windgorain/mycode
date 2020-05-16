/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
                
#include "utl/sdc_utl.h"
#include "utl/cff_utl.h"
        
#include "sdc_inner.h"

typedef struct
{
    _SDC_S stSdc;
    CFF_HANDLE hCff;
}_SDC_CFF_S;

static VOID sdc_cff_Destroy(IN SDC_HANDLE hSdc)
{
    _SDC_CFF_S *pstCtrl = hSdc;

    CFF_Close(pstCtrl->hCff);
    MEM_Free(pstCtrl);
}

static BS_STATUS sdc_cff_SetProp(IN SDC_HANDLE hSdc, IN CHAR *pcTag, IN CHAR *pcProp, IN CHAR *pcValue)
{
    _SDC_CFF_S *pstCtrl = hSdc;
    BS_STATUS eRet;

    eRet = CFF_SetPropAsString(pstCtrl->hCff, pcTag, pcProp, pcValue);
    if (eRet == BS_OK)
    {
        CFF_Save(pstCtrl->hCff);
    }

    return eRet;
}

static CHAR * sdc_cff_GetProp(IN SDC_HANDLE hSdc, IN CHAR *pcTag, IN CHAR *pcProp)
{
    _SDC_CFF_S *pstCtrl = hSdc;
    CHAR *pcValue = NULL;

    CFF_GetPropAsString(pstCtrl->hCff, pcTag, pcProp, &pcValue);

    return pcValue;
}

static BS_STATUS sdc_cff_AddTag(IN SDC_HANDLE hSdc, IN CHAR *pcTag)
{
    _SDC_CFF_S *pstCtrl = hSdc;
    BS_STATUS eRet;

    eRet = CFF_AddTag(pstCtrl->hCff, pcTag);
    if (eRet == BS_OK)
    {
        CFF_Save(pstCtrl->hCff);
    }

    return eRet;
}

static BOOL_T sdc_cff_IsTagExist(IN SDC_HANDLE hSdc, IN CHAR *pcTag)
{
    _SDC_CFF_S *pstCtrl = hSdc;

    return CFF_IsTagExist(pstCtrl->hCff, pcTag);
}

static VOID sdc_cff_DeleteTag(IN SDC_HANDLE hSdc, IN CHAR *pcTag)
{
    _SDC_CFF_S *pstCtrl = hSdc;

    CFF_DelTag(pstCtrl->hCff, pcTag);
    CFF_Save(pstCtrl->hCff);
}

static CHAR * sdc_cff_GetNextTag(IN SDC_HANDLE hSdc, IN CHAR *pcCurrentTag)
{
    _SDC_CFF_S *pstCtrl = hSdc;

    return CFF_GetNextTag(pstCtrl->hCff, pcCurrentTag);
}

static UINT sdc_cff_GetTagNum(IN SDC_HANDLE hSdc)
{
    _SDC_CFF_S *pstCtrl = hSdc;
    
    return CFF_GetTagNum(pstCtrl->hCff);
}

_SDC_FUNC_TBL_S g_stSdcCff =
{
    sdc_cff_SetProp,
    sdc_cff_GetProp,
    sdc_cff_AddTag,
    sdc_cff_IsTagExist,
    sdc_cff_DeleteTag,
    sdc_cff_GetNextTag,
    sdc_cff_GetTagNum,
    sdc_cff_Destroy
};

SDC_HANDLE SDC_INI_Create(IN CHAR *pcIniFileName)
{
    _SDC_CFF_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_SDC_CFF_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->hCff = CFF_INI_Open(pcIniFileName, CFF_FLAG_CREATE_IF_NOT_EXIST | CFF_FLAG_SORT);
    if (NULL == pstCtrl->hCff)
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    pstCtrl->stSdc.pstFuncTbl = &g_stSdcCff;

    return pstCtrl;
}



