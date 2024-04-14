#ifndef __CTYPE_UTL_H_
#define __CTYPE_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define ISSPACE(c)	(((c) == ' ') || ((c) == '\t'))
#define ISDIGIT(c)	('0' <= (c) && (c) <= '9')
#define ISLOWER(c)	('a' <= (c) && (c) <= 'z')
#define TOLOWER(c)  ((c) | 0x20)
#define ISUPPER(c)  (((c) >= 'A') && ((c) <= 'Z'))

static inline int ISXDIGIT(int ch)
{
	if (ISDIGIT(ch))
		return TRUE;

	if ((ch >= 'a') && (ch <= 'f'))
		return TRUE;

	return (ch >= 'A') && (ch <= 'F');
}

/*
checks for a hexadecimal digits, that is, one of
      0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F
*/
static inline BOOL_T CTYPE_IsXDigit(UCHAR ucChar)
{
    if (('0' <= ucChar) && ('9' >= ucChar))
    {
        return TRUE;
    }

    if (('a' <= ucChar) && ('f' >= ucChar))
    {
        return TRUE;
    }

    if (('A' <= ucChar) && ('F' >= ucChar))
    {
        return TRUE;
    }

    return FALSE;
}

static inline BOOL_T CTYPE_IsNumString(CHAR *pcString)
{
    ULONG ulLen;
    ULONG i;

    if (NULL == pcString)
    {
        return FALSE;
    }

    ulLen = strlen(pcString);

    for (i=0; i<ulLen; i++)
    {
        if ((pcString[i] < '0') || (pcString[i] > '9'))
        {
            return FALSE;
        }
    }

    return TRUE;
}




#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__CTYPE_UTL_H_*/


