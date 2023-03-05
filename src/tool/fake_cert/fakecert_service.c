/*================================================================
*   Created by LiXingang: 2018.11.10
*   Description: 
*
================================================================*/
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>   
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include "bs.h"
#ifdef IN_LINUX
#include <sys/timerfd.h>
#endif
#include "utl/mem_utl.h"
#include "utl/mypoll_utl.h"
#include "utl/socket_utl.h"
#include "utl/ssl_utl.h"
#include "utl/time_utl.h"
#include "utl/timerfd_utl.h"
#include "utl/cjson.h"

#include "fakecert_service.h"
#include "fakecert_conf.h"
#include "fakecert_dnsnames.h"
#include "fakecert_lib.h"
#include "fakecert_acl.h"
#include "fakecert_log.h"

#define _FAKECERT_SERVICE_NODE_TIMEOUT_TIME 5 /*5s*/
#define CA_FILE "./ca/all.pem"

#define VERIFY_CERT_FLAG            0x0001
#define CERT_TYPE_SELF_SIGNED       0x0002
#define CERT_TYPE_TRUSTED           0x0004
#define CERT_TYPE_UNTRUSTED         0x0008
#define IS_VERIFY_CERT(flag)        (flag & VERIFY_CERT_FLAG)
#define IS_TRUSTED_CERT(flag)       (flag & CERT_TYPE_TRUSTED)
#define IS_SELF_SIGNED_CERT(flag)   (flag & CERT_TYPE_SELF_SIGNED)
#define IS_UNTRUSTED_CERT(flag)     (flag & CERT_TYPE_UNTRUSTED)

typedef struct {
    DLL_NODE_S linknode; /* must the first */

    time_t create_time;
    char buf[256];
    void *ssl;
    cJSON *json;
    char *hostname;
    unsigned short port; /* 主机序 */
    unsigned short flag; /*flag used to */
    int upfd;
    int downfd;
    UINT ip;
}FAKECERT_SERVICE_NODE_S;

static DLL_HEAD_S g_fakecert_service_node_list = DLL_HEAD_INIT_VALUE(&g_fakecert_service_node_list);
static MYPOLL_HANDLE g_fakecert_service_poller = NULL;
static void *g_fakecert_ssl_ctx = NULL;
static UINT g_fakecert_service_cert_flag = 0;

static void _fakecert_service_free_node(FAKECERT_SERVICE_NODE_S *node);

int fakecert_service_conf_init(IN CFF_HANDLE hCff)
{
    UINT value;

    g_fakecert_service_cert_flag  = 0;
    value = 0;
    CFF_GetPropAsUint(hCff, "cert", "verify", &value);
    if (value) {
        g_fakecert_service_cert_flag |= VERIFY_CERT_FLAG;
    }

    return 0;
}

#ifdef IN_LINUX
static BS_WALK_RET_E _fakecert_service_timeout(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    char buf[128];
    FAKECERT_SERVICE_NODE_S *node, *nodetmp;
    time_t now;

    if (read(iSocketId, buf, sizeof(buf)) <= 0) {
        BS_DBGASSERT(0);
    }

    fakecert_conf_init();

    now = TM_SecondsFromInit();

    DLL_SAFE_SCAN(&g_fakecert_service_node_list, node, nodetmp)  {
        if (now - node->create_time > _FAKECERT_SERVICE_NODE_TIMEOUT_TIME) {
            FAKECERT_LOG_PROCESS(("Request %s timeout\r\n", node->hostname));
            _fakecert_service_free_node(node);
        }
    }

    return BS_WALK_CONTINUE;
}
#endif

static int fakecert_service_timerfd_init()
{
#ifdef IN_LINUX
    int timer_fd;
    timer_fd = TimerFd_Create(1000, TFD_NONBLOCK);
    if (timer_fd < 0) {
        return -1;
    }

    MyPoll_SetEvent(g_fakecert_service_poller, timer_fd, MYPOLL_EVENT_IN, _fakecert_service_timeout, NULL);
#endif

    return 0;
}

