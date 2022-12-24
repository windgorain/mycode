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
#endif /* __cplusplus */

typedef enum
{
    URL_TYPE_ABSOLUTE,             /* 绝对url, 比如 http://www.baidu.com/abc/xxx.htm  */
    URL_TYPE_ABSOLUTE_SIMPLE,      /* 绝对url, 比如 //www.baidu.com/abc/xxx.htm  */
    URL_TYPE_RELATIVE_ROOT,        /* 基于根路径的相对url数据, 比如  "/abc/xxx.htm" */
    URL_TYPE_RELATIVE_PAGE,        /* 基于当前页面的相对url数据, 比如"/abc/xxx.htm" */

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

/* 将绝对路径解析出protocol/host/port/path来 */
BS_STATUS ULR_LIB_ParseAbsUrl
(
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    OUT LSTR_S *pstProtocol,
    OUT LSTR_S *pstHost,
    OUT LSTR_S *pstPort,
    OUT LSTR_S *pstPath
);

/* 将Simple绝对路径(比如"//www.baidu.com/abc/index.htm")解析出 host/port/path 来 */
BS_STATUS ULR_LIB_ParseSimpleAbsUrl
(
    IN CHAR *pcUrl,
    IN UINT uiUrlLen,
    OUT LSTR_S *pstHost,
    OUT LSTR_S *pstPort,
    OUT LSTR_S *pstPath
);

/* 获取URL一开始的回退符"../"的个数 */
UINT URL_LIB_GetUrlBackNum(IN CHAR *pcUrl, IN UINT uiUrlLen);

CHAR * URL_LIB_FullUrl2AbsPath(IN LSTR_S *pstFullUrl, OUT LSTR_S *pstAbsPath /* 可以为NULL */);

/* 构造URL */
BS_STATUS URL_LIB_BuildUrl
(
    IN LSTR_S *pstProto,
    IN LSTR_S *pstHost,
    IN LSTR_S *pstPort,
    IN LSTR_S *pstPath,
    OUT CHAR *pcUrl
);

/* 解析出"http/tcp/unix等://xxx:port/path?query中的每个字段 */
BS_STATUS URL_LIB_ParseUrl(IN CHAR *url, IN UINT url_len, OUT URL_FIELD_S *fields);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__URL_LIB_H_*/


