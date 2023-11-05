/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-10-15
* Description: 
* History:     
******************************************************************************/

#ifndef __URL_LIB_H_
#define __URL_LIB_H_

#ifdef __cplusplus
    extern "C" {
#endif 

typedef enum
{
    URL_TYPE_ABSOLUTE,             
    URL_TYPE_ABSOLUTE_SIMPLE,      
    URL_TYPE_RELATIVE_ROOT,        
    URL_TYPE_RELATIVE_PAGE,        

    URL_TYPE_NOT_URL
}URL_LIB_URL_TYPE_E;

typedef struct {
    LSTR_S protocol;
    LSTR_S host;
    LSTR_S port;
    LSTR_S path;
    LSTR_S query;
}URL_FIELD_S;

URL_LIB_URL_TYPE_E URL_LIB_GetUrlType(IN CHAR *pcUrl, IN UINT uiUrlLen);


BS_STATUS ULR_LIB_ParseAbsUrl
(
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    OUT LSTR_S *pstProtocol,
    OUT LSTR_S *pstHost,
    OUT LSTR_S *pstPort,
    OUT LSTR_S *pstPath
);


BS_STATUS ULR_LIB_ParseSimpleAbsUrl
(
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    OUT LSTR_S *pstHost,
    OUT LSTR_S *pstPort,
    OUT LSTR_S *pstPath
);


UINT URL_LIB_GetUrlBackNum(IN CHAR *pcUrl, IN UINT uiUrlLen);

CHAR * URL_LIB_FullUrl2AbsPath(IN LSTR_S *pstFullUrl, OUT LSTR_S *pstAbsPath );


BS_STATUS URL_LIB_BuildUrl
(
    IN LSTR_S *pstProto,
    IN LSTR_S *pstHost,
    IN LSTR_S *pstPort,
    IN LSTR_S *pstPath,
    OUT CHAR *pcUrl
);


BS_STATUS URL_LIB_ParseUrl(IN CHAR *url, IN UINT url_len, OUT URL_FIELD_S *fields);

#ifdef __cplusplus
    }
#endif 

#endif 


