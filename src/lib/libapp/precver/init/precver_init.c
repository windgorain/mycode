/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "comp/comp_precver.h"
#include "../h/precver_def.h"
#include "../h/precver_conf.h"
#include "../h/precver_worker.h"

int PRecver_Init()
{
    int ret = PRecver_Worker_Init();
    ret |= PRecver_Conf_Init();
    ret |= PRecver_Comp_Init();
    ret |= PRecver_Main_Init();

    return ret;
}
