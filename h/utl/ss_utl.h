/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-3-1
* Description: 
* History:     
******************************************************************************/

#ifndef __SS_UTL_H_
#define __SS_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* Sunday 查找算法 */

typedef struct
{
    UINT aulSundaySkipTable[256];
}SUNDAY_SKIP_TABLE_S;

/* 生成模式字符串的跳转表 */
VOID Sunday_ComplexPatt(IN UCHAR *pucPatt, IN UINT ulPattLen, OUT SUNDAY_SKIP_TABLE_S *pstSkipTb);

/* 查找子串, 前提是已经调用Sunday_ComplexPatt生成了跳转表, 用于对一个模式多次查找的情况 */
UCHAR * Sunday_SearchFast(IN UCHAR *pucData, IN UINT ulDataLen, IN UCHAR *pucPatt, IN UINT ulPattLen, IN SUNDAY_SKIP_TABLE_S *pstSkipTb);

/* 查找子串, 其实是 Sunday_ComplexPatt 和Sunday_SearchFast的调用 */
UCHAR *Sunday_Search(IN UCHAR *pucData, IN UINT ulDataLen, IN UCHAR *pucPatt, IN UINT ulPattLen);



#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SS_UTL_H_*/


