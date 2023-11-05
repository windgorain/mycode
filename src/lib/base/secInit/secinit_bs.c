/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-10
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_SECINIT

#include "bs.h"


#define _SEC_INIT_ELEMENT(pfFunc, eStage, eOrder)    {{0}, (BS_STATUS_FUNC)pfFunc, #pfFunc, eStage, eOrder}

typedef enum
{
    SECINIT_STAGE_INIT0 = 0,
    SECINIT_STAGE_INIT1,         
    SECINIT_STAGE_INIT2, 
    SECINIT_STAGE_START,         
    SECINIT_STAGE_RUN,           
    SECINIT_STAGE_NORMAL,        
    SECINIT_STAGE_LOW,           
    SECINIT_STAGE_NO_SBUPROCESS, 
    SECINIT_STAGE_CMD_RESTORE,   
    SECINIT_STAGE_LAST           
}SECINIT_STAGE_E;


typedef enum
{
    SECINIT_ORDER_FIRST = 0,
    SECINIT_ORDER_NORMAL,
    SECINIT_ORDER_LAST
}SECINIT_ORDER_E;

typedef struct
{
    DLL_NODE_S           stDllNode;
    BS_STATUS_FUNC       pfFunc;
    CHAR                 *pucFuncName;
    SECINIT_STAGE_E      eInitStage;
    SECINIT_ORDER_E      eInitOrder;
}_SEC_INIT_CTRL_S;

static _SEC_INIT_CTRL_S g_stBsInitTable[] =  
{
    _SEC_INIT_ELEMENT (SYSINFO_Init1,SECINIT_STAGE_INIT1, SECINIT_ORDER_NORMAL),
    _SEC_INIT_ELEMENT (CMD_EXP_Init, SECINIT_STAGE_START, SECINIT_ORDER_LAST),
    _SEC_INIT_ELEMENT (CMD_CFG_Init, SECINIT_STAGE_START, SECINIT_ORDER_LAST),
    _SEC_INIT_ELEMENT (PLUGCT_Init,  SECINIT_STAGE_NORMAL,   SECINIT_ORDER_NORMAL),

    _SEC_INIT_ELEMENT (CMD_MNG_CmdRestoreSysCmd, SECINIT_STAGE_CMD_RESTORE, SECINIT_ORDER_NORMAL),
};


BS_STATUS SECINIT_Init()
{
    _SEC_INIT_CTRL_S  *pstSecInitNodeCurrent;

    for (pstSecInitNodeCurrent = &g_stBsInitTable[0];
            (ULONG)pstSecInitNodeCurrent < (((ULONG)&g_stBsInitTable[0]) + sizeof(g_stBsInitTable));
            pstSecInitNodeCurrent++)
    {
        pstSecInitNodeCurrent->pfFunc();
    }


    return BS_OK;
}


