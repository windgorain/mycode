/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "comp/comp_pissuer.h"

#include "../h/pissuer_func.h"

static COMP_PISSUER_S g_pissuer_comp = {
    .comp.comp_name = COMP_PISSUER_NAME,
    .pissuer_reg = PIssuer_Reg,
    .pissuer_unreg = PIssuer_UnReg,
    .pissuer_find = PIssuer_Find,
    .pissuer_set_filter = PIssuer_SetFilter,
    .pissuer_publish = PIssuer_Publish
};

int PIssuer_Comp_Init()
{
    COMP_Reg(&g_pissuer_comp.comp);
    return 0;
}

