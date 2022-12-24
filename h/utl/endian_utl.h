/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-7
* Description: 
* History:     
******************************************************************************/

#ifndef __ENDIAN_UTL_H_
#define __ENDIAN_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

static inline USHORT Host2Litter16(IN USHORT usData)
{
    UCHAR *pucData = (UCHAR*)(VOID*)&usData;

    return (((USHORT)pucData[0]) | ((USHORT)pucData[1] << 8));
}

static inline UINT Host2Litter32(IN UINT uiData)
{
    UCHAR *pucData = (UCHAR*)(VOID*)&uiData;

    return (((UINT)pucData[0]) | ((UINT)pucData[1] << 8) | ((UINT)pucData[2] << 16) | ((UINT)pucData[3] << 24));
}

static inline USHORT Litter2Host16(IN USHORT usData)
{
    return Host2Litter16(usData);
}

static inline UINT Litter2Host32(IN UINT uiData)
{
    return Host2Litter32(uiData);
}

#ifdef IN_MAC

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#endif

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__ENDIAN_UTL_H_*/


