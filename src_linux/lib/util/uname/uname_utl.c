/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/uname_utl.h"
#include "utl/txt_utl.h"

unsigned int UNAME_GetKernelVersion()
{
    struct utsname buff;
    char *argv[3];
    int count;
    unsigned int a, b, c;

    if (UNAME_GetInfo(&buff) < 0) {
        return 0;
    }

    count = TXT_StrToToken(buff.release, ".", argv, 3);
    if (count < 3) {
        return 0;
    }

    a = TXT_Str2Ui(argv[0]);
    b = TXT_Str2Ui(argv[1]);
    c = TXT_Str2Ui(argv[2]);

    return KERNEL_VERSION(a, b, c);
}

int UNAME_GetInfo(OUT struct utsname *buff)
{
    return uname(buff);
}


