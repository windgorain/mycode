#include "bs.h"

#include "utl/base64_utl.h"

/* 返回Encode后的长度, 不包含最后的'\0' */
int BASE64_Encode(IN UCHAR *pucData, IN UINT uiDataLen, OUT CHAR *pcOut)
{
    static const char base64_chars[] =	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    ULONG word;
    UCHAR* pucInput = NULL;
    CHAR * pcOutput = NULL;
    ULONG ulOutLen = 0;
    ULONG n;

    pucInput = pucData;
    pcOutput = pcOut;

    while (uiDataLen > 0)
    {
        n = (uiDataLen < 3 ? uiDataLen : 3);

        word = pucInput[0] << 16;
        if (n > 1)
        {
            word |= pucInput[1] << 8;
        }

        if (n > 2)
        {
            word |= pucInput[2];
        }

        pcOutput[0] = base64_chars[(word >> 18) & 0x3F];
        pcOutput[1] = base64_chars[(word >> 12) & 0x3F];
        if (n > 1)
        {
            pcOutput[2] = base64_chars[(word >> 6) & 0x3F];
        }
        else
        {
            pcOutput[2] = '=';
        }

        if (n > 2)
        {
            pcOutput[3] = base64_chars[word & 0x3F];
        }
        else
        {
            pcOutput[3] = '=';
        }

        pucInput += n;
        uiDataLen -= n;
        pcOutput += 4;
        ulOutLen += 4;
    }

	pcOut[ulOutLen] = '\0';
    
	return ulOutLen;    
}

/* 返回解释之后的长度, 出错返回<0 */
int BASE64_Decode(IN CHAR *pcInput, IN UINT uiDataLen, OUT UCHAR *pucOutput)
{
    int vals[4];
    int i, j, v, len;
    unsigned word;
    CHAR c;

    UINT  ulOutLen   = 0;
    CHAR*  pcTempIn   = pcInput;
    UCHAR* pucTempOut = pucOutput;

    for (i = 0; i < (int)uiDataLen; i += 4)
    {
        for (j = 0; j < 4; j++)
        {
    	    c = pcTempIn[j];
            if (c >= 'A' && c <= 'Z')
            {
                v = c - 'A';
            }
            else if (c >= 'a' && c <= 'z')
            {
                v = c - 'a' + 26;
            }
            else if (c >= '0' && c <= '9')
            {
                v = c - '0' + 52;
            }
            else if (c == '+')
            {
                v = 62;
            }
            else if (c == '/')
            {
                v = 63;
            }
            else if (c == '=')
            {
                v = -1;
            }
            else /* invalid input */
            {
                return -1;
            }

            vals[j] = v;
        }

        if (vals[0] == -1 || vals[1] == -1)
        {
            return -1;
        }

        if (vals[2] == -1 && vals[3] != -1)
        {
            return -1;
        }

        if (vals[3] != -1)
        {
            len = 3;
        }
        else if (vals[2] != -1)
        {
            len = 2;
        }
        else
        {
            len = 1;
        }

        word = ( (vals[0] << 18)         |
                 (vals[1] << 12)         |
                 ((vals[2] & 0x3F) << 6) |
                 (vals[3] & 0x3F) );

        pucTempOut[0] = (unsigned char)((word >> 16) & 0xFF);

        if (len > 1)
        {
            pucTempOut[1] = (unsigned char)((word >> 8) & 0xFF);
        }

        if (len > 2)
        {
            pucTempOut[2] = (unsigned char)(word & 0xFF);
        }

        ulOutLen += len;
        pcTempIn += 4;
        pucTempOut += len;
    }

    return ulOutLen;
}