int fakecert_service_init()
{
    g_fakecert_ssl_ctx = SSL_UTL_Ctx_Create(0, 0);
    if (g_fakecert_ssl_ctx == NULL) {
        printf("Can't create ssl ctx\r\n");
        return -1;
    }

    SSL_UTL_Ctx_LoadVerifyLocations(g_fakecert_ssl_ctx, CA_FILE, NULL);

    g_fakecert_service_poller = MyPoll_Create();
    if (NULL == g_fakecert_service_poller) {
        printf("Can't create poller\r\n");
        return -1;
    }

    return fakecert_service_timerfd_init();
}

static FAKECERT_SERVICE_NODE_S * _fakecert_service_alloc_node()
{
    FAKECERT_SERVICE_NODE_S *node;

    node = MEM_ZMalloc(sizeof(FAKECERT_SERVICE_NODE_S));
    if (node == NULL) {
        return NULL;
    }

    node->create_time = TM_SecondsFromInit();
    if (g_fakecert_service_cert_flag & VERIFY_CERT_FLAG) {
        node->flag = VERIFY_CERT_FLAG;
    }

    DLL_ADD(&g_fakecert_service_node_list, node);

    return node;
}

static void _fakecert_service_free_node(FAKECERT_SERVICE_NODE_S *node)
{
    if (node->upfd > 0) {
        MyPoll_Del(g_fakecert_service_poller, node->upfd);
        Socket_Close(node->upfd);
    }

    if (node->downfd > 0) {
        MyPoll_Del(g_fakecert_service_poller, node->downfd);
        Socket_Close(node->downfd);
    }

    if (NULL != node->ssl) {
        SSL_UTL_Free(node->ssl);
    }
    
    if (NULL != node->json) {
        cJSON_Delete(node->json);
    }

    DLL_DEL(&g_fakecert_service_node_list, node);

    MEM_Free(node);
}

char const* cert_verifystrerror(int err)
{
    switch(err)
    {
        case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
            return "UNABLE_TO_DECRYPT_CERT_SIGNATURE";

        case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
            return "UNABLE_TO_DECRYPT_CRL_SIGNATURE";

        case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
            return "UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY";

        case X509_V_ERR_CERT_SIGNATURE_FAILURE:
            return "CERT_SIGNATURE_FAILURE";

        case X509_V_ERR_CRL_SIGNATURE_FAILURE:
            return "CRL_SIGNATURE_FAILURE";

        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
            return "ERROR_IN_CERT_NOT_BEFORE_FIELD";

        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
            return "ERROR_IN_CERT_NOT_AFTER_FIELD";

        case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
            return "ERROR_IN_CRL_LAST_UPDATE_FIELD";

        case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
            return "ERROR_IN_CRL_NEXT_UPDATE_FIELD";

        case X509_V_ERR_CERT_NOT_YET_VALID:
            return "CERT_NOT_YET_VALID";

        case X509_V_ERR_CERT_HAS_EXPIRED:
            return "CERT_HAS_EXPIRED";

        case X509_V_ERR_OUT_OF_MEM:
            return "OUT_OF_MEM";

        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
            return "UNABLE_TO_GET_ISSUER_CERT";

        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
            return "UNABLE_TO_GET_ISSUER_CERT_LOCALLY";

        case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
            return "UNABLE_TO_VERIFY_LEAF_SIGNATURE";

        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
            return "DEPTH_ZERO_SELF_SIGNED_CERT";

        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            return "SELF_SIGNED_CERT_IN_CHAIN";

        case X509_V_ERR_CERT_CHAIN_TOO_LONG:
            return "CERT_CHAIN_TOO_LONG";

        case X509_V_ERR_CERT_REVOKED:
            return "CERT_REVOKED";

        case X509_V_ERR_INVALID_CA:
            return "INVALID_CA";

        case X509_V_ERR_PATH_LENGTH_EXCEEDED:
            return "PATH_LENGTH_EXCEEDED";

        case X509_V_ERR_INVALID_PURPOSE:
            return "INVALID_PURPOSE";

        case X509_V_ERR_CERT_UNTRUSTED:
            return "CERT_UNTRUSTED";

        case X509_V_ERR_CERT_REJECTED:
            return "CERT_REJECTED";

        case X509_V_ERR_UNABLE_TO_GET_CRL:
            return "UNABLE_TO_GET_CRL";

        case X509_V_ERR_CRL_NOT_YET_VALID:
            return "CRL_NOT_YET_VALID";

        case X509_V_ERR_CRL_HAS_EXPIRED:
            return "CRL_HAS_EXPIRED";
    }

    return "Unknown verify error";
}

