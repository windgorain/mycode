/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "bs.h"
#include "utl/pci_utl.h"
#include "utl/pci_sim.h"
#include "utl/pci_vblk.h"

#define PCIE_SIM_PATH "/tmp/uds.sock"

#define PCIE_SIM_LOG_ERR(fmt, ...) PRINT_RED(fmt, ##__VA_ARGS__)
#define PCIE_SIM_LOG_READ(fmt, ...) PRINT_GREEN(fmt, ##__VA_ARGS__)
#define PCIE_SIM_LOG_WRITE(fmt, ...) PRINT_CYAN(fmt, ##__VA_ARGS__)
#define PCIE_SIM_LOG_SEND(fmt, ...) PRINT_YELLOW(fmt, ##__VA_ARGS__)
#define PCIE_SIM_LOG_RECV(fmt, ...) PRINT_PURPLE(fmt, ##__VA_ARGS__)

struct cosim_trans_hdr{
    uint8_t opcode;
    uint8_t sub_opcode;
    uint16_t flag;
    int32_t data_len;
    int64_t id;
    uint32_t seg_id;
    uint32_t reserve1;
    uint64_t context;
    uint64_t reserve[3];
}__attribute__((packed));

static char g_pcie_sim_recv_buf[1028*4];
static struct cosim_trans_hdr g_pcie_sim_send_hdr;

static int _pcie_sim_print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return 0;
}

static int _pcie_sim_server_send(void *sim, void *data, int len)
{
    char buf[128];
    PCIE_SIM_S *s = sim;

    PCIE_ChangeTlpOrder(data);

    PCIE_SIM_LOG_SEND("Send data len=%d", len);
    MEM_Sprint(data, len, buf, sizeof(buf));
    PCIE_SIM_LOG_SEND("%s", buf);

    int socket_fd = HANDLE_UINT(s->user_data);

    g_pcie_sim_send_hdr.data_len = len;

    send(socket_fd, &g_pcie_sim_send_hdr, sizeof(g_pcie_sim_send_hdr), 0);

    return send(socket_fd, data, len, 0);
}

static PCIE_SIM_S g_pcie_sim = {
    .tlp_send = _pcie_sim_server_send,
    .print_func = _pcie_sim_print,
};

static void _pcie_sim_server_print_ret_err(void)
{
    char info[1024];
    ErrCode_Build(info, sizeof(info));
    PCIE_SIM_LOG_ERR("%s", info);
}

static int _pcie_sim_server_process_tlp(void *tlp_msg, int msg_len)
{
    char buf[128];

    PCIE_SIM_LOG_RECV("Recv tlp len=%d", msg_len);
    MEM_Sprint(tlp_msg, msg_len, buf, sizeof(buf));
    PCIE_SIM_LOG_RECV("%s", buf);

    int ret = PCIE_TlpCheckAndChangeOrder(tlp_msg, msg_len);
    if (ret < 0) {
        _pcie_sim_server_print_ret_err();
        return ret;
    }

    ret = PCIE_SIM_TlpInput(&g_pcie_sim, tlp_msg, msg_len);
    if (ret < 0) {
        _pcie_sim_server_print_ret_err();
        return ret;
    }

    return 0;
}

CONSTRUCTOR(init) {
    PCI_Vblk_Init(&g_pcie_sim.dev);
}

int main(int argc, char **argv)
{
    int nrecv;
    struct sockaddr_un server;
    char *path = PCIE_SIM_PATH;
    struct cosim_trans_hdr hdr;

    if (argc >= 2) {
        path = argv[1];
    }

    unlink(path);

    int sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock < 0) {
        PCIE_SIM_LOG_ERR("opening stream socket failed");
        return -1;
    }

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, path);
    if (bind(sock, (struct sockaddr *) &server, sizeof(server))) {
        PCIE_SIM_LOG_ERR("binding stream socket failed");
        return -1;
    }

    listen(sock, 5);

    while (1) {
        struct sockaddr_un	un;
        unsigned int len = sizeof(un);

        int fd = accept(sock, (void*)&un, &len);
        if (fd < 0) {
            continue;
        }

        g_pcie_sim.user_data = UINT_HANDLE(fd);

        while (1) {
            nrecv = recv(fd, &hdr, sizeof(hdr), 0);
            if (nrecv <= 0) {
                printf("Peer closed \n");
                break;
            }

            nrecv = recv(fd, g_pcie_sim_recv_buf, sizeof(g_pcie_sim_recv_buf), 0);
            if (nrecv <= 0) {
                printf("Peer closed \n");
                break;
            }

            memcpy(&g_pcie_sim_send_hdr, &hdr, sizeof(hdr));

            _pcie_sim_server_process_tlp(g_pcie_sim_recv_buf, hdr.data_len);
        }

        close(fd);
    }

    unlink(path);

    return 0;
}

