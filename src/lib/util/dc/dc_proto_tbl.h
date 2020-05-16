/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-11-27
* Description: 
* History:     
******************************************************************************/

#ifndef __DC_PROTO_TBL_H_
#define __DC_PROTO_TBL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE (*PF_DC_OPEN_INSTACE_FUNC)(IN VOID *pParam);

typedef VOID (*PF_DC_CLOSE_INSTACE_FUNC)(IN HANDLE hHandle);

typedef BS_STATUS (*PF_DC_ADD_TBL_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName
);

typedef VOID (*PF_DC_DEL_TBL_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName
);

typedef BS_STATUS (*PF_DC_ADD_OBJECT_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
);

typedef BOOL_T (*PF_DC_IS_OBJECT_EXIST_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
);

typedef VOID (*PF_DC_DEL_OBJECT_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey
);

typedef BS_STATUS (*PF_DC_SET_FIELD_VALUE_AS_UINT_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    IN UINT uiValue
);

typedef BS_STATUS (*PF_DC_SET_FIELD_VALUE_AS_STRING_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    IN CHAR *pcValue
);

typedef BS_STATUS (*PF_DC_GET_FIELD_VALUE_AS_UINT_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT UINT *puiValue
);

typedef BS_STATUS (*PF_DC_CPY_FIELD_VALUE_AS_STRING_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT CHAR *pcValue,
    IN UINT uiValueMaxSize
);

typedef UINT (*PF_DC_GET_OBJECT_NUM_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName
);

typedef VOID (*PF_DC_WALK_TABLE_FUNC)
(
    IN HANDLE hHandle,
    IN PF_DC_WALK_TBL_CB_FUNC pfFunc,
    IN HANDLE hUserHandle
);

typedef VOID (*PF_DC_WALK_OBJECT_FUNC)
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN PF_DC_WALK_OBJECT_CB_FUNC pfFunc,
    IN HANDLE hUserHandle
);

typedef BS_STATUS (*PF_DC_SAVE_FUNC)(IN HANDLE hHandle);

typedef struct
{
    PF_DC_OPEN_INSTACE_FUNC pfOpenInstanceFunc;
    PF_DC_CLOSE_INSTACE_FUNC pfCloseInstanceFunc;
    PF_DC_ADD_TBL_FUNC pfAddTblFunc;
    PF_DC_DEL_TBL_FUNC pfDelTblFunc;
    PF_DC_ADD_OBJECT_FUNC pfAddObjectFunc;
    PF_DC_IS_OBJECT_EXIST_FUNC pfIsObjectExistFunc;
    PF_DC_DEL_OBJECT_FUNC pfDelObjectFunc;
    PF_DC_GET_FIELD_VALUE_AS_UINT_FUNC pfGetFieldValueAsUintFunc;
    PF_DC_CPY_FIELD_VALUE_AS_STRING_FUNC pfCpyFieldValueAsStringFunc;
    PF_DC_SET_FIELD_VALUE_AS_UINT_FUNC pfSetFieldValueAsUintFunc;
    PF_DC_SET_FIELD_VALUE_AS_STRING_FUNC pfSetFieldValueAsStringFunc;
    PF_DC_GET_OBJECT_NUM_FUNC  pfGetObjectNumFunc;
    PF_DC_WALK_TABLE_FUNC pfWalkTableFunc;
    PF_DC_WALK_OBJECT_FUNC pfWalkObjectFunc;
    PF_DC_SAVE_FUNC pfSaveFunc;
}DC_PROTO_TBL_S;

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__DC_PROTO_TBL_H_*/


