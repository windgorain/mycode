
#include "bs.h"

#include "utl/des_utl.h"

#include "des_locl.h"

void DES_Ecb_encrypt
(
    IN const_DES_cblock *input,
    OUT DES_cblock *output,
    IN DES_key_schedule *ks,
    IN int enc
)
{
	register UINT l;
	UINT ll[2];
	const unsigned char *in = &(*input)[0];
	unsigned char *out = &(*output)[0];

	c2l(in,l); ll[0]=l;
	c2l(in,l); ll[1]=l;
	DES_encrypt1(ll,ks,enc);
	l=ll[0]; l2c(l,out);
	l=ll[1]; l2c(l,out);
	l=ll[0]=ll[1]=0;
}


void DES_Ecb3Encrypt
(
    IN DES_cblock *pstInput,
    OUT DES_cblock *pstOutput,
    IN DES_key_schedule *pstKs1, 
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3,
    IN BOOL_T bIsEnc
)
{
    DES_ecb3_encrypt(pstInput, pstOutput, pstKs1, pstKs2, pstKs3, bIsEnc);
}

void DES_Ede3CbcEncrypt
(
    IN UCHAR *pucInput,
    OUT UCHAR *pucOutput,
    IN INT lLength,
    IN DES_key_schedule *pstKs1,
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3,
    INOUT DES_cblock *pstIvec,
    IN BOOL_T bIsEnc
)
{
    DES_ede3_cbc_encrypt(pucInput, pucOutput, lLength, pstKs1, pstKs2, pstKs3, pstIvec, bIsEnc);
}

void DES_Ede3Cfb64Encrypt
(
    IN UCHAR *pucIn,
    OUT UCHAR *pucOut,
    IN long lLength, 
    IN DES_key_schedule *pstKs1,
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3, 
    INOUT DES_cblock *pstIvec,
	INOUT INT *plNum, 
	IN BOOL_T bIsEnc
)
{
    DES_ede3_cfb64_encrypt(pucIn, pucOut, lLength, pstKs1, pstKs2, pstKs3, pstIvec, plNum, bIsEnc);
}

void DES_Ede3CfbEncrypt
(
    IN UCHAR *pucIn,
    OUT UCHAR *pucOut,
    IN INT lNumbits,
    IN long lLength, 
    IN DES_key_schedule *pstKs1,
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3, 
    INOUT DES_cblock *pstIvec,
	IN BOOL_T bIsEnc
)
{
    DES_ede3_cfb_encrypt(pucIn, pucOut, lNumbits, lLength, pstKs1, pstKs2, pstKs3, pstIvec, bIsEnc);
}

void DES_Ede3Ofb64Encrypt
(
    IN UCHAR *pucIn,
    OUT UCHAR *pucOut,
    IN long lLength, 
    IN DES_key_schedule *pstKs1,
    IN DES_key_schedule *pstKs2,
    IN DES_key_schedule *pstKs3, 
    INOUT DES_cblock *pstIvec,
    INOUT INT *plNum
)
{
    DES_ede3_ofb64_encrypt(pucIn, pucOut, lLength, pstKs1, pstKs2, pstKs3, pstIvec, plNum);
}