static inline int get_cert_by_type(char *data, int data_len, unsigned short type, char *certname)
{
    char *certpath = "";

    if (IS_TRUSTED_CERT(type)) {
        certpath="trusted";
    } else if (IS_UNTRUSTED_CERT(type)) {
        certpath="untrusted";
    } else if (IS_SELF_SIGNED_CERT(type)) {
        certpath="self";
    }

    int len = snprintf(data, data_len, "%s/%s\r\n", certpath, certname);
    data[len] = 0;

    return 1;
}

static void _fakecert_service_up_done(FAKECERT_SERVICE_NODE_S *node)
{
    char data[512];
    X509 *cert;
    char *certname;

    if (IS_VERIFY_CERT(node->flag)) {
        int ret = SSL_get_verify_result(node->ssl);
        switch (ret) {
            case X509_V_OK:
                node->flag |= CERT_TYPE_TRUSTED;
                break;
            case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
                node->flag |= CERT_TYPE_SELF_SIGNED;
                break;
            case X509_V_ERR_CERT_UNTRUSTED:
                node->flag |= CERT_TYPE_UNTRUSTED;
                break;
            default:
                FAKECERT_LOG_ERROR(("SSL verify %s failed: %s(%d)\r\n", node->hostname, cert_verifystrerror(ret), ret));
                return;
        }
    } else {
        node->flag |= CERT_TYPE_TRUSTED;
    }

    cert = SSL_get_peer_certificate(node->ssl);
    if (cert == NULL 
            || (X509_cmp_current_time(X509_get_notBefore((X509*)cert)) >= 0) 
            || (X509_cmp_current_time(X509_get_notAfter((X509*)cert)) <= 0)) {
        X509_free(cert);
        FAKECERT_LOG_ERROR(("Can't get %s cert or cert is expired\r\n", node->hostname));
        return;
    }

    certname = fakecert_build_by_cert(cert, node->hostname);
    X509_free(cert);

    if (certname == NULL) {
        FAKECERT_LOG_ERROR(("Build %s cert failed\r\n", node->hostname));
        return;
    }

    get_cert_by_type(data, sizeof(data), node->flag, certname);

    FAKECERT_LOG_PROCESS(("Send response for %s: %s\r\n", node->hostname, data));

    Socket_Write(node->downfd, data, strlen(data), 0);
}

static BS_WALK_RET_E _fakecert_service_up_event(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    FAKECERT_SERVICE_NODE_S *node = pstUserHandle->ahUserHandle[0];
    int iRet;

    if(uiEvent & MYPOLL_EVENT_ERR) {
        FAKECERT_LOG_EVENT(("Connection %s tcp error\r\n", node->hostname));
        _fakecert_service_free_node(node);
        return BS_WALK_CONTINUE;
    }

    iRet = SSL_UTL_Connect(node->ssl);
    switch (iRet)
    {
        case SSL_UTL_E_NONE:
            _fakecert_service_up_done(node);
            _fakecert_service_free_node(node);
            break;

        case SSL_UTL_E_WANT_READ:
            MyPoll_ModifyEvent(g_fakecert_service_poller, node->upfd, MYPOLL_EVENT_IN);
            break;

        case SSL_UTL_E_WANT_WRITE:
            MyPoll_ModifyEvent(g_fakecert_service_poller, node->upfd, MYPOLL_EVENT_OUT);
            break;

        default:
            FAKECERT_LOG_ERROR(("Connection %s ssl failed\r\n", node->hostname));
            _fakecert_service_free_node(node);
            break;
    }

    return BS_WALK_CONTINUE;
}

