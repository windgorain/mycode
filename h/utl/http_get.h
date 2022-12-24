/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-20
* Description: 
* History:     
******************************************************************************/

#ifndef __HTTP_GET_H_
#define __HTTP_GET_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef VOID (*PF_HTTPGET_NOTIFY_FUNC)(IN CHAR *pszPath, IN CHAR *pszSaveAsFile, IN UINT ulFileSize, IN UINT ulDownLoadSize);


/* BS_ALREADY_EXIST:不用更新; BS_OK:更新成功; 其他:失败 */
BS_STATUS HTTPGET_GetFile
(
    IN CHAR *pszServer,
    IN USHORT usPort, /* 主机序 */
    IN CHAR *pszPath,
    IN time_t ulOldFileTime, /* 原来文件的时间. 如果服务器上的文件时间和这个不同,则下载 */
    IN CHAR *pszSaveAsFile  /* 如果为NULL, 则使用取到的文件名 */
);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__HTTP_GET_H_*/


