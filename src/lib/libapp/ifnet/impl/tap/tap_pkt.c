/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/tun_utl.h"
#include "comp/comp_if.h"
#include "comp/comp_poller.h"

#include "tap_if.h"

static void *g_tap_pkt_poller;

int TAP_PKT_Init()
{
    g_tap_pkt_poller = PollerComp_Get(NULL);
    if (NULL == g_tap_pkt_poller) {
        RETURN(BS_NOT_SUPPORT);
    }

    return 0;
}

static int tappkt_FdEvent(int fd, UINT uiEvent, USER_HANDLE_S *pstUserHandle)
{
    unsigned char data[2048];
    int len;
    MBUF_S *m;
    int id = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);

    len = TUN_Read(fd, data, sizeof(data));
    if (len <= 0) {
        return 0;
    }

    m = MBUF_CreateByCopyBuf(200, data, len , MBUF_DATA_DATA);
    if (! m) {
        return 0;
    }

    TAP_PhyInput(id, m);

    return 0;
}

int TAP_PKT_AddFd(int tap_fd, int id)
{
    USER_HANDLE_S user_handle;
    user_handle.ahUserHandle[0] = UINT_HANDLE(id);
    return MyPoll_SetEvent(PollerComp_GetMyPoll(g_tap_pkt_poller),
            tap_fd, MYPOLL_EVENT_IN,
            tappkt_FdEvent, &user_handle);
}