int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    if(!preverify_ok)
    {
        char buf[256];
        X509 *err_cert;
        int err, depth;

        err_cert = X509_STORE_CTX_get_current_cert(ctx);
        err = X509_STORE_CTX_get_error(ctx);
        depth = X509_STORE_CTX_get_error_depth(ctx);
        X509_STORE_CTX_get_ex_data(ctx,
                SSL_get_ex_data_X509_STORE_CTX_idx());
        X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

        fprintf(stderr, "Verify error: %s(%d)\n",
                X509_verify_cert_error_string(err), err);
        fprintf(stderr, " - depth=%d\n", depth);
        fprintf(stderr, " - sub  =\"%s\"\n", buf);
    }

    return preverify_ok;
}

static int _fakecert_service_connect_server(FAKECERT_SERVICE_NODE_S *node, char *hostname, UINT ip_addr)
{
    USER_HANDLE_S user_handle;
    UINT ip  = 0;
    int ret;

    if (ip_addr == 0) {
        ip = Socket_NameToIpHost(hostname);
    } else {
        ip = ip_addr;
    }

    if (ip == 0) {
        return -1;
    }

    node->ssl = SSL_UTL_New(g_fakecert_ssl_ctx);
    if (NULL == node->ssl) {
        return -1;
    }

    if (IS_VERIFY_CERT(node->flag)) {
        SSL_set_verify(node->ssl, SSL_VERIFY_PEER, verify_callback);
    }

    node->upfd = Socket_Create(AF_INET, SOCK_STREAM);
    if (node->upfd < 0) {
        return -1;
    }

    Socket_SetNoBlock(node->upfd, 1);
    SSL_UTL_SetFd(node->ssl, node->upfd);

    if (hostname != 0) {
        SSL_set_tlsext_host_name(node->ssl, hostname);
    }

    user_handle.ahUserHandle[0] = node;

    ret = Socket_Connect(node->upfd, ip, 443);
    if ((ret < 0) && (ret != SOCKET_E_AGAIN)) {
        return -1;
    }

    MyPoll_SetEvent(g_fakecert_service_poller, node->upfd, MYPOLL_EVENT_OUT, _fakecert_service_up_event, &user_handle);
    return 0;
}

static int _fakecert_service_process_json(FAKECERT_SERVICE_NODE_S *node, char *request)
{
    cJSON *pstNode;

    node->json = cJSON_Parse(request);
    if (NULL == node->json) {
        return -1;
    }

    pstNode = cJSON_GetObjectItem(node->json, "hostname");
    if ((pstNode == NULL) || (pstNode->valuestring == NULL)) {
        return -1;
    }
    node->hostname = pstNode->valuestring;

    node->port = 443;
    pstNode = cJSON_GetObjectItem(node->json, "port");
    if ((pstNode != NULL) && (pstNode->valueint != 0)) {
        node->port = pstNode->valueint;
    }

    node->ip = 0;
    pstNode = cJSON_GetObjectItem(node->json, "ip");
    if ((pstNode != NULL) && (pstNode->valuestring != 0)) {
        node->ip = ntohl(inet_addr(pstNode->valuestring));
    }

    return 0;
}

static void _fakecert_service_process_request(FAKECERT_SERVICE_NODE_S *node, char *request)
{
    char *certname;
    char buf[256];

    FAKECERT_LOG_PROCESS(("Recv request %s\r\n", request));

    if (_fakecert_service_process_json(node, request) < 0) {
        FAKECERT_LOG_ERROR(("Parse request error, request: %s\r\n", request));
        _fakecert_service_free_node(node);
        return;
    }

    if (! fakecert_acl_is_permit(node->hostname)) {
        FAKECERT_LOG_PROCESS(("Acl deny %s\r\n", node->hostname));
        _fakecert_service_free_node(node);
        return;
    }

    certname = fakecert_dnsnames_find(node->hostname);
    if (NULL != certname) {
        get_cert_by_type(buf, sizeof(buf), node->flag, certname);
        Socket_Write(node->downfd, buf, strlen(buf), 0);
        FAKECERT_LOG_PROCESS(("Send response for %s : %s\r\n", node->hostname, buf));
        _fakecert_service_free_node(node);
        return;
    }

    if (0 != _fakecert_service_connect_server(node, node->hostname, node->ip)) {
        FAKECERT_LOG_ERROR(("Connect %s failed\r\n", node->hostname));
        _fakecert_service_free_node(node);
        return;
    }

    return;
}

