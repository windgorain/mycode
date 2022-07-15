/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/sprintf_utl.h"

static FormatCompile_S g_test_Fc;

void func(void *buf, int a, char *b)
{
    BS_Format(&g_test_Fc, buf, a, b);
//    BS_Sprintf(buf, "test the sprintf time for a=%d and b=%s\n", a, b);
//    snprintf(buf, 256, "test the sprintf time for a=%d and b=%s\n", a, b);
}

int main()
{
    char buf[256];
    int i;

    BS_FormatCompile(&g_test_Fc, "test the sprintf time for a=%d and b=%s\n");

    for (i=0; i<10000000; i++) {
        func(buf, 1212121, "testtesttest");
    }

    return 0;
}
