/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-9-21
* Description: 
* History:     
******************************************************************************/

#ifndef __DBG_UTL_H_
#define __DBG_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define DBG_UTL_HEADER_FMT "[%s:%s:%s] "

typedef struct
{
    CHAR *pcModuleName;
    CHAR *pcDbgFlagName;
    UINT uiDbgID;
    UINT uiDbgFlag;
}DBG_UTL_DEF_S;

typedef struct
{
    CHAR *pcProductName;
    UINT *puiDbgFlags;
    DBG_UTL_DEF_S *pstDefs;
    UINT uiDefsCount;
}DBG_UTL_CTRL_S;

#define DBG_INIT_VALUE(_pcProductName, _puiDbgFlags, _pstDefs, _uiDefsCount) \
    {(_pcProductName), (_puiDbgFlags), (_pstDefs), (_uiDefsCount)}

#define DBG_UTL_IS_SWITCH_ON(_pstCtrl, _DbgID, _DbgFlag) ((_pstCtrl)->puiDbgFlags[_DbgID] & (_DbgFlag))
    
#define DBG_UTL_OUTPUT(_pstCtrl, _DbgID, _DbgFlag, _X) do {\
        if ((_pstCtrl)->puiDbgFlags[_DbgID] & (_DbgFlag))  { \
            DBG_UTL_OutputHeader(_pstCtrl, _DbgID, _DbgFlag);   \
            IC_Info _X;	\
        }   \
    }while(0)


extern VOID DBG_UTL_SetDebugFlag(IN DBG_UTL_CTRL_S *pstCtrl, IN UINT uiModuleID, IN UINT uiFlag);
extern VOID DBG_UTL_ClrDebugFlag(IN DBG_UTL_CTRL_S *pstCtrl, IN UINT uiModuleID, IN UINT uiFlag);
extern VOID DBG_UTL_DebugCmd(IN DBG_UTL_CTRL_S *pstCtrl, IN CHAR *pcModuleName, IN CHAR *pcDbgName);
extern VOID DBG_UTL_NoDebugCmd(IN DBG_UTL_CTRL_S *pstCtrl, IN CHAR *pcModuleName, IN CHAR *pcDbgName);
extern VOID DBG_UTL_OutputHeader(IN DBG_UTL_CTRL_S *pstCtrl, IN UINT uiDbgID, IN UINT uiFlag);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__DBG_UTL_H_*/


