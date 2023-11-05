/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "comp/comp_poller.h"
#include "../h/poller_core.h"


PLUG_API BS_STATUS POLLER_CMD_EnterIns(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    if (argv[0][0] != 'n') {
        if (NULL == POLLER_INS_Add(argv[1])) {
            EXEC_OutString("Can't create poller\r\n");
            RETURN(BS_ERR);
        }
    } else {
        POLLER_INS_DelByName(argv[2]);
    }

    return BS_OK;
}


PLUG_API BS_STATUS POLLER_CMD_Show(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    int id;
    char *name;

    EXEC_OutString(" id   name\r\n"
            "-----------------------------------------\r\n");

    for (id=0; id<POLLER_INS_MAX; id++) {
        name = POLLER_INS_GetName(id);
        if (name && (name[0] != 0)) {
            EXEC_OutInfo(" %-4d %-15s\r\n", id, name);
        }
    }

    return BS_OK;
}

PLUG_API BS_STATUS POLLER_CMD_Save(IN HANDLE hFile)
{
    int id;
    char *name;

    for (id=0; id<POLLER_INS_MAX; id++) {
        name = POLLER_INS_GetName(id);
        if (name && (name[0] != 0)) {
            CMD_EXP_OutputCmd(hFile, "poller %s", name);
        }
    }

    return BS_OK;
}

void POLLER_CMD_Init()
{
}
