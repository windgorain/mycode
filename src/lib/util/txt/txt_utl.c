/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/num_utl.h"
#include "utl/rand_utl.h"
#include "utl/stack_utl.h"
#include "utl/ctype_utl.h"

BS_STATUS TXT_Lower(INOUT CHAR *pucTxtBuf)
{
    CHAR *pucTmp = pucTxtBuf;

    if (pucTxtBuf == NULL)
    {
        RETURN(BS_NULL_PARA);
    }

    while (*pucTmp != '\0')
    {
        if (NUM_IN_RANGE(*pucTmp, 'A', 'Z'))
        {
            *pucTmp += ('a' - 'A');
        }
        pucTmp++;
    }

    return BS_OK;
}

BS_STATUS TXT_Upper(INOUT CHAR *pucTxtBuf)
{
    CHAR *pucTmp = pucTxtBuf;

    if (pucTxtBuf == NULL)
    {
        RETURN(BS_NULL_PARA);
    }

    while (*pucTmp != '\0')
    {
        if (NUM_IN_RANGE(*pucTmp, 'a', 'z'))
        {
            *pucTmp -= ('a' - 'A');
        }
        pucTmp++;
    }

    return BS_OK;
}

static BS_STATUS txt_DelSubStr(IN CHAR *pucTxtBuf, IN CHAR *pucSubStr, OUT CHAR *pucTxtOutBuf, IN ULONG ulSize)
{
    CHAR *pcFind;
    CHAR *pcTxtBufTmp;
    CHAR *pcTxtOutBufTmp;
    ULONG ulSizeTmp = ulSize;
    UINT ulCopyLen;

    if (strlen(pucTxtBuf) == 0)
    {
        pucTxtOutBuf[0] = '\0';
        return BS_OK;
    }

    if (strlen(pucSubStr) == 0)
    {
        TXT_Strlcpy(pucTxtOutBuf, pucTxtBuf, ulSize);
        return BS_OK;
    }

    pucTxtOutBuf[ulSize - 1] = '\0';

    pcTxtBufTmp = pucTxtBuf;
    pcTxtOutBufTmp = pucTxtOutBuf;

    pcFind = strstr(pcTxtBufTmp, pucSubStr);
    if (pcFind == NULL)
    {
        TXT_Strlcpy(pcTxtOutBufTmp, pcTxtBufTmp, ulSize);
        return BS_OK;
    }

    while ((pcTxtBufTmp[0] != '\0') && (ulSizeTmp > 1))
    {
        pcFind = strstr(pcTxtBufTmp, pucSubStr);
        if (pcFind == NULL)
        {
            TXT_Strlcpy(pcTxtOutBufTmp, pcTxtBufTmp, ulSizeTmp);
            break;
        }
        else
        {
            if (pcFind != pcTxtBufTmp)
            {
                ulCopyLen = MIN((ULONG)pcFind - (ULONG)pcTxtBufTmp, ulSizeTmp - 1);
                strlcpy(pcTxtOutBufTmp, pcTxtBufTmp, ulCopyLen + 1);
                ulSizeTmp -= ulCopyLen;
                pcTxtOutBufTmp += ulCopyLen;
            }
            pcTxtBufTmp = pcFind + strlen(pucSubStr);
        }
    }

    return txt_DelSubStr(pucTxtOutBuf, pucSubStr, pucTxtOutBuf, ulSize);
}

VOID TXT_DelSubStr(IN CHAR *pucTxtBuf, IN CHAR *pucSubStr, OUT CHAR *pucTxtOutBuf, IN ULONG ulSize)
{
    BS_DBGASSERT(NULL != pucTxtBuf);
    BS_DBGASSERT(NULL != pucSubStr);
    BS_DBGASSERT(NULL != pucTxtOutBuf);

    if (ulSize == 0)
    {
        return;
    }

    txt_DelSubStr(pucTxtBuf, pucSubStr, pucTxtOutBuf, ulSize);
}

static BS_STATUS txt_DelLineComment
(
    IN CHAR *pcInBuf,
    IN CHAR *pcCommentFlag,  /* 注释标记, 如"#", "//" */
    OUT CHAR *pcOutBuf
)
{
    CHAR *pcFind;
    CHAR *read;
    CHAR *write;
    UINT ulCopyLen;

    if (strlen(pcInBuf) == 0) {
        pcOutBuf[0] = '\0';
        return BS_OK;
    }

    if (strlen(pcCommentFlag) == 0) {
        if (pcInBuf != pcOutBuf) {
            strlcpy(pcOutBuf, pcInBuf, strlen(pcInBuf) + 1);
        }
        return BS_OK;
    }

    read = pcInBuf;
    write = pcOutBuf;

    while (read[0] != '\0') {
        pcFind = strstr(read, pcCommentFlag);
        if (pcFind == NULL) {
            if (read != write) {
                strlcpy(write, read, strlen(read) + 1);
            }
            break;
        } else {
            if (pcFind != read) {
                /* Copy之前的内容 */
                ulCopyLen = (ULONG)pcFind - (ULONG)read;
                if (read != write) {
                    strlcpy(write, read, ulCopyLen + 1);
                }
                write += ulCopyLen;
            }

            read = pcFind + strlen(pcCommentFlag);

            /* 查找行尾 */
            pcFind = TXT_MStrnchr(read, strlen(read), (void*)"\r\n");
            if (NULL == pcFind) {
                break;
            }

            read = pcFind + 1;
            if ((*read == '\r') || (*read == '\n')) {
                read ++;
            }
        }
    }

    return BS_OK;
}

