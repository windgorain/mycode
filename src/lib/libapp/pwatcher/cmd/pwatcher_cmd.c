/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "../h/pwatcher_def.h"
#include "../h/pwatcher_recver_cmd.h"
#include "../h/pwatcher_event.h"
#include "../h/pwatcher_link.h"
#include "../h/pwatcher_zone.h"

PLUG_API int PWatcherCmd_Save(HANDLE hFile)
{
    PWatcherRecver_CmdSave(hFile);
    PWatcherSession_CmdSave(hFile);
    PWatcherLink_CmdSave(hFile);
    PWatcherZone_Save(hFile);

    return 0;
}

