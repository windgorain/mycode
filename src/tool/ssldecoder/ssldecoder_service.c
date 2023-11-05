/*================================================================
*   Created by LiXingang: 2018.11.14
*   Description: 
*
================================================================*/
#include "bs.h"
#ifdef IN_LINUX
#include <sys/timerfd.h>
#endif

#include "utl/vbuf_utl.h"
#include "utl/lstr_utl.h"
#include "utl/mypoll_utl.h"
#include "utl/socket_utl.h"
#include "utl/ssl_decode.h"
#include "ssldecoder_service.h"

typedef struct {
    DLL_NODE_S linknode; 

    time_t create_time;
    VBUF_S vbuf;
    int downfd;
}SSLPARSER_SERVICE_NODE_S;

static DLL_HEAD_S g_ssldecoder_service_node_list = DLL_HEAD_INIT_VALUE(&g_ssldecoder_service_node_list);
static MYPOLL_HANDLE g_ssldecoder_service_poller = NULL;

static SSLPARSER_SERVICE_NODE_S * _ssldecoder_service_alloc_node()
{
    SSLPARSER_SERVICE_NODE_S *node;

    node = MEM_ZMalloc(sizeof(SSLPARSER_SERVICE_NODE_S));
    if (node == NULL) {
        return NULL;
    }

    VBUF_Init(&node->vbuf);
    VBUF_ExpandTo(&node->vbuf, 4096);

    node->create_time = time(0);

    DLL_ADD(&g_ssldecoder_service_node_list, node);

    return node;
}

static void _ssldecoder_service_free_node(SSLPARSER_SERVICE_NODE_S *node)
{
    if (node->downfd > 0) {
        MyPoll_Del(g_ssldecoder_service_poller, node->downfd);
        Socket_Close(node->downfd);
    }

    DLL_DEL(&g_ssldecoder_service_node_list, node);

    VBUF_Finit(&node->vbuf);
    MEM_Free(node);
}

#ifdef IN_LINUX
static int _ssldecoder_service_timeout(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    char buf[128];
    SSLPARSER_SERVICE_NODE_S *node, *nodetmp;
    time_t now;

    if (read(iSocketId, buf, sizeof(buf)) <= 0) {
        BS_DBGASSERT(0);
    }

    now = time(0);

    DLL_SAFE_SCAN(&g_ssldecoder_service_node_list, node, nodetmp)  {
        if (now - node->create_time > 5) {
            _ssldecoder_service_free_node(node);
        }
    }

    return 0;
}
#endif

static int _ssldecoder_service_timerfd_init()
{
#ifdef IN_LINUX
    int timer_fd;
    struct itimerspec new_value;

    new_value.it_value.tv_sec = 1;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = 1;
    new_value.it_interval.tv_nsec = 0;

    timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd < 0) {
        printf("Can't create timerfd\r\n");
        return -1;
    }

    timerfd_settime(timer_fd, 0, &new_value, NULL);

    MyPoll_SetEvent(g_ssldecoder_service_poller, timer_fd, MYPOLL_EVENT_IN, _ssldecoder_service_timeout, NULL);
#endif

    return 0;
}

static void _ssldecoder_service_decoder_client_hello(void *data, int len)
{
    SSL_CLIENT_HELLO_INFO_S info;
    LSTR_S sni;
    char hostname[512];

    if (0 != SSLDecode_ParseClientHello(data, len, &info)) {
        printf("Can't parse client hello\r\n");
        return;
    }
    
    printf("SSL Version:%x\r\n", info.handshake_ssl_ver);

    if (NULL == SSLDecode_GetSNI(info.extensions, info.extensions_len, &sni)) {
        printf("Can't get sni\r\n");
        return;
    }

    if (sni.uiLen >= sizeof(hostname)) {
        printf("Hostname is bigger then %lu\r\n", sizeof(hostname) - 1);
    }

    memcpy(hostname, sni.pcData, sni.uiLen);
    hostname[sni.uiLen] = '\0';
    
    printf("Hostname: %s\r\n", hostname);

    return;
}

static void _ssldecoder_service_process_request(SSLPARSER_SERVICE_NODE_S *node)
{
    if (! SSLDecode_IsRecordComplete(VBUF_GetData(&node->vbuf), VBUF_GetDataLength(&node->vbuf))) {
        return;
    }

    _ssldecoder_service_decoder_client_hello(VBUF_GetData(&node->vbuf), VBUF_GetDataLength(&node->vbuf));

    _ssldecoder_service_free_node(node);

    return;
}

static int _ssldecoder_service_down_event(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    int len;
    SSLPARSER_SERVICE_NODE_S *node = pstUserHandle->ahUserHandle[0];

    if (uiEvent & MYPOLL_EVENT_ERR) {
        _ssldecoder_service_free_node(node);
        return 0;
    }

    if (uiEvent & MYPOLL_EVENT_IN) {
        if (VBUF_GetTailFreeLength(&node->vbuf) <= 0) {
            _ssldecoder_service_free_node(node);
            return 0;
        }
        len = Socket_Read(iSocketId, VBUF_GetTailFreeBuf(&node->vbuf), VBUF_GetTailFreeLength(&node->vbuf), 0);
        if (len <= 0) {
            if (len != SOCKET_E_AGAIN) {
                _ssldecoder_service_free_node(node);
            }
            return 0;
        }

        VBUF_AddDataLength(&node->vbuf, len);

        _ssldecoder_service_process_request(node);
    }

    return 0;
}

static int _ssldecoder_service_Accept(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    int socketid;
    SSLPARSER_SERVICE_NODE_S *node;
    USER_HANDLE_S user_handle;

    socketid = Socket_Accept(iSocketId, NULL, NULL);
    if (socketid < 0) {
        return 0;
    }

    node = _ssldecoder_service_alloc_node();
    if (node == NULL) {
        Socket_Close(socketid);
        return 0;
    }

    node->downfd = socketid;
    user_handle.ahUserHandle[0] = node;

    MyPoll_SetEvent(g_ssldecoder_service_poller, socketid, MYPOLL_EVENT_IN, _ssldecoder_service_down_event, &user_handle);

    return 0;
}

int ssldecoder_service_init()
{
    g_ssldecoder_service_poller = MyPoll_Create();
    if (NULL == g_ssldecoder_service_poller) {
        printf("Can't create poller\r\n");
        return -1;
    }

    return _ssldecoder_service_timerfd_init();
}

int ssldecoder_service_open_tcp(unsigned short port)
{
    int on = 1;
    int rc;
    int listenfd;
    struct sockaddr_in address;

    if (port == 0) {
        return -1;
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        return -1;
    }

    rc = setsockopt(listenfd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on));
    if (rc < 0) {
        close(listenfd);
        return -1;
    }

    Socket_SetNoBlock(listenfd, 1);

    memset(&address, 0, sizeof(struct sockaddr));
    
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        printf("Can't bind tcp port %d\r\n", port);
        close(listenfd);
        return -1;
    }

    if (listen(listenfd, 5) == -1) {
        close(listenfd);
        return -1;
    }

    MyPoll_SetEvent(g_ssldecoder_service_poller, listenfd, MYPOLL_EVENT_IN, _ssldecoder_service_Accept, NULL);
    
    return 0;
}

void ssldecoder_service_run()
{
    MyPoll_Run(g_ssldecoder_service_poller);
}
