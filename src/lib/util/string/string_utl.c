/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-15
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_STRING

#include "bs.h"

#include "utl/vbuf_utl.h"
#include "utl/string_utl.h"


HSTRING STRING_Create()
{
    VBUF_S *pstVBuf;

    pstVBuf = MEM_Malloc(sizeof(VBUF_S));
    if (NULL == pstVBuf) {
        return NULL;
    }

    VBUF_Init(pstVBuf);
    VBUF_SetMemDouble(pstVBuf, 1);

    /* 初始化成"\0"空字符串 */
    if (BS_OK != VBUF_CpyFromBuf(pstVBuf, "", 1)) {
        VBUF_Finit(pstVBuf);
        MEM_Free(pstVBuf);
        return NULL;
    }

    return pstVBuf;
}

VOID STRING_Delete(IN HSTRING hHandle)
{
    if (NULL == hHandle)
    {
        return;
    }
    
    VBUF_Finit(hHandle);

    MEM_Free(hHandle);
}

UINT STRING_GetLength(IN HSTRING hHandle)
{
    UINT ulLen;
    ulLen = VBUF_GetDataLength(hHandle);

    if (ulLen > 0)
    {
        ulLen--;
    }

    return ulLen;
}

BS_STATUS STRING_CatFromBuf(IN HSTRING hHandle, IN CHAR *pszDate)
{
    BS_DBGASSERT(NULL != pszDate);
    
    VBUF_CutTail(hHandle, 1);   /* 删除'\0' */
    return VBUF_CatFromBuf(hHandle, pszDate, strlen(pszDate) + 1);
}

BS_STATUS STRING_CatFromString(IN HSTRING hHandleDst, IN HSTRING hHandleSrc)
{
    VBUF_CutTail(hHandleDst, 1);   /* 删除'\0' */
    return VBUF_CatFromVBuf(hHandleDst, hHandleSrc);
}

BS_STATUS STRING_CpyFromBuf(IN HSTRING hHandle, IN CHAR *pszDate)
{
    BS_DBGASSERT(NULL != pszDate);
    
    return VBUF_CpyFromBuf(hHandle, (void*)pszDate, strlen(pszDate) + 1);
}

BS_STATUS STRING_CpyFromString(IN HSTRING hHandleDst, IN HSTRING hHandleSrc)
{
    return VBUF_CpyFromVBuf(hHandleDst, hHandleSrc);
}

INT STRING_CmpByBuf(IN HSTRING hHandle, IN CHAR *pszDate)
{
    BS_DBGASSERT(NULL != pszDate);

    return VBUF_CmpByBuf(hHandle, (void*)pszDate, strlen(pszDate) + 1);
}

INT STRING_CmpByString(IN HSTRING hHandle1, IN HSTRING hHandle2)
{
    return VBUF_CmpByVBuf(hHandle1, hHandle2);
}

CHAR * STRING_GetBuf(IN HSTRING hHandle)
{
    return VBUF_GetData(hHandle);
}

void STRING_Clear(HSTRING hHandle)
{
    VBUF_CutAll(hHandle);
}

void STRING_CutHead(HSTRING hHandle, int len)
{
    VBUF_CutHead(hHandle, len);
}