/* 去除行注释 */
VOID TXT_DelLineComment
(
    IN CHAR *pcInBuf,
    IN CHAR *pcCommentFlag,  /* 注释标记, 如"#", "//" */
    OUT CHAR *pcOutBuf
)
{
    BS_DBGASSERT(NULL != pcInBuf);
    BS_DBGASSERT(NULL != pcCommentFlag);
    BS_DBGASSERT(NULL != pcOutBuf);

    txt_DelLineComment(pcInBuf, pcCommentFlag, pcOutBuf);
}

static BS_STATUS txt_ReplaceSubStr
(
    IN CHAR *pucTxtBuf,
    IN CHAR *pucSubStrFrom,
    IN CHAR *pucSubStrTo,
    OUT CHAR *pucTxtOutBuf,
    IN ULONG ulSize
)
{
    CHAR *pcFind;
    CHAR *pcTxtBufTmp;
    CHAR *pcTxtOutBufTmp;
    UINT ulSubStrToLen;
    ULONG ulSizeTmp = ulSize;
    UINT uiCopyLen;

    if (strlen(pucTxtBuf) == 0)
    {
        *pucTxtOutBuf = '\0';
        return BS_OK;
    }

    if (strlen(pucSubStrFrom) == 0)
    {
        TXT_Strlcpy(pucTxtOutBuf, pucTxtBuf, ulSize);
        return BS_OK;
    }

    pcTxtBufTmp = pucTxtBuf;
    pcTxtOutBufTmp = pucTxtOutBuf;
    ulSubStrToLen = strlen(pucSubStrTo);

    /* 判断是否需要替换 */
    pcFind = strstr(pcTxtBufTmp, pucSubStrFrom);
    if (pcFind == NULL)
    {
        TXT_Strlcpy(pcTxtOutBufTmp, pcTxtBufTmp, ulSize);
        return BS_OK;
    }

    /* 进行递归替换 */
    while ((pcTxtBufTmp[0] != '\0') && (ulSizeTmp > 1))
    {
        pcFind = strstr(pcTxtBufTmp, pucSubStrFrom);
        if (pcFind == NULL)
        {
            TXT_Strlcpy(pcTxtOutBufTmp, pcTxtBufTmp, ulSizeTmp);
            break;
        }
        else
        {
            if (pcFind != pcTxtBufTmp)
            {
                uiCopyLen = MIN(ulSizeTmp - 1, (ULONG)pcFind - (ULONG)pcTxtBufTmp);

                strlcpy(pcTxtOutBufTmp, pcTxtBufTmp, uiCopyLen + 1);
                ulSizeTmp -= uiCopyLen;
                pcTxtOutBufTmp += uiCopyLen;
                pcTxtOutBufTmp[0] = '\0';

                uiCopyLen = MIN(ulSizeTmp - 1, ulSubStrToLen);
                strlcpy(pcTxtOutBufTmp, pucSubStrTo, uiCopyLen + 1);
                ulSizeTmp -= uiCopyLen;
                pcTxtOutBufTmp += ulSubStrToLen;
            }
            pcTxtBufTmp = pcFind + strlen(pucSubStrFrom);
        }
    }

    return txt_ReplaceSubStr(pucTxtOutBuf, pucSubStrFrom, pucSubStrTo, pucTxtOutBuf, ulSize);
}

void TXT_ReplaceChar(INOUT char *pcTxtBuf, char from, char to)
{
    char *c = pcTxtBuf;
    while (*c) {
        if (*c == from) {
            *c = to;
        }
        c++;
    }
}

VOID TXT_ReplaceSubStr(IN CHAR *pcTxtBuf, IN CHAR *pcSubStrFrom, IN CHAR *pcSubStrTo, OUT CHAR *pcTxtOutBuf, IN ULONG ulSize)
{
    BS_DBGASSERT(NULL != pcTxtBuf);
    BS_DBGASSERT(NULL != pcSubStrFrom);
    BS_DBGASSERT(NULL != pcSubStrTo);
    BS_DBGASSERT(NULL != pcTxtOutBuf);

    if (ulSize == 0)
    {
        return;
    }

    txt_ReplaceSubStr(pcTxtBuf, pcSubStrFrom, pcSubStrTo, pcTxtOutBuf, ulSize);
}

