/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-3-1
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_SUNDAY

#include "bs.h"

#include "utl/ss_utl.h"

VOID Sunday_ComplexPatt(IN UCHAR *pucPatt, IN UINT ulPattLen, OUT SUNDAY_SKIP_TABLE_S *pstSkipTb)
{
    UINT i;

    BS_DBGASSERT(NULL != pucPatt);
    BS_DBGASSERT(NULL != pstSkipTb);

    for (i=0; i<256; i++)
    {
        pstSkipTb->aulSundaySkipTable[i] = ulPattLen + 1;
    }

    for (i=0; i<ulPattLen; i++)
    {
        pstSkipTb->aulSundaySkipTable[pucPatt[i]] = ulPattLen - i;     /* 表示需要向后移动的字节数 */
    }
}

UCHAR * Sunday_SearchFast
(
    IN UCHAR *pucData,
    IN UINT ulDataLen,
    IN UCHAR *pucPatt,
    IN UINT ulPattLen,
    IN SUNDAY_SKIP_TABLE_S *pstSkipTb
)
{
    UCHAR *t;
    UCHAR *tx = pucData;
    UCHAR *p;
    UCHAR *pucDataEnd;
    UCHAR *pucPattEnd;

    BS_DBGASSERT(NULL != pucData);
    BS_DBGASSERT(NULL != pucPatt);

    if (ulDataLen < ulPattLen)
    {
        return NULL;
    }

    pucDataEnd = pucData + ulDataLen;
    pucPattEnd = pucPatt + ulPattLen;

    while (tx + ulPattLen <= pucDataEnd)
    {
        for (p = pucPatt, t = tx; p<pucPattEnd; ++p, ++t)
        {
            if (*p != *t)
            {
                break;
            }
        }
        
        if (p == pucPattEnd)   /* 找到了 */
        {
            return (UCHAR*)tx;
        }

        if (tx + ulPattLen == pucDataEnd)
        {
            break;
        }
        
        tx += pstSkipTb->aulSundaySkipTable[tx[ulPattLen]];
    }
    
    return NULL;
}

UCHAR *Sunday_Search(IN UCHAR *pucData, IN UINT ulDataLen, IN UCHAR *pucPatt, IN UINT ulPattLen)
{
    SUNDAY_SKIP_TABLE_S stSkipTb;

    Sunday_ComplexPatt(pucPatt, ulPattLen, &stSkipTb);
    return Sunday_SearchFast(pucData, ulDataLen, pucPatt, ulPattLen, &stSkipTb);
}

