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

char * ErrCode_GetFileName()
{
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
        len = snprintf(buf, buf_size,
                "Err: file:%s(%d):%d\r\n", file, line, code);
    }

    if (errinfo && errinfo[0]) {
        len += snprintf(buf + len, buf_size-len, "ErrInfo: %s\r\n", errinfo);
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
        fprintf(stderr, "ErrInfo: file:%s(%d):%d\r\n", file, line, code);
    }

    if (errinfo && errinfo[0]) {
        fprintf(stderr, "ErrInfo: %s\r\n", errinfo);
    }
}

