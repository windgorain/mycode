/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-6
* Description: string with length. 使用长度描述字符串,而不是使用'\0'
* History:     
******************************************************************************/

#ifndef __LSTR_UTL_H_
#define __LSTR_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef int (*PF_LSTR_MULTI_KV_SCAN_CB)(IN LSTR_S *pstKey, IN LSTR_S *pstValue, IN VOID *pUserhandle);

typedef struct {
    LSTR_S key;
    LSTR_S value;
}LSTR_KV_S;

typedef struct {
    LSTR_KV_S kv;
    LSTR_S stLstr;
}LSTR_KV_ITER_S;

static inline VOID LSTR_Init(OUT LSTR_S *pstStr)
{
    pstStr->pcData = NULL;
    pstStr->uiLen  = 0;

    return;
}

VOID LSTR_Strim(IN LSTR_S *pstString, IN CHAR *pcSkipChars, OUT LSTR_S *pstStringOut);
CHAR * LSTR_Strchr(IN LSTR_S *pstLstr, IN CHAR cChr);
CHAR * LSTR_ReverseStrchr(LSTR_S *pstLstr, CHAR cChar);
CHAR *LSTR_Strstr(IN LSTR_S *pstLstr, IN CHAR *pcToFind);
/* 消除字符串头尾的引号 */
VOID LSTR_RemoveQuotation(INOUT LSTR_S *pstString);
/* 将字符串分隔成两个字符串 */
VOID LSTR_MSplit
(
    IN LSTR_S *pstString,
    IN CHAR *pcSplitChar, /* 分隔符集合 */
    OUT LSTR_S *pstStr1,
    OUT LSTR_S *pstStr2
);
/* 将字符串分隔成两个字符串 */
VOID LSTR_Split
(
    IN LSTR_S *pstString,
    IN CHAR cSplitChar, /* 分隔符 */
    OUT LSTR_S *pstStr1,
    OUT LSTR_S *pstStr2
);
/* 将字符串分隔成多个字符串, 返回分割成了多少个子串 */
UINT LSTR_XSplit
(
    IN LSTR_S *pstString,
    IN CHAR cSplitChar, /* 分隔符 */
    OUT LSTR_S *pstStr,/* 指向数组 */
    IN UINT uiCount /* 数组中元素个数 */
);
/* 从keyXvalue字符串中获取指定key的value */
BS_STATUS LSTR_GetValueByKey
(
    IN LSTR_S *pstString,
    IN CHAR cSplitChar,
    IN CHAR *pcKey,
    OUT LSTR_S *pstStrValue
);
/* 按照字符串比较 */
INT LSTR_StrCmp(IN LSTR_S *pstStr, IN CHAR *pcString);
/*str结构按照字符串进行不区分大小写比较 */
INT LSTR_StrCaseCmp(IN LSTR_S *pstStr, IN CHAR *pcString);
/* 按照字符串比较 */
INT LSTR_Cmp(IN LSTR_S *pstStr, IN LSTR_S *pstStr2);
/*str结构按照字符串进行不区分大小写比较 */
INT LSTR_CaseCmp(IN LSTR_S *pstStr, IN LSTR_S *pstStr2);
/* str结构复制到buff中并转为字符串 */
CHAR *LSTR_Strlcpy(IN LSTR_S *pstStr, IN UINT uiSzSize, OUT CHAR *pcSz);
VOID LSTR_Cat(IN LSTR_S *pstStr1, IN LSTR_S *pstStr2);
BS_STATUS LSTR_Atoui(IN LSTR_S *pstStr, OUT UINT *puiNum);
UINT LSTR_A2ui(IN LSTR_S *pstStr);
BS_STATUS LSTR_XAtoui(IN LSTR_S *pstStr, OUT UINT *puiNum);
/* 获取扩展名 */
BS_STATUS LSTR_GetExt(IN LSTR_S *pstFilePath, OUT LSTR_S *pstExt);

VOID LSTR_Lstr2Str(IN LSTR_S *pstLstr, OUT CHAR *pcStr, IN UINT uiStrSize);
/* 将多行压缩成一行 */
VOID LSTR_CompressLine(INOUT LSTR_S *pstLstr);

/* 删除指定的Elemeent. 比如删除"exp1,exp2,exp3"中的exp2 */
VOID LSTR_DelElement(INOUT LSTR_S *pstLstr, IN CHAR cSplitChar, IN LSTR_S *pstElement);
CHAR * LSTR_FindElement(INOUT LSTR_S *pstLstr, IN CHAR cSplitChar, IN LSTR_S *pstElement);
void LSTR_KvIterInit(IN LSTR_S *pstStr, OUT LSTR_KV_ITER_S *iter);
LSTR_KV_S * LSTR_GetNextKV(INOUT LSTR_KV_ITER_S *iter, IN char split, IN char eque);
INT LSTR_ScanMultiKV(IN LSTR_S *pstStr, IN char cSplit, IN char cEque, PF_LSTR_MULTI_KV_SCAN_CB pfCallBack, IN void *pUserHandle);

/* 遍历字符串中的每个元素. 元素是用cSplitChar隔开的字符串, pstElement是LSTR_S变量 */
#define LSTR_SCAN_ELEMENT_BEGIN(pszTxtBuf, uiBufLen, cSplitChar, pstElement)  \
    {    \
        CHAR *_pc = (pszTxtBuf);    \
        UINT _uiRemainLen = (uiBufLen); \
        while (_pc != NULL) {    \
            (pstElement)->pcData = _pc;   \
            _pc = TXT_Strnchr(_pc,cSplitChar,_uiRemainLen); \
            if (NULL != _pc) {(pstElement)->uiLen = _pc - (pstElement)->pcData; _pc++;_uiRemainLen-=(pstElement)->uiLen;_uiRemainLen--;} \
            else {(pstElement)->uiLen = _uiRemainLen;}

#define LSTR_SCAN_ELEMENT_END()  }}


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__LSTR_UTL_H_*/


