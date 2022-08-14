/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/tun_utl.h"
#include "comp/comp_if.h"
#include "comp/comp_poller.h"

#include "tun_if.h"

static void *g_tun_pkt_poller;

int TUN_PKT_Init()
{
    g_tun_pkt_poller = PollerComp_Get(NULL);
    if (NULL == g_tun_pkt_poller) {
        RETURN(BS_NOT_SUPPORT);
    }

    return 0;
}

static BS_WALK_RET_E tunpkt_TunFdEvent(int fd, UINT uiEvent, USER_HANDLE_S *pstUserHandle)
{
    unsigned char data[2048];
    int len;
    MBUF_S *m;
    int id = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);

    len = TUN_Read(fd, data, sizeof(data));
    if (len <= 0) {
        return BS_WALK_CONTINUE;
    }

    m = MBUF_CreateByCopyBuf(200, data, len , MBUF_DATA_DATA);
    if (! m) {
        return BS_WALK_CONTINUE;
    }

    TUN_LinkInput(id, m);

    return BS_WALK_CONTINUE;
}

int TUN_PKT_AddFd(int tun_fd, int id)
{
    USER_HANDLE_S user_handle;
    user_handle.ahUserHandle[0] = UINT_HANDLE(id);
    return MyPoll_SetEvent(PollerComp_GetMyPoll(g_tun_pkt_poller),
            tun_fd, MYPOLL_EVENT_IN,
            tunpkt_TunFdEvent, &user_handle);
}

