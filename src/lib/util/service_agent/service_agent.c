/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/service_agent.h"
#include "utl/socket_utl.h"
#include "utl/mypoll_utl.h"

int ServiceAgent_Init(SERVICE_AGENT_S *ctrl, void *mypller)
{
    Mem_Zero(ctrl, sizeof(SERVICE_AGENT_S));
    ctrl->my_poll = mypller;

    return 0;
}

void ServiceAgent_SetUd(SERVICE_AGENT_S *ctrl, void *ud)
{
    ctrl->ud = ud;
}

void * ServiceAgent_GetUd(SERVICE_AGENT_S *ctrl)
{
    return ctrl->ud;
}

int serviceagent_ListenTcp(unsigned short port)
{
    int fd;

    fd = Socket_Create(AF_INET, SOCK_STREAM);
    if (fd < 0) {
        return fd;
    }

    Socket_SetNoDelay(fd, 1);
    Socket_SetNoBlock(fd, 1);

    if (0 != Socket_Listen(fd, 0, htons(port), 5)) {
        Socket_Close(fd);
        return -1;
    }

    return fd;
}

static BS_WALK_RET_E serviceagent_InEvent(int fd, UINT event,
        USER_HANDLE_S *userhandle)
{
    SERVICE_AGENT_S *ctrl = userhandle->ahUserHandle[0];
    unsigned char data[1024];
    int len;

    if (event & MYPOLL_EVENT_IN) {
        len = recv(fd, data, sizeof(data) - 1, 0);
        if (len < 0) {
            return BS_WALK_CONTINUE;
        }

        if (len == 0) {
            if (ctrl->event_func) {
                ctrl->event_func(ctrl, fd, SERVICE_AGENT_EVENT_FREE);
            }
            MyPoll_Del(ctrl->my_poll, fd);
            Socket_Close(fd);
            return BS_WALK_CONTINUE;
        }

        ctrl->data_func(ctrl, fd, data, len);
    }

    return BS_WALK_CONTINUE;
}

static BS_WALK_RET_E serviceagent_AcceptEvent(int fd, UINT uiEvent,
        USER_HANDLE_S *userhandle)
{
    int accept_fd;
    SERVICE_AGENT_S *ctrl = userhandle->ahUserHandle[0];

    accept_fd = Socket_Accept(fd, NULL, NULL);
    if (accept_fd < 0) {
        return BS_WALK_CONTINUE;
    }

    MyPoll_SetEvent(ctrl->my_poll, accept_fd, MYPOLL_EVENT_IN,
            serviceagent_InEvent, userhandle);

    if (ctrl->event_func) {
        if (0 != ctrl->event_func(ctrl, accept_fd, SERVICE_AGENT_EVENT_NEW)) {
            MyPoll_Del(ctrl->my_poll, accept_fd);
            Socket_Close(accept_fd);
        }
    }

    return BS_WALK_CONTINUE;
}

/* 设置关联到连接上的用户数据 */
void ServiceAgent_SetFdUd(SERVICE_AGENT_S *ctrl, int fd, void *fd_ud)
{
    USER_HANDLE_S ud;

    ud.ahUserHandle[0] = ctrl;
    ud.ahUserHandle[1] = fd_ud;

    MyPoll_ModifyUserHandle(ctrl->my_poll, fd, &ud);
}

void * ServiceAgent_GetFdUd(SERVICE_AGENT_S *ctrl, int fd)
{
    USER_HANDLE_S *ud;

    ud = MyPoll_GetUserHandle(ctrl->my_poll, fd);
    if (! ud) {
        return NULL;
    }

    return ud->ahUserHandle[1];
}

int ServiceAgent_OpenTcp(SERVICE_AGENT_S *ctrl, USHORT port/* host order */)
{
    USER_HANDLE_S userhandle;

    if (ctrl->listen_fd > 0) {
        RETURN(BS_ALREADY_EXIST);
    }

    ctrl->listen_fd = serviceagent_ListenTcp(port);
    if (ctrl->listen_fd < 0) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    userhandle.ahUserHandle[0] = ctrl;
    MyPoll_SetEvent(ctrl->my_poll, ctrl->listen_fd, MYPOLL_EVENT_IN,
            serviceagent_AcceptEvent, &userhandle);

    return 0;
}

void ServiceAgent_SetDataOb(SERVICE_AGENT_S *ctrl, PF_SERVICE_AGENT_DATA_OB fn)
{
    ctrl->data_func = fn;
}

void ServiceAgent_SetEventOb(SERVICE_AGENT_S *ctrl, PF_SERVICE_AGENT_EV_OB fn)
{
    ctrl->event_func = fn;
}

void ServiceAgent_Close(SERVICE_AGENT_S *ctrl, int fd)
{
    if (ctrl->event_func) {
        ctrl->event_func(ctrl, fd, SERVICE_AGENT_EVENT_FREE);
    }

    MyPoll_Del(ctrl->my_poll, fd);
    Socket_Close(fd);
}

