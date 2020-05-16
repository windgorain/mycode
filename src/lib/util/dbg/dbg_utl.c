/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-9-21
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ic_utl.h"
#include "utl/dbg_utl.h"


VOID DBG_UTL_SetDebugFlag(IN DBG_UTL_CTRL_S *pstCtrl, IN UINT uiModuleID, IN UINT uiFlag)
{
    pstCtrl->puiDbgFlags[pstCtrl->pstDefs[uiModuleID].uiDbgID] |= uiFlag;
}

VOID DBG_UTL_ClrDebugFlag(IN DBG_UTL_CTRL_S *pstCtrl, IN UINT uiModuleID, IN UINT uiFlag)
{
    pstCtrl->puiDbgFlags[pstCtrl->pstDefs[uiModuleID].uiDbgID] &= ~uiFlag;
}

VOID DBG_UTL_DebugCmd(IN DBG_UTL_CTRL_S *pstCtrl, IN CHAR *pcModuleName, IN CHAR *pcDbgName)
{
    UINT i;
    BOOL_T bModuleWild = FALSE; /* 模块名是否通配 */
    BOOL_T bFlagWild = FALSE;   /* debug falg name是否通配 */

    if (strcmp(pcModuleName, "all") == 0)
    {
        bModuleWild = TRUE;
    }

    if (strcmp(pcDbgName, "all") == 0)
    {
        bFlagWild = TRUE;
    }

    for (i=0; i<pstCtrl->uiDefsCount; i++)
    {
        if ((bModuleWild == FALSE) && (strcmp(pstCtrl->pstDefs[i].pcModuleName, pcModuleName) != 0))
        {
            continue;
        }

        if ((bFlagWild == FALSE) && (strcmp(pstCtrl->pstDefs[i].pcDbgFlagName, pcDbgName) != 0))
        {
            continue;
        }

        pstCtrl->puiDbgFlags[pstCtrl->pstDefs[i].uiDbgID] |= pstCtrl->pstDefs[i].uiDbgFlag;
    }
}

VOID DBG_UTL_NoDebugCmd(IN DBG_UTL_CTRL_S *pstCtrl, IN CHAR *pcModuleName, IN CHAR *pcDbgName)
{
    UINT i;
    BOOL_T bModuleWild = FALSE; /* 模块名是否通配 */
    BOOL_T bFlagWild = FALSE;   /* debug falg name是否通配 */

    if (strcmp(pcModuleName, "all") == 0)
    {
        bModuleWild = TRUE;
    }

    if (strcmp(pcDbgName, "all") == 0)
    {
        bFlagWild = TRUE;
    }

    for (i=0; i<pstCtrl->uiDefsCount; i++)
    {
        if ((bModuleWild == FALSE) && (strcmp(pstCtrl->pstDefs[i].pcModuleName, pcModuleName) != 0))
        {
            continue;
        }

        if ((bFlagWild == FALSE) && (strcmp(pstCtrl->pstDefs[i].pcDbgFlagName, pcDbgName) != 0))
        {
            continue;
        }

        pstCtrl->puiDbgFlags[pstCtrl->pstDefs[i].uiDbgID] &= (~pstCtrl->pstDefs[i].uiDbgFlag);
    }
}

static DBG_UTL_DEF_S * _dbg_utl_GetDef(IN DBG_UTL_CTRL_S *pstCtrl, IN UINT uiDbgID, IN UINT uiFlag)
{
    UINT i;

    for (i=0; i<pstCtrl->uiDefsCount; i++)
    {
        if ((pstCtrl->pstDefs[i].uiDbgID == uiDbgID) && (pstCtrl->pstDefs[i].uiDbgFlag == uiFlag))
        {
            return &pstCtrl->pstDefs[i];
        }
    }

    return NULL;
}

VOID DBG_UTL_OutputHeader(IN DBG_UTL_CTRL_S *pstCtrl, IN UINT uiDbgID, IN UINT uiFlag)
{
    DBG_UTL_DEF_S *pstDef;

    pstDef = _dbg_utl_GetDef(pstCtrl, uiDbgID, uiFlag);
    if (NULL == pstDef)
    {
        BS_DBGASSERT(0);
        return;
    }

    IC_Info(DBG_UTL_HEADER_FMT, pstCtrl->pcProductName, pstDef->pcModuleName, pstDef->pcDbgFlagName);
}


