/*================================================================
*   Created by LiXingang: 2018.11.14
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/subcmd_utl.h"
#include "ssldecoder_service.h"

static int ssldecoder_run(int argc, char **argv);

static SUB_CMD_NODE_S g_ssldecoder_subcmds[] = 
{
    {"run", ssldecoder_run},
    {NULL, NULL}
};

static void ssldecoder_help_run()
{
    printf("Usage: ssldecoder run [OPTIONS]\r\n");
    printf("Options:\r\n");
    printf("  -h,       Help\r\n");
    printf("  -p port,  Listen tcp port. Default port 443. If you want to close tcp service please use 0.\r\n");
    return;
}

static int ssldecoder_run(int argc, char **argv)
{
    int c;
    unsigned short port = 443;

    if (argc < 1) {
        ssldecoder_help_run();
        return -1;
    }

    while ((c = getopt(argc, argv, "hp:")) != -1) {
        switch (c) {
            case 'h':
                ssldecoder_help_run();
                return 0;
                break;
            case 'p':
                port = atol(optarg);
                break;
            default:
                printf("Unknown option -%c\r\n", c);
                ssldecoder_help_run();
                return -1;
        }
    }

    ssldecoder_service_init();

    if (port != 0) {
        ssldecoder_service_open_tcp(port);
    }

    ssldecoder_service_run();

    return 0;
}

int main(int argc, char **argv)
{
    return SUBCMD_Do(g_ssldecoder_subcmds, argc, argv);
}
