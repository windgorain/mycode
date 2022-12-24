#ifndef __BASE64_UTL_H_
#define __BASE64_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* 根据数据长度获取base64转换后的字符串长度,不含'/0' */
#define BASE64_LEN(ulDataLen) (((ulDataLen) + 2) / 3 * 4)

/* 根据数据长度获取base64反转换后的字符串长度,不含'/0' */
#define BASE64_CLEAR_LEN(ulBase64Len)  ((((ulBase64Len) + 3) / 4) * 3)

int BASE64_Encode(IN UCHAR *pucData, IN UINT uiDataLen, OUT CHAR *pcOut);
int BASE64_Decode(IN CHAR *pcInput, IN UINT uiDataLen, OUT UCHAR *pucOutput);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__BASE64_UTL_H_*/