VOID TXT_ReplaceSubStrOnce
(
    IN CHAR *pcTxtBuf,
    IN CHAR *pcSubStrFrom,
    IN CHAR *pcSubStrTo,
    OUT CHAR *pcTxtOutBuf,
    IN ULONG ulOutSize
)
{
    CHAR *pcFind;

    BS_DBGASSERT(NULL != pcTxtBuf);
    BS_DBGASSERT(NULL != pcSubStrFrom);
    BS_DBGASSERT(NULL != pcSubStrTo);
    BS_DBGASSERT(NULL != pcTxtOutBuf);

    if (strlen(pcTxtBuf) == 0)
    {
        *pcTxtOutBuf = '\0';
        return;
    }

    if (strlen(pcSubStrFrom) == 0)
    {
        TXT_Strlcpy(pcTxtOutBuf, pcTxtBuf, ulOutSize);
        return;
    }

    /* 判断是否需要替换 */
    pcFind = strstr(pcTxtBuf, pcSubStrFrom);
    if (pcFind == NULL)
    {
        TXT_Strlcpy(pcTxtOutBuf, pcTxtBuf, ulOutSize);
        return;
    }

    memcpy(pcTxtOutBuf, pcTxtBuf, pcFind - pcTxtBuf);
    TXT_Strlcat(pcTxtOutBuf, pcFind + strlen(pcSubStrFrom), ulOutSize);
}

char * TXT_CompressLine(INOUT char * pcString)
{
    LSTR_S stStr;

    stStr.pcData = pcString;
    stStr.uiLen = strlen(pcString);

    LSTR_CompressLine(&stStr);

    stStr.pcData[stStr.uiLen] = '\0';

    return stStr.pcData;
}

CHAR * TXT_StrimHead(IN CHAR *pcData, IN ULONG ulDataLen, IN CHAR *pcSkipChars)
{
    CHAR *pcTemp = pcData;
    CHAR *pcEnd = pcData + ulDataLen;

    /* 入参合法性检查 */
    if((NULL == pcData) || (NULL == pcSkipChars))
    {
        return NULL;
    }

    if (0 == ulDataLen)
    {
        return pcData;
    }

    while (pcTemp < pcEnd)
    {
        if(NULL == strchr(pcSkipChars, *pcTemp))
        {
            break;
        }
        pcTemp ++;
    }

    return pcTemp;
}

/* 返回新的长度 */
ULONG TXT_StrimTail(IN CHAR *pcData, IN ULONG ulDataLen, IN CHAR *pcSkipChars)
{
    CHAR *pcTemp;

    /* 入参合法性检查 */
    if ((NULL == pcData) || (NULL == pcSkipChars))
    {
        return 0;
    }

    if (0 == ulDataLen)
    {
        return 0;
    }

    pcTemp = (pcData + ulDataLen) - 1;

    while (pcTemp >= pcData)
    {
        if (NULL == strchr(pcSkipChars, *pcTemp))
        {
            break;
        }
        pcTemp --;
    }

    return ((ULONG)(pcTemp + 1) - (ULONG)pcData);
}

CHAR * TXT_StrimHeadTail(CHAR *pcData, ULONG ulDataLen, CHAR *pcSkipChars, OUT ULONG *pulNewLen)
{
    CHAR *head = TXT_StrimHead(pcData, ulDataLen, pcSkipChars);
    ULONG len = ulDataLen - (head - pcData);
    *pulNewLen = TXT_StrimTail(head, len, pcSkipChars);

    return head;
}

CHAR * TXT_StrimString(IN CHAR *pcData, IN CHAR *pcSkipChars)
{
    ULONG ulLen;

    if (NULL == pcData)
    {
        return NULL;
    }

    ulLen = TXT_StrimTail(pcData, strlen(pcData), pcSkipChars);
    pcData[ulLen] = '\0';

    return TXT_StrimHead(pcData, ulLen, pcSkipChars);
}

/* 删除字符串头和尾的空格\t \n \r 字符 */
CHAR *TXT_Strim(IN CHAR *pszStr)
{
    LSTR_S stString;

    stString.pcData = pszStr;
    stString.uiLen = strlen(pszStr);
    
    LSTR_Strim(&stString, (void*)" \t\n\r", &stString);
    stString.pcData[stString.uiLen] = '\0';

    return stString.pcData;
}

