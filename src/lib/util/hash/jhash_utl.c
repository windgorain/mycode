/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-6-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/jhash_utl.h"


/*****************************************************************************
  Description: JHash通用接口，对一系列的字节计算Hash值。
               该函数对pKey的起始地址和长度没有对齐要求。
        Input: pKey        ----  存放Hash Key的缓冲区
               uiLength    ----  Hash Key的长度，单位字节
       Return: Hash值
      Caution: 如果Hash的Key为一个结构，需要注意结构所占空间中否有可能出现随机值的地方。
               如果可能出现随机值，需要对这些地方明确赋值。
*****************************************************************************/
UINT JHASH_GeneralBuffer(VOID *pKey, UINT uiLength, UINT initval)
{
    UINT uia, uib, uic;
    UINT uiRemainlen;
    UCHAR *pucKey = (UCHAR *)pKey;

    uiRemainlen = uiLength;
    uia = uib = JHASH_GOLDEN_RATIO;
    uic = initval;

    while (uiRemainlen >= 12)
    {
        uia += (pucKey[0] +((UINT)pucKey[1]<<8) +((UINT)pucKey[2]<<16) +((UINT)pucKey[3 ]<<24));
        uib += (pucKey[4] +((UINT)pucKey[5]<<8) +((UINT)pucKey[6]<<16) +((UINT)pucKey[7 ]<<24));
        uic += (pucKey[8] +((UINT)pucKey[9]<<8) +((UINT)pucKey[10]<<16)+((UINT)pucKey[11]<<24));

        JHASH_MIX(uia,uib,uic);

        pucKey      += 12;
        uiRemainlen -= 12;
    }
    
    switch (uiRemainlen)
    {
        case 11:
            uic += ((UINT)pucKey[10]<<24);
        case 10:
            uic += ((UINT)pucKey[9]<<16);
        case 9 :
            uic += ((UINT)pucKey[8]<<8);         
        case 8 :
            uib += ((UINT)pucKey[7]<<24);      
        case 7 :
            uib += ((UINT)pucKey[6]<<16);
        case 6 :
            uib += ((UINT)pucKey[5]<<8);
        case 5 :
            uib += pucKey[4];   
        case 4 :
            uia += ((UINT)pucKey[3]<<24);
        case 3 :
            uia += ((UINT)pucKey[2]<<16);
        case 2 :
            uia += ((UINT)pucKey[1]<<8);
        case 1 :
            uia += pucKey[0];
        default:
            uic += uiLength;
    };

    JHASH_MIX(uia,uib,uic);

    return uic;
}

/*****************************************************************************
  Description: 对一个连续存放的Key计算Hash值。要求Key的起始地址和长度4字节对齐。
        Input: puiKey      ----  存放Hash Key的缓冲区
               uiLength    ----  Hash Key中UINT的个数。
      Caution: 如果Hash的Key为一个结构，需要注意结构所占空间中否有可能出现随机值的地方。
               如果可能出现随机值，需要对这些地方明确赋值。
*****************************************************************************/
UINT JHASH_U32Buffer(UINT *puiKey, UINT uiLength, UINT initval)
{
    UINT uia, uib, uic;
    UINT uiRemainlen;

    uia = uib = JHASH_GOLDEN_RATIO;
    uic = initval;
    uiRemainlen = uiLength;

    while (uiRemainlen >= 3)
    {
        uia += puiKey[0];
        uib += puiKey[1];
        uic += puiKey[2];

        JHASH_MIX(uia, uib, uic);

        puiKey += 3;
        uiRemainlen -= 3;
    }
    switch (uiRemainlen)
    {
        case 2 :
            uib += puiKey[1];  
        case 1 :
            uia += puiKey[0];
        default:
            uic += uiLength * 4;
    };

    JHASH_MIX(uia,uib,uic);

    return uic;
}


