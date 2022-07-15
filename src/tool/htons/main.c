/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"

static void help()
{
    printf("Usage: htons number");
}

int main(int argc, char **argv)
{
    if (argc <= 1) {
        help();
        return 0;
    }

    USHORT num = TXT_Str2Ui(argv[1]);
    USHORT num1 = htons(num);

    printf("num:%u, htons:%u \r\n", num, num1);

    return 0;
}
