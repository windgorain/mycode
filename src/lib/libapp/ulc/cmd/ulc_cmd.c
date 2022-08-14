/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "../h/ulc_def.h"
#include "../h/ulc_osbase.h"
#include "../h/ulc_map.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_prog.h"
#include "../h/ulc_loader.h"

/* load xdp-file %STRING [sec %STRING] */
int ULC_CMD_LoadXdpFile(int argc, char **argv)
{
    int ret;
    char *filename = argv[2];
    char *xdp_sec = NULL;

    if (argc >= 5) {
        xdp_sec = argv[4];
    }

    ret = ULC_LOADER_LoadXdp(filename, xdp_sec);
    if (ret < 0) {
        EXEC_OutInfo("Can't load file %s \r\n", filename);
    }

    return ret;
}

/* show map */
int ULC_CMD_ShowMap(int argc, char **argv)
{
    ULC_MAP_ShowMap();
    return 0;
}

/* show prog */
int ULC_CMD_ShowProg(int argc, char **argv)
{
    ULC_PROG_ShowProg();
    return 0;
}