static BS_WALK_RET_E _fakecert_service_down_event(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    int len;
    int have_len;
    FAKECERT_SERVICE_NODE_S *node = pstUserHandle->ahUserHandle[0];

    if (uiEvent & MYPOLL_EVENT_ERR) {
        _fakecert_service_free_node(node);
        return BS_WALK_CONTINUE;
    }

    if (uiEvent & MYPOLL_EVENT_IN) {
        have_len = strlen(node->buf);
        len = Socket_Read(iSocketId, node->buf + have_len, (sizeof(node->buf) - 1) - have_len, 0);
        if (len <= 0) {
            if (len != SOCKET_E_AGAIN) {
                _fakecert_service_free_node(node);
            }
            return BS_WALK_CONTINUE;
        }

        node->buf[have_len + len] = 0;
        if (strstr(node->buf, "\r\n") != NULL) {
            node->buf[have_len + len -2] = 0;
            MyPoll_DelEvent(g_fakecert_service_poller, iSocketId, MYPOLL_EVENT_IN);
            _fakecert_service_process_request(node, node->buf);
        }
    }

    return BS_WALK_CONTINUE;
}

static BS_WALK_RET_E _fakecert_service_Accept(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    int socketid;
    FAKECERT_SERVICE_NODE_S *node;
    USER_HANDLE_S user_handle;

    socketid = Socket_Accept(iSocketId, NULL, NULL);
    if (socketid < 0) {
        return BS_WALK_CONTINUE;
    }

    node = _fakecert_service_alloc_node();
    if (node == NULL) {
        Socket_Close(socketid);
        return BS_WALK_CONTINUE;
    }

    node->downfd = socketid;
    user_handle.ahUserHandle[0] = node;

    MyPoll_SetEvent(g_fakecert_service_poller, socketid, MYPOLL_EVENT_IN, _fakecert_service_down_event, &user_handle);

    return BS_WALK_CONTINUE;
}

#ifdef IN_UNIXLIKE
int fakecert_service_open_unix(char *path)
{
    int on = 1;
    int rc;
    int listenfd;
    struct sockaddr_un my_addr;

    listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("Can't create unix socket\r\n");
        return -1;
    }

    rc = setsockopt(listenfd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on));
    if (rc < 0) {
        printf("Can't set unix socket to reuse addr\r\n");
        close(listenfd);
        return -1;
    }

    Socket_SetNoBlock(listenfd, 1);

    memset(&my_addr, 0, sizeof(struct sockaddr_un));
    my_addr.sun_family = AF_UNIX;
    strncpy(my_addr.sun_path, path, sizeof(my_addr.sun_path) - 1);
    unlink(path);
    if (bind(listenfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_un)) == -1) {
        printf("Can't bin unix socket\r\n");
        close(listenfd);
        return -1;
    }
    chmod(path, 0777);

    if (listen(listenfd, 5) == -1) {
        printf("Can't listen unix socket\r\n");
        close(listenfd);
        return -1;
    }

    MyPoll_SetEvent(g_fakecert_service_poller, listenfd, MYPOLL_EVENT_IN, _fakecert_service_Accept, NULL);

    return 0;
}
#endif

int fakecert_service_open_tcp(unsigned short port)
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
    /* Clear structure */
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

    MyPoll_SetEvent(g_fakecert_service_poller, listenfd, MYPOLL_EVENT_IN, _fakecert_service_Accept, NULL);
    
    return 0;
}

void fakecert_service_run()
{
    MyPoll_Run(g_fakecert_service_poller);
}