/* 删除字符串头和尾的空格\t \n \r 字符, 并移动数据到头 */
VOID TXT_StrimAndMove(IN CHAR *pszStr)
{
	CHAR *pt = pszStr;
	INT len, idx, b = 0;

	/* skip white spaces in the string */
	for (; *pt != '\0'; pt++ )
	{
		if (! TXT_IS_BLANK_OR_RN(*pt))
			break;
	}

    memmove(pszStr, pt, strlen(pt) + 1);

	len = strlen(pt);
	if (len)
	{
		for ( idx = (len - 1); idx >= 0; idx-- )
		{
			switch (pt[idx])
			{
				case ' ':
				case '\t':
				case '\r':
				case '\n':
					pt[idx] = '\0';
					break;
					
				default:
					b++;
					break;
			}
			
			if (b)
				break;
		}
	}
}

/* 删除字符串中的空格\t \n \r 字符, 包含头尾和中间的空白字符 */
char * TXT_StrimAll(IN CHAR *pcStr)
{
    CHAR *pcRead;
    CHAR *pcWrite;
    CHAR c;

    pcRead = pcStr;
    pcWrite = pcStr;

    while (*pcRead != '\0')
    {
        c = *pcRead;
        pcRead ++;

        if (TXT_IS_BLANK_OR_RN(c))  {
            continue;
        }

        *pcWrite = c;
        pcWrite ++;
    }

    *pcWrite = '\0';

    return pcStr;
}

/* 将不包含头尾空白字符的结果copy到dst */
char * TXT_StrimTo(char *in, char *out)
{
    in = TXT_StrimHead(in, strlen(in), (void*)" \t\r\n");
    strcpy(out, in);
    TXT_Strim(out);

    return out;
}

/* 将不包含空白字符的结果copy到dst */
char * TXT_StrimAllTo(char *in, char *out)
{
    in = TXT_StrimHead(in, strlen(in), (void*)" \t\r\n");
    strcpy(out, in);
    TXT_StrimAll(out);

    return out;
}

/* 得到字符串中第一个非空格\t \n \r 字符的指针, 如果找不到, 返回NULL . */
UCHAR * TXT_FindFirstNonBlank(IN UCHAR *pszStr, IN UINT ulLen)
{
    UINT i;

	/* skip white spaces in the string */
	for (i=0; i<ulLen; i++)
	{
		if (! TXT_IS_BLANK_OR_RN(pszStr[i]))
		{
			break;
		}
	}

    if (i == ulLen)
    {
        return NULL;
    }

    return pszStr + i;
}

BOOL_T TXT_IsInRange(IN CHAR cCh, IN UCHAR *pszStringRange, IN UINT ulLen)
{
    UINT i;

    for (i=0; i<ulLen; i++)
    {
        if (cCh == pszStringRange[i])
        {
            return TRUE;
        }
    }

    return FALSE;
}

UCHAR *TXT_FindFirstNonSuch(IN UCHAR *pszStr, IN UINT ulLen, IN UCHAR *pszNoSuch, IN UINT ulNoSuchLen)
{
    UINT i;

	/* skip pszNoSuch chars in the string */
	for (i=0; i<ulLen; i++)
	{
		if (! TXT_IsInRange(pszStr[i], pszNoSuch, ulNoSuchLen))
		{
			break;
		}
	}

    if (i == ulLen)
    {
        return NULL;
    }

    return pszStr + i;
}

/* 查找字符串中制定字符集中的任一个字符 */
CHAR * TXT_FindOneOf(IN CHAR *pszStr, IN CHAR *pszPattern)
{
    CHAR *pt = pszStr;
    CHAR *pt1;

    CHAR cPt;

    while (*pt != '\0')
    {
        cPt = *pt;

        pt1 = pszPattern;
        while (*pt1 != '\0')
        {
            if (*pt1 == cPt)
            {
                return pt;
            }
            pt1 ++;
        }

        pt ++;
    }

    return NULL;
}

/* 获取有多少个token */
UINT TXT_GetTokenNum(IN CHAR *pszStr, IN CHAR *pszPatterns)
{
    CHAR * pt = pszStr;
    CHAR * pt1;
    UINT uiCount = 0;
    UINT uiPatternLen;

    if (*pszStr == '\0') {
        return 0;
    }

    uiPatternLen = strlen(pszPatterns);

    do {
        pt = (CHAR*)TXT_FindFirstNonSuch((UCHAR*)pt, strlen(pt), (UCHAR*)pszPatterns, uiPatternLen);
        if (pt == NULL) {
            return uiCount;
        }

        pt1 = TXT_FindOneOf(pt, pszPatterns);
        if (NULL != pt1) {
            pt1 ++;
        }

        uiCount ++;

        pt = pt1;
    } while (pt != NULL);

    return uiCount;
}

