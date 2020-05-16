/*================================================================
*   Created by LiXingang: 2019.01.04
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/signal_utl.h"

#ifdef IN_UNIXLIKE

static DLL_HEAD_S g_signal_handler_chain = DLL_HEAD_INIT_VALUE(&g_signal_handler_chain);

static void shc_notify(int sig)
{
    SIGNAL_HANDLER_S *node;
 
    DLL_SCAN(&g_signal_handler_chain, node) {
        node->pfHandler(sig, node->user_data);
    }
}

void SHC_Reg(int sig, IN SIGNAL_HANDLER_S *node)
{
    DLL_ADD(&g_signal_handler_chain, node);
}

void SHC_Unreg(IN SIGNAL_HANDLER_S *node)
{
    DLL_DEL(&g_signal_handler_chain, node);
}

void * SHC_Watch(int sig)
{
    return SIGNAL_Set(sig, 0, shc_notify);
}

#endif

