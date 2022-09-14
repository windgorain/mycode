/*================================================================
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2017-4-23
* Description: 
* History:     
================================================================*/
#include "bs.h"
#include "utl/netlink_utl.h"

#define GENLMSG_DATA(glh)	((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define NLA_DATA(na)		((void *)((char*)(na) + NLA_HDRLEN))

struct netlink_reply_st {
    struct nlmsghdr hdr;
    struct nlmsgerr err;
};

static int _netlink_init_gen_family_id(int nl_fd, char *nl_gen_name)
{
    int nl_family_id = -1;  
    int len;
    struct sockaddr_nl nl_address;  
    struct nlattr *nl_na;  
    struct {
        struct nlmsghdr n;
        struct genlmsghdr g;
        char buf[256];
    }nl_request_msg, nl_response_msg;

    memset(&nl_address, 0, sizeof(nl_address));  
    nl_address.nl_family = AF_NETLINK;  
    nl_address.nl_groups = 0;  
    if (bind(nl_fd, (struct sockaddr *) &nl_address, sizeof(nl_address)) < 0) {  
        return -1;
    }  

    nl_request_msg.n.nlmsg_type = GENL_ID_CTRL;
    nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;  
    nl_request_msg.n.nlmsg_seq = 0;  
    nl_request_msg.n.nlmsg_pid = getpid();
    nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);  
    nl_request_msg.g.cmd = CTRL_CMD_GETFAMILY;  
    nl_request_msg.g.version = 0x1;  

    nl_na = (struct nlattr *) GENLMSG_DATA(&nl_request_msg);

    nl_na->nla_type = CTRL_ATTR_FAMILY_NAME;  
    nl_na->nla_len = strlen(nl_gen_name) + 1 + NLA_HDRLEN;  
    strcpy(NLA_DATA(nl_na), nl_gen_name);

    nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);  

    memset(&nl_address, 0, sizeof(nl_address));  
    nl_address.nl_family = AF_NETLINK;  

    len= sendto(nl_fd, (char *) &nl_request_msg, nl_request_msg.n.nlmsg_len,  
            0, (struct sockaddr *) &nl_address, sizeof(nl_address));  
    if (len != nl_request_msg.n.nlmsg_len) {  
        return -1; 
    }  

    len= recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
    if (len < 0) {
        return -1;
    }

    if (!NLMSG_OK((&nl_response_msg.n), len)) {  
        return -1; 
    }

    if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) {
        return -1; 
    }  

    //解析出attribute中的family id  
    nl_na = (struct nlattr *) GENLMSG_DATA(&nl_response_msg);  
    nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));  
    if (nl_na->nla_type == CTRL_ATTR_FAMILY_ID) {  
        nl_family_id = *(__u16 *) NLA_DATA(nl_na);
    } 

    if (nl_family_id < 0) {
        return -1; 
    }

    return nl_family_id; 
}

static int _netlink_create_socket()
{
    int nl_fd;
    struct timeval timeout = {5, 0};

    nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);  
    if (nl_fd < 0) {  
        return -1;
    }

    (void)setsockopt(nl_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

    return nl_fd;
}

void NetLink_Init(NETLINK_S *nl)
{
    nl->nl_fd = -1;
    nl->gen_family_id = -1;
}

int NetLink_Open(NETLINK_S *nl, char *nl_gen_name)
{
    if (nl->nl_fd < 0) {
        nl->nl_fd = _netlink_create_socket();
        if (nl->nl_fd < 0) {
            return -1;
        }
    }

    if (nl->gen_family_id < 0) {
        nl->gen_family_id = _netlink_init_gen_family_id(nl->nl_fd, nl_gen_name);
        if (nl->gen_family_id < 0) {
            return -1;
        }
    }

    return 0;
}

void NetLink_Close(NETLINK_S *nl)
{
    if (nl->nl_fd >= 0) {
        close(nl->nl_fd);
        nl->nl_fd = -1;
    }
}

int NetLink_SendMsg(NETLINK_S *nl, unsigned int msg_type, void *data, int datalen, void *recv_buf)
{
    unsigned int msg_len;
    NETLINK_MSG_S *message;
	struct sockaddr_nl 	local;
	struct sockaddr_nl 	kpeer;
	struct netlink_reply_st reply;
	socklen_t 		kpeerlen;
	int 			rcvlen = -1;	

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_pid = getpid();
	local.nl_groups = 0;

    memset(&kpeer, 0, sizeof(kpeer));
    kpeer.nl_family = AF_NETLINK;

    msg_len = datalen + sizeof(NETLINK_MSG_S);
    message = (void*)malloc(msg_len);
    if (message == NULL) {
        return -1;
    }

    memset(message, 0, msg_len);
    message->hdr.nlmsg_flags = NLM_F_REQUEST;
    message->hdr.nlmsg_pid = local.nl_pid;
    message->hdr.nlmsg_len = msg_len;
    message->hdr.nlmsg_type = nl->gen_family_id;
    message->g.cmd = NETLINK_GEN_C_CMD;
    message->msg_type = msg_type;
    message->reply_ptr = recv_buf;

    memcpy(message->data, data, datalen);

    if( sendto(nl->nl_fd, message, message->hdr.nlmsg_len, 0, (struct sockaddr*)&kpeer, sizeof(kpeer))>0 ) {
        kpeerlen = sizeof(struct sockaddr_nl);
        rcvlen = recvfrom(nl->nl_fd, &reply, sizeof(struct netlink_reply_st), 0, (struct sockaddr*)&kpeer, &kpeerlen);
    }

    free(message);

    if(rcvlen < 0){	
        return -1;
    }

    return reply.err.error;
}

int NetLink_Do(PF_NETLINK_DO do_func, int cmd, void *data, int data_len)
{
    NETLINK_S nl;
    int ret;

    NetLink_Init(&nl);

    ret = NetLink_Open(&nl, NETLINK_GEN_NAME);
    if (ret < 0) {
        return -1;
    }

    ret = do_func(&nl, cmd, data, data_len);

    NetLink_Close(&nl);

    return ret;
}

