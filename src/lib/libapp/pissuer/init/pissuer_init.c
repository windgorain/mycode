/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "../h/pissuer_func.h"

int PIssuer_Init()
{
    PIssuer_Core_Init();
    PIssuer_Comp_Init();

    return 0;
}

