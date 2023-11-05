/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/in_checksum.h"


USHORT IN_CHKSUM_AddRaw(IN USHORT usCurrentSum, IN UCHAR *pucData, IN UINT uiDataLen)
{
    UINT uiTmp;
    UINT i;
    UINT uiCheckSum = usCurrentSum;

    uiTmp = uiDataLen & ~1U;
    
	for (i = 0; i < uiTmp; i += 2)
    {
        uiCheckSum += (((pucData[i] << 8) & 0xFF00) + (pucData[i+1] & 0xFF));
	}

    if (i < uiDataLen)
    {
        uiCheckSum += ((pucData[i] << 8) & 0xFF00);
    }

    
    while (uiCheckSum & 0xffff0000)
    {
        uiCheckSum = (uiCheckSum & 0xFFFF) + ((uiCheckSum >> 16) & 0xFFFF);
    }

	return (USHORT) uiCheckSum;
}

USHORT IN_CHKSUM_DelRaw(IN USHORT usCurrentSum, IN UCHAR *pucData, IN UINT uiDataLen)
{
    UINT uiTmp;
    UINT i;
    UINT uiCheckSum = usCurrentSum;
    USHORT usWork;

    uiTmp = uiDataLen & ~1U;

    for (i = 0; i < uiTmp; i += 2)
    {
        usWork = (((pucData[i] << 8) & 0xFF00) + (pucData[i+1] & 0xFF));
        if (uiCheckSum < usWork)
        {
            uiCheckSum += 0xffff;
        }
        uiCheckSum -= usWork;
	}

    if (i<uiDataLen)
    {
        usWork = ((pucData[i] << 8) & 0xFF00);
        if (uiCheckSum < usWork)
        {
            uiCheckSum += 0xffff;
        }
        uiCheckSum -= usWork;
    }

    return (USHORT)uiCheckSum;
}


