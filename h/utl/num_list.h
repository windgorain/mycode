/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-8-31
* Description: 
* History:     
******************************************************************************/

#ifndef __NUM_LIST_H_
#define __NUM_LIST_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    DLL_NODE_S stLinkNode;
    INT64 iNumBegin;
    INT64 iNumEnd;
}NUM_LIST_NODE_S;

typedef struct {
    DLL_HEAD_S stList;
}NUM_LIST_S;

/* 解析: min-max格式 */
int NumList_ParseElement(LSTR_S *lstr, OUT UINT *min, OUT UINT *max);

VOID NumList_Init(IN NUM_LIST_S *pstList);
VOID NumList_Finit(IN NUM_LIST_S *pstList);
BS_STATUS NumList_AddRange(IN NUM_LIST_S *pstList, INT64 iBegin, INT64 iEnd);
/* 删除一个Range, 只查找完全匹配的第一个节点删除 */
BS_STATUS NumList_DelRange(IN NUM_LIST_S *pstList, INT64 iBegin, INT64 iEnd);
void NumList_DelNode(IN NUM_LIST_S *pstList, IN NUM_LIST_NODE_S *node);
NUM_LIST_NODE_S * NumList_FindRange(IN NUM_LIST_S *pstList, INT64 begin, INT64 end);
BS_STATUS NumList_ParseLstr(IN NUM_LIST_S *pstList, IN LSTR_S *pstNumListString);
BS_STATUS NumList_ParseStr(IN NUM_LIST_S *pstList, IN char *str);
/* 将两个List串成一个 */
VOID NumList_Cat(INOUT NUM_LIST_S *pstListDst, INOUT NUM_LIST_S *pstListSrc);
/* 对数字进行排序 */
VOID NumList_Sort(IN NUM_LIST_S *pstList);
/* 将有重叠部分消除 */
VOID NumList_Compress(IN NUM_LIST_S *pstList);
/* 将可以连续的部分进行连续化 */
VOID NumList_Continue(IN NUM_LIST_S *pstList);
/* 判断一个数字是否在List 内 */
BOOL_T NumList_IsNumInTheList(IN NUM_LIST_S *pstList, IN INT iNum);

#define NUM_LIST_SCAN_BEGIN(_pstList, _iBegin, _iEnd)   do { \
    NUM_LIST_NODE_S *_pstNode;    \
    DLL_SCAN(&(_pstList)->stList, _pstNode)  \
    {   \
        (_iBegin) = _pstNode->iNumBegin; \
        (_iEnd) = _pstNode->iNumEnd;

#define NUM_LIST_SCAN_END()   }}while(0)

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__NUM_LIST_H_*/


