/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-9-10
* Description: 
* History:     
******************************************************************************/

#ifndef __STR_CONVERSION_H_
#define __STR_CONVERSION_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


/* 返回值      : 
    1. uiSize==0时,返回需要的字节数
    2. uiSize!=0时,返回0(失败)或者填充的字节数(包含结束符)
/************************************************************************/
UINT STR_UnicodeToAnsi(IN VOID * pUnicode, OUT CHAR * pcStr, IN UINT uiSize);

/* 返回值      : 
    1. uiSize==0时,返回需要的字节数
    2. uiSize!=0时,返回0(失败)或者填充的字节数(包含结束符)
/************************************************************************/
UINT STR_AnsiToUnicode(IN CHAR *pcStr, OUT VOID *pUnicode, IN UINT uiSize);

/* 返回值      : 
    1. uiSize==0时,返回需要的字节数
    2. uiSize!=0时,返回0(失败)或者填充的字节数(包含结束符)
/************************************************************************/
UINT STR_UnicodeToUtf8(IN VOID *pUnicode, OUT VOID *pUtf8, IN UINT uiSize);

/* 返回值      : 
    1. uiSize==0时,返回需要的字节数
    2. uiSize!=0时,返回0(失败)或者填充的字节数(包含结束符)
/************************************************************************/
UINT STR_Utf8ToUnicode(IN CHAR *pUtf8, OUT VOID *pUnicode, IN UINT uiSize);

/* 返回值 :
    1. uiSize==0时,返回需要的字节数
    2. uiSize!=0时,返回0(失败)或者填充的字节数(包含结束符)
*/
UINT STR_AnsiToUtf8(IN CHAR *pcStr, OUT VOID *pUtf8, IN UINT uiSize);

/* 返回值 :
    1. uiSize==0时,返回需要的字节数
    2. uiSize!=0时,返回0(失败)或者填充的字节数(包含结束符)
*/
UINT STR_Utf8ToAnsi(IN VOID *pUtf8, OUT CHAR * pcStr, IN UINT uiSize);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__STR_CONVERSION_H_*/


