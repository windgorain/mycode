#ifndef __CTYPE_UTL_H_
#define __CTYPE_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


static inline BOOL_T CTYPE_IsXDigit(IN UCHAR ucChar)
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

static inline BOOL_T CTYPE_IsNumString(IN CHAR *pcString)
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
#endif 

#endif 


