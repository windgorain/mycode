/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-21
* Description: 
* History:     
******************************************************************************/

#ifndef __UPDATE_UTL_H_
#define __UPDATE_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    UPDATE_EVENT_START,
    UPDATE_EVENT_UPDATE_FILE,
    UPDATE_EVENT_UPDATE_FILE_RESULT,
    UPDATE_EVENT_END
}UPDATE_EVENT_E;

typedef struct
{
    UINT uiTotleUpdCount;
    UINT uiCurrentUpdCount;
    CHAR *pcUpdFile;
    UINT uiUpdFileFileSize;
    UINT uiUpdFileDownLoadSize;    
}UPDATE_FILE_S;

typedef enum
{
    UPDATE_FILE_RESULT_OK,          /* 更新完成 */
    UPDATE_FILE_RESULT_IS_NEWEST,   /* 已经是最新版本,无需更新 */
    UPDATE_FILE_RESULT_FAILED       /* 更新失败 */
}UPDATE_FILE_RESULT_E;

typedef enum
{
    UPDATE_RET_OK,
    UPDATE_RET_REBOOT,
    UPDATE_RET_ERR
}UPDATE_RET_E;

typedef VOID (*PF_UPDATE_NOTIFY_FUNC)(IN UPDATE_EVENT_E eEvent, IN VOID *pData, IN VOID *pUserHandle);

UPDATE_RET_E UPD_Update
(
    IN CHAR * pcUrl,
    IN CHAR *pcSaveFileName,
    IN PF_UPDATE_NOTIFY_FUNC pfUpdateNotify,
    IN VOID *pUserHandle
);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__UPDATE_UTL_H_*/


