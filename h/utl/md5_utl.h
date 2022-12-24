/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-22
* Description: 
* History:     
******************************************************************************/

#ifndef __MD5_UTL_H_
#define __MD5_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define MD5_LEN 16

typedef struct 
{
	UINT	state[4];				/* state (ABCD) */
	UINT	count[2];				/* number of bits, modulo 2^64 (lsb first) */
	UCHAR	buffer[64];				/* input buffer */
} MD5_CTX;


void MD5UTL_Init (IN MD5_CTX *context);
void MD5UTL_Update (IN MD5_CTX *context, IN UCHAR *input, IN UINT inputLen);
void MD5UTL_Final (OUT UCHAR digest[MD5_LEN], IN MD5_CTX *context);

//  用MD5算法取散列值
//  参数:	szSour 源字符串
//			iLen   源字符串长度
//			szDest 目的串(16字节)
extern BS_STATUS MD5_Create(IN void *buf, IN UINT ulLen, OUT UCHAR aucDest[MD5_LEN]);



#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__MD5_UTL_H_*/


