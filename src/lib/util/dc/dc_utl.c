/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-11-24
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_DCUTL

#include "bs.h"

#include "utl/dc_utl.h"
#include "utl/dc_xml.h"

#include "dc_proto_tbl.h"
#include "dc_mysql.h"

typedef struct
{
    DC_TYPE_E eDcType;
    HANDLE hFileHandle;
    USER_HANDLE_S stUserHandle;
}_DC_CTRL_S;

static BS_STATUS dc_NotSupport();
static BOOL_T dc_NotSupportBool();


static DC_PROTO_TBL_S g_stDcXmlProtoTbl = 
{
    DC_XML_OpenInstance,
    DC_XML_CloseInstance,
    DC_XML_AddTbl,
    DC_XML_DelTbl,
    DC_XML_AddObject,
    DC_XML_IsObjectExist,
    DC_XML_DelObject,
    DC_XML_GetFieldValueAsUint,
    DC_XML_CpyFieldValueAsString,
    DC_XML_SetFieldValueAsUint,
    DC_XML_SetFieldValueAsString,
    (PF_DC_GET_OBJECT_NUM_FUNC)dc_NotSupport,
    DC_XML_WalkTable,
    DC_XML_WalkTableObject,
    DC_XML_Save
};

static DC_PROTO_TBL_S g_stDcMysqlProtoTbl = 
{
    DC_Mysql_OpenInstance,
    DC_Mysql_CloseInstance,
    (PF_DC_ADD_TBL_FUNC)dc_NotSupport,
    (PF_DC_DEL_TBL_FUNC)dc_NotSupport,
    (PF_DC_ADD_OBJECT_FUNC)dc_NotSupport,
    (PF_DC_IS_OBJECT_EXIST_FUNC)dc_NotSupportBool,
    (PF_DC_DEL_OBJECT_FUNC)dc_NotSupport,
    DC_Mysql_GetFieldValueAsUint,
    DC_Mysql_CpyFieldValueAsString,
    (PF_DC_SET_FIELD_VALUE_AS_UINT_FUNC)dc_NotSupport,
    (PF_DC_SET_FIELD_VALUE_AS_STRING_FUNC)dc_NotSupport,
    (PF_DC_GET_OBJECT_NUM_FUNC)dc_NotSupport,
    (PF_DC_WALK_TABLE_FUNC)dc_NotSupport,
    (PF_DC_WALK_OBJECT_FUNC)dc_NotSupport,
    (PF_DC_SAVE_FUNC)dc_NotSupport
};

static DC_PROTO_TBL_S * g_apstDcProtoTbl[DC_TYPE_MAX] = 
{
    &g_stDcXmlProtoTbl,
    &g_stDcMysqlProtoTbl
};


static BS_STATUS dc_NotSupport()
{
    return BS_NOT_SUPPORT;
}

static BOOL_T dc_NotSupportBool()
{
    return FALSE;
}

static int dc_WalkObjectCb(IN DC_DATA_S *pstKey, IN HANDLE hUserHandle)
{
    USER_HANDLE_S *pstUserHandle = (USER_HANDLE_S *)hUserHandle;
    PF_DC_WALK_OBJECT_CB_FUNC pfFunc = (PF_DC_WALK_OBJECT_CB_FUNC)pstUserHandle->ahUserHandle[0];

    return pfFunc(pstKey, pstUserHandle->ahUserHandle[1]);
}

static int dc_WalkTableCb(IN CHAR *pcTable, IN HANDLE hUserHandle)
{
    USER_HANDLE_S *pstUserHandle = (USER_HANDLE_S *)hUserHandle;
    PF_DC_WALK_TBL_CB_FUNC pfFunc = (PF_DC_WALK_TBL_CB_FUNC)pstUserHandle->ahUserHandle[0];

    return pfFunc(pcTable, pstUserHandle->ahUserHandle[1]);
}

HANDLE DC_OpenInstance(IN DC_TYPE_E eDcType, IN VOID *pParam)
{
    _DC_CTRL_S *pstDcCtrl;

    if (eDcType >= DC_TYPE_MAX)
    {
        return 0;
    }

    pstDcCtrl = MEM_ZMalloc(sizeof(_DC_CTRL_S));
    if (NULL == pstDcCtrl)
    {
        return 0;
    }

    pstDcCtrl->eDcType = eDcType;
    
    pstDcCtrl->hFileHandle = g_apstDcProtoTbl[eDcType]->pfOpenInstanceFunc (pParam);
    if (NULL == pstDcCtrl->hFileHandle)
    {
        MEM_Free(pstDcCtrl);
        return 0;
    }

    return pstDcCtrl;
}

