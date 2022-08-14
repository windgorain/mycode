/*================================================================
*   Created：2018.09.22
*   Description：
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/err_code.h"

static THREAD_LOCAL ERR_CODE_S g_err_code;

void ErrCode_Set(int err_code, char *info, char *file_name, const char *func_name, int line)
{
    ERR_CODE_S *err_code_ctrl = &g_err_code;

    err_code_ctrl->file_name = file_name;
    err_code_ctrl->func_name = (char*)func_name;
    err_code_ctrl->line = line;
    err_code_ctrl->err_code = err_code;
    err_code_ctrl->info[0] = '\0';
    if (info != NULL) {
        strlcpy(err_code_ctrl->info, info, ERR_INFO_SIZE);
    }
}

void ErrCode_Clear()
{
    ERR_CODE_S *err_code_ctrl = &g_err_code;

    err_code_ctrl->file_name = NULL;
    err_code_ctrl->func_name = NULL;
    err_code_ctrl->line = 0;
    err_code_ctrl->err_code = 0;
    err_code_ctrl->info[0] = '\0';
}

char * ErrCode_GetFileName()
{
    if (g_err_code.file_name == NULL) {
        return "null";
    }

    return g_err_code.file_name;
}

int ErrCode_GetLine()
{
    return g_err_code.line;
}

int ErrCode_GetErrCode()
{
    return g_err_code.err_code;
}

char * ErrCode_GetInfo()
{
    return g_err_code.info;
}

char * ErrCode_Build(OUT char *buf, int buf_size)
{
    char *file = ErrCode_GetFileName();
    int line = ErrCode_GetLine();
    int code = ErrCode_GetErrCode();
    char *errinfo = ErrCode_GetInfo();
    int len = 0;

    if (file) {
        len = scnprintf(buf, buf_size,
                "Err: file:%s(%d):%d \r\n", file, line, code);
    }

    if (errinfo && errinfo[0]) {
        len += scnprintf(buf + len, buf_size-len, "ErrInfo: %s \r\n", errinfo);
    }

    return buf;
}

void ErrCode_Print()
{
    char *file = ErrCode_GetFileName();
    int line = ErrCode_GetLine();
    int code = ErrCode_GetErrCode();
    char *errinfo = ErrCode_GetInfo();

    if (file) {
        fprintf(stderr, "Err: file:%s(%d):%d \r\n", file, line, code);
    }

    if (errinfo && errinfo[0]) {
        fprintf(stderr, "ErrInfo: %s \r\n", errinfo);
    }
}

void ErrCode_FatalError(char *format, ...)
{
#define STD_BUF 1024
    char buf[STD_BUF+1];
    va_list ap;

    va_start(ap, format);
    vsnprintf(buf, STD_BUF, format, ap);
    va_end(ap);

    buf[STD_BUF] = '\0';

    fprintf(stderr, "ERROR: %s", buf);
    fprintf(stderr,"Fatal Error, Quitting..\n");

    exit(1);
}

