/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include <stdio.h>
#include <stdlib.h>

static void hail_helper(char *prog_name)
{
    printf("Usage: %s nummber \n", prog_name);
}

int main(int argc, char **argv)
{
    int num;
    int count = 0;

    if (argc <= 1) {
        hail_helper(argv[0]);
        return -1;
    }

    num = atoi(argv[1]);

    while (num > 1) {
        if (num & 0x1) {
            num = num * 3 + 1;
        } else {
            num = num / 2;
        }
        count ++;
    }

    printf("Count=%d \n", count);

    return 0;
}
