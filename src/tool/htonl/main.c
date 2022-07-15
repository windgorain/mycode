/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"

static void help()
{
    printf("Usage: htonl number");
}

int main(int argc, char **argv)
{
    if (argc <= 1) {
        help();
        return 0;
    }

    UINT num = TXT_Str2Ui(argv[1]);
    UINT num1 = htonl(num);

    printf("num:%u, htonl:%u \r\n", num, num1);

    return 0;
}

