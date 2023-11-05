/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/getopt2_utl.h"
#include "utl/jhash_utl.h"
#include "utl/subcmd_utl.h"



static int jhash_string(int argc, char **argv)
{
    char *string = NULL;
    unsigned int initval = 0;
    UINT hash;

    GETOPT2_NODE_S opts[] = {
        {'P', 0, "string", GETOPT2_V_STRING, &string, "string", 0},
        {'o', 'i', "initval", GETOPT2_V_U32, &initval, "jhash init val", 0},
        {0}
    };

	if (0 != GETOPT2_Parse(argc, argv, opts)) {
        GETOPT2_PrintHelp(opts);
        return -1;
    }

    hash = JHASH_GeneralBuffer(string, strlen(string), initval);

    printf("hash: %u \n", hash);

    return 0;
}

int main(int argc, char **argv)
{
    static SUB_CMD_NODE_S subcmds[] = {
        {"string", jhash_string},
        {NULL, NULL}
    };

    return SUBCMD_Do(subcmds, argc, argv);
}

