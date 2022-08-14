/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-10
* Description: 为了和windows保持一致,模拟windows的DllMain函数
* History:     
******************************************************************************/
/* retcode所需要的宏 */
    
#include "bs.h"
#include "utl/plug_utl.h"

#ifdef IN_UNIXLIKE

#define DLL_PROCESS_ATTACH 0 /*\!< loading library (before Get*Version) */
#define DLL_THREAD_ATTACH  1 /*\!< attaching a thread */
#define DLL_THREAD_DETACH  2 /*\!< detaching from a thread */
#define DLL_PROCESS_DETACH 3 /*\!< about to unload (after Terminate*) */

/* Linux 上加载动态链接库 */
PLUG_ID PLUG_LoadLib(IN CHAR *pszFilePath)
{
    PF_PLUG_Entry pfDllMainFunc = NULL;
    PLUG_ID hPlugId;

    hPlugId = dlopen(pszFilePath, RTLD_NOW);
    if (0 == hPlugId) {
        return 0;
    }

    /* 为了和windows保持一致, 加载动态链接库成功后, 调用windows默认链接库函数 */
    pfDllMainFunc = PLUG_GET_FUNC_BY_NAME(hPlugId, "PLUG_Entry");
    if (NULL != pfDllMainFunc) {
        pfDllMainFunc(hPlugId, DLL_PROCESS_ATTACH, NULL);
    }

    return hPlugId;
}

/* Linux 上卸载动态链接库 */
void PLUG_UnloadLib(PLUG_ID plug)
{
    PF_PLUG_Entry pfDllMainFunc = NULL;

    /* 为了和windows保持一致, 加载动态链接库成功后, 调用windows默认链接库函数 */
    pfDllMainFunc = PLUG_GET_FUNC_BY_NAME(plug, "PLUG_Entry");
    if (NULL != pfDllMainFunc) {
        pfDllMainFunc(plug, DLL_PROCESS_DETACH, NULL);
    }

    dlclose(plug);
}

#endif