/* pszPatterns是隔离符集合 */
UINT TXT_StrToToken(IN CHAR *pszStr, IN CHAR *pszPatterns, OUT CHAR *apszArgz[], IN UINT uiMaxArgz)
{
    CHAR * pt = pszStr;
    CHAR * pt1;
    UINT uiCount = 0;
    UINT uiPatternLen;

    if (*pszStr == '\0') {
        return 0;
    }

    if (uiMaxArgz == 0) {
        return 0;
    }

    uiPatternLen = strlen(pszPatterns);

    do {
        pt = (CHAR*)TXT_FindFirstNonSuch((UCHAR*)pt, strlen(pt), (UCHAR*)pszPatterns, uiPatternLen);
        if (pt == NULL) {
            return uiCount;
        }

        pt1 = TXT_FindOneOf(pt, pszPatterns);
        if (NULL != pt1) {
            *pt1 = '\0';
            pt1 ++;
        }

        apszArgz[uiCount] = pt;
        uiCount ++;

        pt = pt1;
    } while ((pt != NULL) && (uiCount < uiMaxArgz));

    return uiCount;
}

/*
 将解析后的结果放在HSTACK中返回
*/
HANDLE TXT_StrToDynamicToken(IN CHAR *pszStr, IN CHAR *pszPatterns)
{
    CHAR *pt1, *pt = pszStr;
    HANDLE hStack;
    UINT uiPatternLen;

    if (*pszStr == '\0')
    {
        return NULL;
    }

    hStack = HSTACK_Create(0);
    if (NULL == hStack)
    {
        return NULL;
    }

    uiPatternLen = strlen(pszPatterns);

    do {
        pt = (CHAR*)TXT_FindFirstNonSuch((void*)pt, strlen(pt), (void*)pszPatterns, uiPatternLen);
        if (pt == NULL)
        {
            return hStack;
        }

        pt1 = TXT_FindOneOf(pt, pszPatterns);
        if (NULL != pt1)
        {
            *pt1 = '\0';
            pt1 ++;
        }

        if (BS_OK != HSTACK_Push(hStack, pt))
        {
            HSTACK_Destory(hStack);
            return NULL;
        }

        pt = pt1;
    }while (pt != NULL);

    return hStack;
}

/*得到一行长度，不包括换行符*/
BS_STATUS TXT_GetLine(IN CHAR *pucTxtBuf, OUT UINT *puiLineLen, OUT BOOL_T *pbIsFoundLineEndFlag/*是否找到了\n*/, OUT CHAR **ppucLineNext)
{
    CHAR *spucLineEnd;
    
    BS_DBGASSERT(NULL != pucTxtBuf);
    BS_DBGASSERT(NULL != puiLineLen);
    BS_DBGASSERT(NULL != ppucLineNext);

    spucLineEnd = strchr(pucTxtBuf, '\n');
    if (spucLineEnd == NULL)
    {
        spucLineEnd = strchr(pucTxtBuf, '\r');
    }

    if (spucLineEnd == NULL)    /*没有找到换行符，则整个buf是一行*/
    {
        *pbIsFoundLineEndFlag = FALSE;
        *puiLineLen = strlen(pucTxtBuf);
        *ppucLineNext = NULL;
        return BS_OK;
    }

    *pbIsFoundLineEndFlag = TRUE;

    /*已经找到了换行符*/
    if (spucLineEnd > pucTxtBuf)    /*第一个字符不是换行符*/
    {
        if ((spucLineEnd[-1] == '\r') || (spucLineEnd[-1] == '\n'))
        {
            *puiLineLen = spucLineEnd - pucTxtBuf - 1;
        }
        else
        {
            *puiLineLen = spucLineEnd - pucTxtBuf;
        }
    }
    else
    {
        *puiLineLen = 0;
    }

    if (spucLineEnd[1] == '\0')
    {
        *ppucLineNext = NULL;
    }
    else
    {
        *ppucLineNext = spucLineEnd + 1;
    }

    return BS_OK;
}

/* 得到一行长度，不包括换行符 */
BS_STATUS TXT_N_GetLine(IN CHAR *pucTxtBuf, IN UINT ulLen, OUT UINT *pulLineLen, OUT BOOL_T *pbIsFoundLineEndFlag/*是否找到了\n*/, OUT CHAR **ppucLineNext)
{
    CHAR *spucLineEnd;
    
    BS_DBGASSERT(NULL != pucTxtBuf);
    BS_DBGASSERT(NULL != pulLineLen);
    BS_DBGASSERT(NULL != ppucLineNext);

    spucLineEnd = TXT_Strnchr(pucTxtBuf, '\n', ulLen);

    if (spucLineEnd == NULL)    /*没有找到换行符，则整个buf是一行*/
    {
        *pbIsFoundLineEndFlag = FALSE;
        *pulLineLen = ulLen;
        *ppucLineNext = NULL;
        return BS_OK;
    }

    *pbIsFoundLineEndFlag = TRUE;

    /*已经找到了换行符*/
    if (spucLineEnd > pucTxtBuf)    /*第一个字符不是换行符*/
    {
        if (spucLineEnd[-1] == '\r')
        {
            *pulLineLen = spucLineEnd - pucTxtBuf - 1;
        }
        else
        {
            *pulLineLen = spucLineEnd - pucTxtBuf;
        }
    }

    if (spucLineEnd[1] == '\0')
    {
        *ppucLineNext = NULL;
    }
    else
    {
        *ppucLineNext = spucLineEnd + 1;
    }

    return BS_OK;
}

