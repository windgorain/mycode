/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/err_code.h"

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

