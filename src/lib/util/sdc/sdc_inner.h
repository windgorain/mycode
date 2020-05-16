/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-22
* Description: 
* History:     
******************************************************************************/

#ifndef __SDC_INNER_H_
#define __SDC_INNER_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef BS_STATUS (*PF_SDC_SetProp)(IN SDC_HANDLE hSdc, IN CHAR *pcTag, IN CHAR *pcProp, IN CHAR *pcValue);
typedef CHAR* (*PF_SDC_GetProp)(IN SDC_HANDLE hSdc, IN CHAR *pcTag, IN CHAR *pcProp);
typedef BS_STATUS (*PF_SDC_AddTag)(IN SDC_HANDLE hSdc, IN CHAR *pcTag);
typedef BOOL_T (*PF_SDC_IsTagExist)(IN SDC_HANDLE hSdc, IN CHAR *pcTag);
typedef VOID (*PF_SDC_DeleteTag)(IN SDC_HANDLE hSdc, IN CHAR *pcTag);
typedef CHAR * (*PF_SDC_GetNextTag)(IN SDC_HANDLE hSdc, IN CHAR *pcCurrentTag);
typedef UINT (*PF_SDC_GetTagNum)(IN SDC_HANDLE hSdc);
typedef VOID (*PF_SDC_Destroy)(IN SDC_HANDLE hSdc);

typedef struct
{
    PF_SDC_SetProp pfSetProp;
    PF_SDC_GetProp pfGetProp;
    PF_SDC_AddTag pfAddTag;
    PF_SDC_IsTagExist pfIsTagExist;
    PF_SDC_DeleteTag pfDeleteTag;
    PF_SDC_GetNextTag pfGetNextTag;
    PF_SDC_GetTagNum pfGetTagNum;
    PF_SDC_Destroy pfDestroy;
}_SDC_FUNC_TBL_S;

typedef struct
{
    _SDC_FUNC_TBL_S *pstFuncTbl;
}_SDC_S;


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SDC_INNER_H_*/


