/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-4-24
* Description: 
* History:     
******************************************************************************/

#ifndef __KV_UTL_H_
#define __KV_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define KV_FLAG_CASE_SENSITIVE  0x1  /* 大小写敏感 */
#define KV_FLAG_PERMIT_MUL_KEY  0x2  /* 允许Key重复 */
#define KV_FLAG_SORT_KEY        0x4  /* 根据Key进行排序 */

typedef HANDLE KV_HANDLE;

typedef struct
{
    CHAR *pcKey;
    CHAR *pcValue;
}KV_S;

typedef struct {
    void *memcap;
    UINT uiFlag; /* KV_FLAG_XXX */
}KV_PARAM_S;

typedef CHAR* (*PF_KV_DECODE_FUNC)(IN LSTR_S *pstLstr, void *memcap);
typedef BS_WALK_RET_E (*PF_KV_WALK_FUNC)(IN CHAR *pcKey, IN CHAR *pcValue, IN HANDLE hUserHandle);

KV_HANDLE KV_Create(KV_PARAM_S *p);
VOID KV_Destory(IN KV_HANDLE hKvHandle);
void KV_Reset(KV_HANDLE hKvHandle);
/* 设置解码函数 */
VOID KV_SetDecode(IN KV_HANDLE hKvHandle, IN PF_KV_DECODE_FUNC pfDecode);
BS_STATUS KV_Parse(IN KV_HANDLE hKvHandle, IN LSTR_S *pstLstr, IN CHAR cSeparator, IN CHAR cEquelChar);
BS_STATUS KV_ParseOne(IN KV_HANDLE hKvHandle, IN LSTR_S *pstLstr, IN CHAR cSeparator);
BS_STATUS KV_Build
(
    IN KV_HANDLE hKvHandle,
    IN CHAR cSeparator,
    IN CHAR cEquelChar,
    IN UINT uiBufSize,
    OUT CHAR *pcBuf
);
CHAR * KV_GetKeyValue(IN KV_HANDLE hKvHandle, IN CHAR *pcKey);
BS_STATUS KV_SetKeyValue(IN KV_HANDLE hKvHandle, IN CHAR *pcKey, IN CHAR *pcValue/* 为NULL表示删除此节点 */);
KV_S * KV_GetNext(IN KV_HANDLE hKvHandle, IN KV_S *pstCurrent);
VOID KV_DelKey(IN KV_HANDLE hKvHandle, IN CHAR *pcKey);
VOID KV_Walk(IN KV_HANDLE hKvHandle, IN PF_KV_WALK_FUNC pfFunc, IN HANDLE hUserHandle);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__KV_UTL_H_*/