VOID DC_CloseInstance(IN HANDLE hDcHandle)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    g_apstDcProtoTbl[eDcType]->pfCloseInstanceFunc (pstDcCtrl->hFileHandle);

    MEM_Free(pstDcCtrl);
}

BS_STATUS  DC_AddTbl
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    return g_apstDcProtoTbl[eDcType]->pfAddTblFunc (pstDcCtrl->hFileHandle, pcTableName);
}

VOID DC_DelTbl
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    g_apstDcProtoTbl[eDcType]->pfDelTblFunc (pstDcCtrl->hFileHandle, pcTableName);
}

BS_STATUS DC_AddObject
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    return g_apstDcProtoTbl[eDcType]->pfAddObjectFunc (pstDcCtrl->hFileHandle, pcTableName, pstKey);
}

BOOL_T DC_IsObjectExist
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    return g_apstDcProtoTbl[eDcType]->pfIsObjectExistFunc(pstDcCtrl->hFileHandle, pcTableName, pstKey);
}

VOID DC_DelObject
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    g_apstDcProtoTbl[eDcType]->pfDelObjectFunc (pstDcCtrl->hFileHandle, pcTableName, pstKey);
}

BS_STATUS DC_GetFieldValueAsUint
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT UINT *puiValue
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    return g_apstDcProtoTbl[eDcType]->pfGetFieldValueAsUintFunc (pstDcCtrl->hFileHandle, pcTableName, pstKey, pcFieldName, puiValue);
}

BS_STATUS DC_CpyFieldValueAsString
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT CHAR *pcValue,
    IN UINT uiValueMaxSize
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    return g_apstDcProtoTbl[eDcType]->pfCpyFieldValueAsStringFunc (pstDcCtrl->hFileHandle,
        pcTableName, pstKey, pcFieldName, pcValue, uiValueMaxSize);
}

BS_STATUS DC_SetFieldValueAsUint
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    IN UINT uiValue
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    return g_apstDcProtoTbl[eDcType]->pfSetFieldValueAsUintFunc
        (pstDcCtrl->hFileHandle, pcTableName, pstKey, pcFieldName, uiValue);
}

BS_STATUS DC_SetFieldValueAsString
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    IN CHAR *pcValue
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    return g_apstDcProtoTbl[eDcType]->pfSetFieldValueAsStringFunc
        (pstDcCtrl->hFileHandle, pcTableName, pstKey, pcFieldName, pcValue);
}

UINT DC_GetObjectNum
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName
)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    return g_apstDcProtoTbl[eDcType]->pfGetObjectNumFunc (pstDcCtrl->hFileHandle, pcTableName);
}

VOID DC_WalkTable
(
    IN HANDLE hDcHandle,
    IN PF_DC_WALK_TBL_CB_FUNC pfWalkFunc,
    IN HANDLE hUserHandle
)
{
    USER_HANDLE_S stUserHandle;
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    stUserHandle.ahUserHandle[0] = pfWalkFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    g_apstDcProtoTbl[eDcType]->pfWalkTableFunc (pstDcCtrl->hFileHandle, dc_WalkTableCb, &stUserHandle);
}

VOID DC_WalkObject
(
    IN HANDLE hDcHandle,
    IN CHAR *pcTableName,
    IN PF_DC_WALK_OBJECT_CB_FUNC pfWalkFunc,
    IN HANDLE hUserHandle
)
{
    USER_HANDLE_S stUserHandle;
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    stUserHandle.ahUserHandle[0] = pfWalkFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    g_apstDcProtoTbl[eDcType]->pfWalkObjectFunc (pstDcCtrl->hFileHandle,
		     pcTableName, dc_WalkObjectCb, &stUserHandle);
}

CHAR * DC_GetKeyValueByName(IN DC_DATA_S *pstKey, IN CHAR *pcKeyName)
{
    UINT i;

    for (i=0; i<pstKey->uiNum; i++)
    {
        if (strcmp(pstKey->astKeyValue[i].pcKey, pcKeyName) == 0)
        {
            return pstKey->astKeyValue[i].pcValue;
        }
    }

    return NULL;
}

BS_STATUS DC_Save(IN HANDLE hDcHandle)
{
    _DC_CTRL_S *pstDcCtrl = (_DC_CTRL_S*)hDcHandle;
    DC_TYPE_E eDcType;

    BS_DBGASSERT(NULL != pstDcCtrl);

    eDcType = pstDcCtrl->eDcType;

    return g_apstDcProtoTbl[eDcType]->pfSaveFunc (pstDcCtrl->hFileHandle);
}


