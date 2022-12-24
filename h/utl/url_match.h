/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-8
* Description: 
* History:     
******************************************************************************/

#ifndef __URL_DELIVER_H_
#define __URL_DELIVER_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE URL_MATCH_HANDLE;

typedef struct
{
    DLL_NODE_S stLinkNode;
    CHAR *pcKey;
    UINT uiKeyLen;
    UINT uiFlag;
    USER_HANDLE_S stUserHandle;
}URL_MATCH_NODE_S;

URL_MATCH_HANDLE URL_Match_Create();
BS_STATUS URL_Match_RegMethod
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcMethod,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
);
BS_STATUS URL_Match_RegFile
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcFile,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
);
BS_STATUS URL_Match_RegPath
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcPath,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
);
BS_STATUS URL_Match_RegExternName
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcExternName,
    IN UINT uiFlag,
    IN USER_HANDLE_S *pstUserHandle
);
URL_MATCH_NODE_S * URL_Match_Match
(
    IN URL_MATCH_HANDLE hUD,
    IN CHAR *pcMethod,
    IN CHAR *pcRequestFile
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__URL_DELIVER_H_*/


