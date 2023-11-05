/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-8
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_TXT

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/num_utl.h"
#include "utl/rand_utl.h"
#include "utl/stack_utl.h"

#ifdef IN_WINDOWS



UINT STR_UnicodeToAnsi(IN VOID * pUnicode, OUT CHAR * pcStr, IN UINT uiSize)
{  
    return WideCharToMultiByte(CP_ACP, 0, pUnicode, -1, pcStr, uiSize, NULL, FALSE);
}  


UINT STR_AnsiToUnicode(IN CHAR *pcStr, OUT VOID *pUnicode, IN UINT uiSize)  
{
    UINT uiLen;

    uiLen = MultiByteToWideChar (CP_ACP, 0, pcStr, -1, pUnicode, uiSize/sizeof(wchar_t));    

    return uiLen * sizeof(wchar_t);  
}


UINT STR_UnicodeToUtf8(IN VOID *pUnicode, OUT VOID *pUtf8, IN UINT uiSize)
{  
    return WideCharToMultiByte (CP_UTF8, 0, pUnicode, -1, pUtf8, uiSize, NULL,NULL);
}


UINT STR_Utf8ToUnicode(IN CHAR *pUtf8, OUT VOID *pUnicode, IN UINT uiSize)
{  
    UINT uiLen;
    
    uiLen = MultiByteToWideChar(CP_UTF8, 0, pUtf8, -1, pUnicode, uiSize/sizeof(wchar_t));

    return uiLen * sizeof(wchar_t); 
}


UINT STR_AnsiToUtf8(IN CHAR *pcStr, OUT VOID *pUtf8, IN UINT uiSize)
{
    UINT uiLen;
    VOID *pWideChar;
    UINT uiNeedLen;

    uiNeedLen = STR_AnsiToUnicode(pcStr, NULL, 0);
    if (uiNeedLen == 0)
    {
        return 0;
    }

    pWideChar = MEM_ZMalloc(uiNeedLen);
    if (NULL == pWideChar)
    {
        return 0;
    }

    STR_AnsiToUnicode(pcStr, pWideChar, uiNeedLen);    

    uiLen = STR_UnicodeToUtf8(pWideChar, pUtf8, uiSize);

    MEM_Free(pWideChar);

    return uiLen;
}


UINT STR_Utf8ToAnsi(IN VOID *pUtf8, OUT CHAR * pcStr, IN UINT uiSize)
{
    VOID *pWide;
    UINT uiLen;

    uiLen = STR_Utf8ToUnicode(pUtf8, NULL, 0);
    if (0 == uiLen)
    {
        return 0;
    }

    pWide = MEM_ZMalloc(sizeof(wchar_t) * uiLen);
    if (NULL == pWide)
    {
        return 0;
    }

    STR_Utf8ToUnicode(pUtf8, pWide, uiLen);

    uiLen = STR_UnicodeToAnsi(pWide, pcStr, uiSize);

    MEM_Free(pWide);

    return uiLen;
}

#endif



