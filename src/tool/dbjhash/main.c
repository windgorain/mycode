/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/getopt2_utl.h"
#include "utl/hash_calc.h"
#include "utl/subcmd_utl.h"

/* 计算字符串的dbjhash值 */
/* dbjhash string xxxx [-i initval] */
static int dbjhash_string(int argc, char **argv)
{
    char *string = NULL;
    UINT hash;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "string", 's', &string, "string", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    hash = DJBHash((void*)string, strlen(string));

    printf("hash: %u \n", hash);

    return 0;
}

int main(int argc, char **argv)
{
    static SUB_CMD_NODE_S subcmds[] = {
        {"string", dbjhash_string},
        {NULL, NULL}
    };

    return SUBCMD_Do(subcmds, argc, argv);
}

