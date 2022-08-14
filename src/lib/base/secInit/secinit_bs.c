/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-10
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_SECINIT

#include "bs.h"


#define _SEC_INIT_ELEMENT(pfFunc, eStage, eOrder)    {{0}, (BS_STATUS_FUNC)pfFunc, #pfFunc, eStage, eOrder}

typedef enum
{
    SECINIT_STAGE_INIT0 = 0,
    SECINIT_STAGE_INIT1,         /* 初始化自己，不能在此期间调用其他模块的函数，但可以调用SYSCFG函数 */
    SECINIT_STAGE_INIT2, 
    SECINIT_STAGE_START,         /* 可以调用部分其他模块的函数进行初始化操作 */
    SECINIT_STAGE_RUN,           /* 可以调用所有其他模块的函数进行初始化了 */
    SECINIT_STAGE_NORMAL,        /* 大多数非基础模块的初始化阶段 */
    SECINIT_STAGE_LOW,           /* 在正常的模块初始化之后 */
    SECINIT_STAGE_NO_SBUPROCESS, /* 从这个阶段开始, 子进程不再自动初始化,只有主进程继续执行初始化操作 */
    SECINIT_STAGE_CMD_RESTORE,   /* 配置恢复 */
    SECINIT_STAGE_LAST           /* 最后的初始化阶段 */
}SECINIT_STAGE_E;

/*在同一stage阶段的初始化顺序*/
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


