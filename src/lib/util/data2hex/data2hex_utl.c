/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-22
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_DATA2HEX

#include "bs.h"

#include "utl/data2hex_utl.h"

void UCHAR_2_HEX(UCHAR c, OUT CHAR *hex)
{
    static char * hexchars="0123456789abcdef";

    hex[0] = hexchars[(c>>4) & 0xf];
    hex[1] = hexchars[(c) & 0xf];
}

/* funcs */
BS_STATUS DH_Data2Hex(IN UCHAR *pucData, IN UINT ulLen, OUT CHAR *pszOutString)
{
    UINT i;
    
    if ((pucData == NULL) || (pszOutString == NULL))
    {
        RETURN(BS_NULL_PARA);
    }

    for (i=0; i<ulLen; i++)
    {
        UCHAR_2_HEX(pucData[i], (pszOutString + 2*i));
    }

	return BS_OK;
}

BS_STATUS DH_Hex2Data(IN CHAR *pszHex, IN UINT ulHexLen, OUT UCHAR *pucData)
{
    UINT i;

    if ((pszHex == NULL) || (pucData == NULL))
    {
        RETURN(BS_NULL_PARA);
    }

    for (i=0; i<ulHexLen/2; i++)
    {
        if (BS_OK != HEX_2_UCHAR(pszHex + i*2, pucData + i))
        {
            RETURN(BS_ERR);
        }
    }

	return BS_OK;
}

BS_STATUS DH_Data2HexString(IN UCHAR *pucData, IN UINT ulLen, OUT CHAR *pszOutString)
{
    BS_STATUS eRet;

    eRet = DH_Data2Hex(pucData, ulLen, pszOutString);
    if (BS_OK != eRet)
    {
        return eRet;
    }
    pszOutString[ulLen * 2] = '\0';
	return BS_OK;
}

BS_STATUS DH_HexString2Data(IN CHAR *pszHexString, OUT void *data)
{
    if (pszHexString == NULL)
    {
        RETURN(BS_NULL_PARA);
    }

    return DH_Hex2Data(pszHexString, strlen(pszHexString), data);
}

