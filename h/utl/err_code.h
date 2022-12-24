/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2008-1-15
* Description: 
* History:     
******************************************************************************/
#ifndef __ERR_CODE_H_
#define __ERR_CODE_H_

#include "utl/print_color.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define ERR_INFO_SIZE 256

typedef struct {
    const char *file_name;
    const char *func_name;
    char info[ERR_INFO_SIZE];
    int line;
    int err_code;
}ERR_CODE_S;

void ErrCode_Set(int err_code, char *info, const char *file_name, const char *func_name, int line);
void ErrCode_Clear(void);
const char * ErrCode_GetFileName(void);
int ErrCode_GetLine(void);
int ErrCode_GetErrCode(void);
char * ErrCode_GetInfo(void);
char * ErrCode_Build(OUT char *buf, int buf_size);
void ErrCode_Print(void);
void ErrCode_Output(PF_PRINT_FUNC output);
void ErrCode_FatalError(char *format, ...);

/* print file line */
#define PRINTFL() PRINT_GREEN("%s(%d) \n", __FILE__, __LINE__)

/* print file line msg */
#define PRINTFLM_COLOR(_color, _fmt, ...) PRINT_COLOR(_color, "%s(%d): " _fmt, __FILE__, __LINE__, ##__VA_ARGS__)


#define PRINTFLM_BLACK(fmt, ...) PRINTFLM_COLOR(SHELL_FONT_COLOR_BLACK, fmt, ##__VA_ARGS__)
#define PRINTFLM_GREEN(fmt, ...) PRINTFLM_COLOR(SHELL_FONT_COLOR_GREEN, fmt, ##__VA_ARGS__)
#define PRINTFLM_RED(fmt, ...) PRINTFLM_COLOR(SHELL_FONT_COLOR_RED, fmt, ##__VA_ARGS__)
#define PRINTFLM_YELLOW(fmt, ...) PRINTFLM_COLOR(SHELL_FONT_COLOR_YELLOW, fmt, ##__VA_ARGS__)
#define PRINTFLM_CYAN(fmt, ...) PRINTFLM_COLOR(SHELL_FONT_COLOR_CYAN, fmt, ##__VA_ARGS__)
#define PRINTFLM_PURPLE(fmt, ...) PRINTFLM_COLOR(SHELL_FONT_COLOR_PURPLE, fmt, ##__VA_ARGS__)
#define PRINTFLM_BLUE(fmt, ...) PRINTFLM_COLOR(SHELL_FONT_COLOR_BLUE, fmt, ##__VA_ARGS__)
#define PRINTFLM_WHITE(fmt, ...) PRINTFLM_COLOR(SHELL_FONT_COLOR_WHITE, fmt, ##__VA_ARGS__)

#define PRINTFLM(fmt, ...) PRINTFLM_WHITE(fmt, ##__VA_ARGS__)
#define PRINTFLM_ERR(fmt, ...) PRINTFLM_RED(fmt, ##__VA_ARGS__)
#define PRINTFLM_WARN(fmt, ...) PRINTFLM_YELLOW(fmt, ##__VA_ARGS__)

#define ERROR_SET_INFO(errcode, info)  ErrCode_Set((errcode), info, __FILE__, __FUNCTION__, __LINE__)
#define ERROR_SET(errcode)  ERROR_SET_INFO((errcode), NULL)

#define RETURN(ulErrCode) \
    do  {   \
        if (ulErrCode != BS_OK) { \
            ERROR_SET(ulErrCode); \
        } \
        return (ulErrCode); \
    }while(0)

#define RETURNI(ulErrCode, fmt, ...) do {return ERR_VSet(ulErrCode, fmt, ##__VA_ARGS__); } while(0)

static inline int _err_code_set(int code, char *info, char *file, const char *func, int line)
{
    ErrCode_Set(code, info, file, func, line);
    return code;
}

#define ERR_Set(code, errinfo) _err_code_set((code), (errinfo), __FILE__, __FUNCTION__, __LINE__)

#define ERR_VSet(code, fmt, ...) ({ \
        char _buf[ERR_INFO_SIZE]; \
        snprintf(_buf, sizeof(_buf), (fmt), ##__VA_ARGS__); \
        _err_code_set((code), _buf, __FILE__, __FUNCTION__, __LINE__); \
        })

#define RETCODE(ulErrCode)  (ulErrCode)

#define FATAL(...) do { \
    fprintf(stderr, "Error(%s:%d): ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    exit(1); \
} while(0)

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__ERR_CODE_H_*/