CHAR *TXT_Strnchr(IN CHAR *pcBuf, IN CHAR ch2Find, IN UINT ulLen)
{
    UINT i;
    
    if (pcBuf == NULL)
    {
        return NULL;
    }

    for (i=0; i<ulLen; i++)
    {
        if (ch2Find == pcBuf[i])
        {
            return (pcBuf + i);
        }
        else if ('\0' == pcBuf[i])
        {
            return NULL;
        }
    }

    return NULL;
}

/* 在字符串中查找多个字符之一 */
CHAR * TXT_MStrnchr
(
    IN CHAR *pcString,
    IN UINT uiStringLen,
    IN CHAR *pcToFind
)
{
    CHAR *pcStringEnd;
    CHAR *pcChar;
    CHAR *pcFound = NULL;
    UINT uiPatternLen;

    pcStringEnd = pcString + uiStringLen;
    pcChar = pcString;
    uiPatternLen = (UINT)strlen(pcToFind);

    while (pcChar < pcStringEnd)
    {
        if (TXT_Strnchr(pcToFind, *pcChar, uiPatternLen) != NULL)
        {
            pcFound = pcChar;
            break;
        }

        pcChar ++;
    }

    return pcFound;
}

char * TXT_MStrchr(char *string, char *to_finds)
{
    return TXT_MStrnchr(string, strlen(string), to_finds);
}

/* 将字符串反序 */
char * TXT_Invert(char *in, char *out)
{
    int len = strlen(in);

    MEM_Invert(in, len, out);
    out[len] = '\0';

    return out;
}

/* 从最后开始扫描字符 */
CHAR * TXT_ReverseStrnchr(CHAR *pcBuf, CHAR ch2Find, UINT uiLen)
{
    LSTR_S lstr;
    lstr.pcData = pcBuf;
    lstr.uiLen = uiLen;
    return LSTR_ReverseStrchr(&lstr, ch2Find);
}

/* 从最后开始扫描字符 */
CHAR * TXT_ReverseStrchr(IN CHAR *pcBuf, IN CHAR ch2Find)
{
    LSTR_S lstr;
    lstr.pcData = pcBuf;
    lstr.uiLen = strlen(pcBuf);
    return LSTR_ReverseStrchr(&lstr, ch2Find);
}

CHAR *TXT_Strnstr(IN CHAR *s1, IN CHAR *s2, IN ULONG ulLen) 
{
	ULONG ulLen2;

	ulLen2 = strlen(s2);

	if (ulLen2 == 0)
	{
		return s1;
	}

	while (ulLen >= ulLen2)
    {
		ulLen--;

		if (!memcmp(s1, s2, ulLen2))
		{
			return s1;
		}

		s1++;
	}

	return NULL;
}

char * TXT_Strnistr(char *s1, char *s2, ULONG ulLen) 
{
	ULONG ulLen2;

	ulLen2 = strlen(s2);

	if (ulLen2 == 0) {
		return s1;
	}

	while (ulLen >= ulLen2) {
		ulLen--;

        if (0 == strnicmp(s1, s2, ulLen2)) {
            return s1;
        }

		s1++;
	}

	return NULL;
}

UINT TXT_Str2Ui(IN CHAR *pcStr)
{
    UINT uiNum = 0;

    TXT_Atoui(pcStr, &uiNum);

    return uiNum;
}

BS_STATUS TXT_Atoui(IN CHAR *pszBuf, OUT UINT *puiNum)
{
    if ((pszBuf == NULL) || (pszBuf[0] == '\0')) {
        RETURN(BS_BAD_PTR);
    }

	if (FALSE == CTYPE_IsNumString(pszBuf)) {
		RETURN(BS_ERR);
	}

    sscanf(pszBuf, "%u", puiNum);

    return BS_OK;
}

BS_STATUS TXT_Atoll(CHAR *pszBuf, OUT INT64 *var)
{
    if ((pszBuf == NULL) || (pszBuf[0] == '\0')) {
        RETURN(BS_BAD_PTR);
    }

	if (FALSE == CTYPE_IsNumString(pszBuf)) {
		RETURN(BS_ERR);
	}

    sscanf(pszBuf, "%lld", var);

    return BS_OK;
}

