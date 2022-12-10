/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
*****************************************************************************/
#include "bs.h"

static void help(char **argv)
{
    printf("Usage: %s string \n", argv[0]);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        help(argv);
        return -1;
    }

    MEM_Print((void*)argv[1], strlen(argv[1]), NULL);

    return 0;
}

