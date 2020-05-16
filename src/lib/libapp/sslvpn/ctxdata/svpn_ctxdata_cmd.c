/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-8-26
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"

BS_STATUS SVPN_CD_EnterView
(
    IN VOID *pEnv,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcName
)
{
    SVPN_CONTEXT_HANDLE hSvpnContext;

    hSvpnContext = SVPN_Context_GetByEnv(pEnv, 0);
    if (NULL == hSvpnContext)
    {
        EXEC_OutString("Can't get context");
        return BS_ERR;
    }

    if (! SVPN_CtxData_IsObjectExist(hSvpnContext, enDataIndex, pcName))
    {
        if (BS_OK != SVPN_CtxData_AddObject(hSvpnContext, enDataIndex, pcName))
        {
            EXEC_OutString("Can't create.\r\n");
            return BS_ERR;
        }
    }

    return BS_OK;
}

BS_STATUS SVPN_CD_SetProp
(
    IN VOID *pEnv,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcProp,
    IN CHAR *pcPropValue
)
{
    SVPN_CONTEXT_HANDLE hSvpnContext;
    CHAR *pcName;
    
    hSvpnContext = SVPN_Context_GetByEnv(pEnv, 1);
    if (hSvpnContext == NULL)
    {
        EXEC_OutString("Can't get context");
        return BS_ERR;
    }

    pcName = CMD_EXP_GetCurrentModeValue(pEnv);
    if (NULL == pcName)
    {
        EXEC_OutString("Can't get name");
        return BS_ERR;
    }
    
    if (BS_OK != SVPN_CtxData_SetProp(hSvpnContext, enDataIndex, pcName, pcProp, pcPropValue))
    {
        EXEC_OutString("Failed");
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS SVPN_CD_AddPropElement
(
    IN VOID *pEnv,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcProp,
    IN CHAR *pcElement,
    IN CHAR cSplit
)
{
    SVPN_CONTEXT_HANDLE hSvpnContext;
    CHAR *pcName;

    if (strchr(pcElement, cSplit) != NULL)
    {
        EXEC_OutInfo("Can't inlucde \'%c\'", cSplit);
        return BS_ERR;
    }
    
    hSvpnContext = SVPN_Context_GetByEnv(pEnv, 1);
    if (hSvpnContext == NULL)
    {
        EXEC_OutString("Can't get context");
        return BS_ERR;
    }

    pcName = CMD_EXP_GetCurrentModeValue(pEnv);
    if (NULL == pcName)
    {
        EXEC_OutString("Can't get name");
        return BS_ERR;
    }

    if (BS_OK != SVPN_CtxData_AddPropElement(hSvpnContext, enDataIndex, pcName, pcProp, pcElement, cSplit))
    {
        EXEC_OutString("Failed");
        return BS_ERR;
    }

    return BS_OK;
}

VOID SVPN_CD_SaveProp
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcName,
    IN CHAR *pcProp,
    IN CHAR *pcCmdPrefix,
    IN HANDLE hFile
)
{
    HSTRING hString;
    CHAR *pcString;

    hString = SVPN_CtxData_GetPropAsHString(hSvpnContext, enDataIndex, pcName, pcProp);
    if (NULL == hString)
    {
        return;
    }

    pcString = STRING_GetBuf(hString);
    if ((pcString == NULL) || (pcString[0] == '\0'))
    {
        STRING_Delete(hString);
        return;
    }

    CMD_EXP_OutputCmd(hFile, "%s %s ", pcCmdPrefix, pcString);

    STRING_Delete(hString);

    return;
}

VOID SVPN_CD_SaveBoolProp
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcName,
    IN CHAR *pcProp,
    IN CHAR *pcCmdPrefix,
    IN HANDLE hFile
)
{
    HSTRING hString;
    CHAR *pcString;

    hString = SVPN_CtxData_GetPropAsHString(hSvpnContext, enDataIndex, pcName, pcProp);
    if (NULL == hString)
    {
        return;
    }

    pcString = STRING_GetBuf(hString);
    if ((pcString == NULL) || (pcString[0] == '\0'))
    {
        STRING_Delete(hString);
        return;
    }

    if (pcString[0] == 't')
    {
        CMD_EXP_OutputCmd(hFile, "%s", pcCmdPrefix);
    }

    STRING_Delete(hString);

    return;
}

VOID SVPN_CD_SaveElements
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcName,
    IN CHAR *pcProp,
    IN CHAR *pcCmdPrefix,
    IN CHAR cSplit,
    IN HANDLE hFile
)
{
    HSTRING hString;
    CHAR *pcElements;
    CHAR *pcEle;

    hString = SVPN_CtxData_GetPropAsHString(hSvpnContext, enDataIndex, pcName, pcProp);
    if (NULL == hString)
    {
        return;
    }

    pcElements = STRING_GetBuf(hString);
    if ((pcElements == NULL) || (pcElements[0] == '\0'))
    {
        STRING_Delete(hString);
        return;
    }

    TXT_SCAN_ELEMENT_BEGIN(pcElements, cSplit, pcEle)
    {
        CMD_EXP_OutputCmd(hFile, "%s %s", pcCmdPrefix, pcEle);
    }TXT_SCAN_ELEMENT_END();
    
    STRING_Delete(hString);

    return;
}