BS_STATUS TXT_AtouiWithCheck(IN CHAR *pszBuf, OUT UINT *puiNum)
{
    BS_STATUS eRet;

    /*4294967295是UINT的最大表示范围*/
    if (strlen(pszBuf) >= sizeof("4294967295")) {
        RETURN(BS_OUT_OF_RANGE);
    }

    eRet = TXT_Atoui(pszBuf, puiNum);
    if (eRet != BS_OK) {
        return eRet;
    }

    return BS_OK;
}

BS_STATUS TXT_XAtoui(IN CHAR *pszBuf, OUT UINT *pulNum)
{
    UINT ulNum = 0;

    if ((pszBuf == NULL) || (pszBuf[0] == '\0'))
    {
        RETURN(BS_BAD_PTR);
    }

    sscanf(pszBuf, "%x", &ulNum);

    *pulNum = ulNum;

    return BS_OK;
}

CHAR TXT_Random(void)
{
    /* A-Z, a-z, 0-9 共62个字符 */
    UINT uiRandom;

    uiRandom = RAND_Get() % 62;

    if (uiRandom < 26)
    {
        return uiRandom + 'A';
    }

    if (uiRandom < 52)
    {
        return uiRandom - 26 + 'a';
    }

    return uiRandom - 52 + '0';
}

/*  查找第N个字符在字符串中的位置  */
CHAR * TXT_StrchrX(IN CHAR *pszStr, IN CHAR pcToFind, IN UINT ulNum)
{
    CHAR *pcFind = NULL;
    
    if (NULL == pszStr)
    {
        return NULL;
    }

    pcFind = pszStr;
    while (ulNum > 0)
    {
        pcFind = strchr(pcFind, pcToFind);
        if (NULL == pcFind)
        {
            break;
        }
		ulNum--;
		if (ulNum > 0)
		{
			pcFind++;
		}        
    }

    return pcFind;
}

BS_STATUS TXT_FindBracket
(
    IN CHAR *pszString,
    IN UINT ulLen,
    IN CHAR *pszBracket/*两个字节,用来表示左右括号*/,
    OUT CHAR **ppcStart,
    OUT CHAR **ppcEnd
)
{
    UINT ulCount = 0;
    UINT i;
    CHAR *pcStart, *pcEnd;
    
    BS_DBGASSERT(NULL != pszString);

    pcStart = NULL;
    pcEnd = NULL;

    for (i=0; i<ulLen; i++)
    {
        if (pszString[i] == pszBracket[0])
        {
            if (pcStart == NULL)
            {
                pcStart = pszString + i;
            } 
            ulCount++;
        }
        else if (pszString[i] == pszBracket[1])
        {
            if (ulCount == 0)
            {
                RETURN(BS_ERR);
            }
            
            ulCount--;

            if (ulCount == 0)
            {
                pcEnd = pszString + i;
                break;
            }
        }
    }

    if (pcEnd == NULL)
    {
        RETURN(BS_ERR);
    }

    *ppcStart = pcStart;
    *ppcEnd = pcEnd;

    return BS_OK;
}

UINT TXT_CountCharNum(IN CHAR *pszString, IN CHAR cCharToCount)
{
    UINT ulCount = 0;
    UINT i = 0;

    while (pszString[i] != '\0')
    {
        if (pszString[i] == cCharToCount)
        {
            ulCount ++;
        }
        i++;
    }

    return ulCount;    
}

VOID TXT_Strlwr(INOUT CHAR *pszString)
{
    CHAR *pcTemp;
    
    BS_DBGASSERT(NULL != pszString);

    pcTemp = pszString;

    while (*pcTemp != '\0')
    {
        if (NUM_IN_RANGE(*pcTemp, 'A', 'Z'))
        {
            *pcTemp += 'a' - 'A';
        }

        pcTemp ++;
    }
}

char *TXT_Strdup(IN CHAR *pcStr)
{
    UINT uiLen;
    CHAR *pcDup;

    if (NULL == pcStr)
    {
        return NULL;
    }

    uiLen = strlen(pcStr);
    pcDup = MEM_Malloc(uiLen + 1);
    if (NULL == pcDup)
    {
        return NULL;
    }
    TXT_Strlcpy(pcDup, pcStr, uiLen + 1);

    return pcDup;
}

/* 
Copy src to string dst of size siz. At most siz-1 characters
 will be copied. Always NUL terminates (unless siz == 0).
 Returns strlen(src); if retval >= siz, truncation occurred.

 用法: TXT_Strlcpy(szDest, szSrc, sizeof(szDest));
*/
UINT TXT_Strlcpy(IN CHAR *pcDest, IN CHAR *pcSrc, IN UINT uiSize)
{
    ULONG n;
    CHAR *p;

    for (p = pcDest, n = 0; n + 1 < uiSize && *pcSrc != '\0';  ++p, ++pcSrc, ++n)
    {
        *p = *pcSrc;
    }
    *p = '\0';
    if(*pcSrc == '\0')
    {
        return n;
    }
    else
    {
        return n + strlen (pcSrc);
    }
}


