/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-4-29
* Description: 
* History:     
******************************************************************************/

#ifndef __SPRINTF_UTL_H_
#define __SPRINTF_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

struct printf_spec {
	UCHAR	type;		/* format_type enum */
	UCHAR	flags;		/* flags to number() */
	UCHAR	base;		/* number base, 8, 10 or 16 only */
	UCHAR	qualifier;
    SHORT	field_width;
	SHORT	precision;	/* # of digits/chars */
};

typedef struct {
    struct printf_spec spec;
    char *fmt;
    int read;
}FormatCompile_ELE_S;

#define SPRINTF_COMPILE_ELE_NUM 128

typedef struct {
    FormatCompile_ELE_S ele[SPRINTF_COMPILE_ELE_NUM];
}FormatCompile_S;

INT BS_Printf(const char *fmt, ...);
INT BS_Sprintf(char * buf, const char *fmt, ...);
INT BS_Snprintf(char * buf, IN INT iLen, const char *fmt, ...);
INT BS_Vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int BS_Scnprintf(char *buf, int size, const char *fmt, ...);

int BS_FormatCompile(FormatCompile_S *fc, char *fmt);
int BS_VFormat(FormatCompile_S *fc, char *buf, size_t size, va_list args);
int BS_Format(FormatCompile_S *fc, char * buf, ...);
int BS_FormatN(FormatCompile_S *fc, char *buf, int size, ...);

/* 溢出则返回size-1, 即实际输出的长度(不包含\0), 可用于不判断返回值连续拼装 */
#define scnprintf BS_Scnprintf

/* 溢出返回-1. 不能用于不判断返回值的连续拼装 */
#define SNPRINTF(buf,size, ...) ({ \
        int _nlen = snprintf((buf), (size), ##__VA_ARGS__); \
        if (_nlen >= (size)) _nlen = -1; \
        _nlen; })

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SPRINTF_UTL_H_*/


