/*================================================================
*   Created by LiXingang: 2018.12.11
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/md5_utl.h"
#include "utl/data2hex_utl.h"

static void help()
{
    printf("Usage:\r\n");
    printf("md5tool string\r\n");
}

int main(int argc, char **argv)
{
    UCHAR data[MD5_LEN];
    CHAR str[MD5_LEN * 2 + 1];

    if (argc < 2) {
        help();
        return -1;
    }

    MD5_Create((void*)argv[1], strlen(argv[1]), data);
    DH_Data2HexString(data, MD5_LEN, str);

    printf("%s\r\n", str);

    return 0;
}