/*
 * The strlcat() function appends the NUL-terminated string src to the end
 * of dst. It will append at most size - strlen(dst) - 1 bytes, NUL-termi-
 * nating the result.
 *
 * The strlcpy() and strlcat() functions return the total length of the
 * string they tried to create.  For strlcpy() that means the length of src.
 * For strlcat() that means the initial length of dst plus the length of
 * src. While this may seem somewhat confusing it was done to make trunca-
 * tion detection simple.
 *
 *
 */
ULONG TXT_Strlcat(char *dst, const char *src, ULONG siz)
{
    char *d = dst;
    const char *s = src;
    ULONG n = siz;
    ULONG dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
    {
        d++;
    }
    
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
    {
        return(dlen + strlen(s));
    }

    while (*s != '\0')
    {
        if (n != 1)
        {
            *d++ = *s;
            n--;
        }
        s++;
    }

    *d = '\0';

    return (dlen + (s - src));    /* count does not include NUL */
}


/* 向指定位置插入一个字符,这个位置之后的字符都向后挪动 */
BOOL_T TXT_InsertChar(IN CHAR *pcDest, IN UINT uiOffset, IN CHAR cInsertChar)
{
    UINT uiLen;
    UINT i;

    uiLen = strlen(pcDest);
    if (uiLen < uiOffset)
    {
        return FALSE;
    }

    for (i=uiLen; i>uiOffset; i--)
    {
        pcDest[i] = pcDest[i-1];
    }

    pcDest[uiOffset] = cInsertChar;
    pcDest[uiLen + 1] = '\0';

    return TRUE;
}

/* 从指定位置删除一个字符,这个位置之后的字符都向前挪动 */
BOOL_T TXT_RemoveChar(IN CHAR *pcDest, IN UINT uiOffset)
{
    UINT uiLen;
    UINT i;

    uiLen = strlen(pcDest);
    if (uiLen <= uiOffset)
    {
        return FALSE;
    }

    for (i=uiOffset; i<uiLen; i++)
    {
        pcDest[i] = pcDest[i+1];
    }

    return TRUE;    
}

/* 获取两个字符串相同前缀的长度 */
ULONG TXT_GetSamePrefixLen(IN CHAR *pcStr1, IN CHAR *pcStr2)
{
    ULONG ulLen = 0;

    while ((pcStr1[ulLen] != '\0') && (pcStr1[ulLen] == pcStr2[ulLen]))
    {
        ulLen ++;
    }

    return ulLen;
}

BS_STATUS TXT_StrCpy(IN CHAR *pszDest, IN CHAR *pszSrc)
{
    TXT_Strlcpy(pszDest, pszSrc, strlen(pszSrc) + 1);

    return BS_OK;
}

VOID TXT_StrSplit(IN CHAR *pcString, IN CHAR cSplitChar, OUT LSTR_S * pstStr1, OUT LSTR_S * pstStr2)
{
    LSTR_S stString;

    stString.pcData = pcString;
    stString.uiLen = strlen(pcString);

    LSTR_Split(&stString, cSplitChar, pstStr1, pstStr2);
}

VOID TXT_MStrSplit(IN CHAR *pcString, IN CHAR *pcSplitChar, OUT LSTR_S * pstStr1, OUT LSTR_S * pstStr2)
{
    LSTR_S stString;

    stString.pcData = pcString;
    stString.uiLen = strlen(pcString);

    LSTR_MSplit(&stString, pcSplitChar, pstStr1, pstStr2);
}

/* 将字符串添加转义字符 */
char * TXT_Str2Translate(char *str, char *trans_char_sets, char *out, int out_size)
{
    char *c;
    char *dst;
    char *dst_end = out + out_size - 1;
    int count = strlen(trans_char_sets);

    c = str;
    dst = out;

    while (*c != '\0') {
        if (dst >= dst_end) {
            return NULL;
        }

        if (TXT_IsInRange(*c, (UCHAR*)trans_char_sets, count)) {
            *dst = '\\';
            dst++;
            if (dst >= dst_end) {
                return NULL;
            }
        }
        *dst = *c;
        dst++;
        c++;
    }

    *dst = '\0';

    return out;
}

/* 将数字转为二进制字符串:
 * min_len: 最小输出字节数, 如果不足则在前面补0
 */
char * TXT_Num2BitString(uint64_t v, int min_len, OUT char *str)
{
    int i, j = 0;
    int flag = 0;

    for(i=63; i>=0; i--) {
        if (i < min_len) {
            flag = 1;
        }
        if (v & (1ULL << i)) {
            flag = 1;
        }
        if (flag) {
            str[j] = '0';
            if (v & (1ULL << i)) {
                str[j] = '1';
            }
            j++;
        }
    }

    str[j] = '\0';

    return str;
}


