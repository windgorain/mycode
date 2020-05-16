/*================================================================
*   Created：2018.10.04 LiXingang All rights reserved.
*   Description: 自动根据系统创建接口
*
================================================================*/
#include "bs.h"

#include "utl/local_info.h"

extern int AIF_Loaded();
extern int AIF_CfgLoaded();
extern int AIF_CmdReged0();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOADED:
            AIF_Loaded();
            break;
        case PLUG_STAGE_CMD_REGED0:
            AIF_CmdReged0();
            break;
        case PLUG_STAGE_CFG_LOADED:
            AIF_CfgLoaded();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN
