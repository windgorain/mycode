/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-5-13
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/cff_utl.h"


#define RETCODE_FILE_NUM RETCODE_FILE_NUM_SYSCFG


static CFF_HANDLE g_hSysCfgCff = 0;
static CHAR * g_pcSysconfigFileName = "config.ini";

static void syscfg_init()
{
    if (g_pcSysconfigFileName != NULL) {
        g_hSysCfgCff = CFF_INI_Open(g_pcSysconfigFileName, CFF_FLAG_READ_ONLY);
    }
}

CONSTRUCTOR(init) {
    syscfg_init();
}

CHAR * SYSCFG_GetConfigFileName()
{
    return g_pcSysconfigFileName;
}

HANDLE SYSCFG_GetCffHandle()
{
    return g_hSysCfgCff;
}

BS_STATUS SYSCFG_GetKeyValueAsUint(IN CHAR *pucMarkName, IN CHAR *pucKeyName, OUT UINT *pulKeyValue)
{
    if (g_hSysCfgCff == 0)
    {
        RETURN(BS_NO_SUCH);
    }

    return CFF_GetPropAsUint(g_hSysCfgCff, pucMarkName, pucKeyName, pulKeyValue);
}

BS_STATUS SYSCFG_GetKeyValueAsInt(IN CHAR *pucMarkName, IN CHAR *pucKeyName, OUT INT *plKeyValue)
{
    if (g_hSysCfgCff == 0)
    {
        RETURN(BS_NO_SUCH);
    }

    return CFF_GetPropAsInt(g_hSysCfgCff, pucMarkName, pucKeyName, plKeyValue);
}

BS_STATUS SYSCFG_GetKeyValueAsString(IN CHAR *pucMarkName, IN CHAR *pucKeyName,  OUT CHAR **ppucKeyValue)
{
    if (g_hSysCfgCff == 0)
    {
        RETURN(BS_NO_SUCH);
    }

    return CFF_GetPropAsString(g_hSysCfgCff, pucMarkName, pucKeyName, ppucKeyValue);
}

BS_STATUS SYSCFG_WalkKey(IN CHAR *pcMarkName, IN PF_SYSCFG_WALK_KEY pfFunc, IN VOID *pUserHandle)
{
    CHAR *pcKey;
    CHAR *pcValue;
    
    if (g_hSysCfgCff == 0)
    {
        RETURN(BS_NO_SUCH);
    }

    CFF_SCAN_PROP_START(g_hSysCfgCff, pcMarkName, pcKey, pcValue)
    {
        pfFunc(pcMarkName, pcKey, pcValue, pUserHandle);
    }CFF_SCAN_END();

    return BS_OK;
}

