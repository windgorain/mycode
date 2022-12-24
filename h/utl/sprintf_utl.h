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

int BS_FormatCompile(FormatCompile_S *fc, char *fmt);
int BS_VFormat(FormatCompile_S *fc, char *buf, size_t size, va_list args);
int BS_Format(FormatCompile_S *fc, char * buf, ...);
int BS_FormatN(FormatCompile_S *fc, char *buf, int size, ...);

static inline int BS_Scnprintf(char *buf, int size, const char *fmt, ...)
{
	va_list args;
	int ret_len;

    if (! buf) {
        return -1;
    }

    assert(size > 1);

	va_start(args, fmt);
	ret_len = vsnprintf(buf, size, fmt, args);
	va_end(args);

    if (ret_len >= size) {
        return size - 1;
    }

    return ret_len;
}

#define scnprintf BS_Scnprintf

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SPRINTF_UTL_H_*/


