#ifndef __DC_MYSQL_H_
#define __DC_MYSQL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

HANDLE DC_Mysql_OpenInstance(IN VOID *pParam);
VOID DC_Mysql_CloseInstance(IN HANDLE hHandle);
BS_STATUS DC_Mysql_GetFieldValueAsUint
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT UINT *puiValue
);
BS_STATUS DC_Mysql_CpyFieldValueAsString
(
    IN HANDLE hHandle,
    IN CHAR *pcTableName,
    IN DC_DATA_S *pstKey,
    IN CHAR *pcFieldName,
    OUT CHAR *pcValue,
    IN UINT uiValueMaxSize
);

#ifdef __cplusplus
    }
#endif 

#endif 

