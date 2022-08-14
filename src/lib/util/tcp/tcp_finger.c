#include "bs.h"
#include "utl/net.h"
#include "utl/map_utl.h"
#include "utl/ip_utl.h"
#include "utl/tcp_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/tcp_finger.h"

void _tcpfinger_free(void *data, VOID *pUserHandle)
{
    MEM_Free(data);
}

BS_STATUS TcpFinger_Init(TCP_FINGER_S *tcp_finger)
{
    tcp_finger->map = MAP_Create(0);
    if (NULL == tcp_finger->map) {
        RETURN(BS_NO_MEMORY);
    }

    return BS_OK;
}

void TcpFinger_Fin(TCP_FINGER_S *tcp_finger)
{
    if (NULL == tcp_finger) {
        return;
    }

    if (tcp_finger->map) {
        MAP_Destroy(tcp_finger->map, _tcpfinger_free, NULL);
    }

    return;
}

BS_STATUS TcpFinger_AddFinger(TCP_FINGER_S *tcp_finger,
        char *finger, char *os, int id)
{
    TCP_FINGER_NODE_S *node;
    int len = strlen(finger);

    node = MAP_Get(tcp_finger->map, finger, len);
    if (node) {
        TXT_Strlcat(node->os, ";", TCP_FINGER_MAX_OS_LEN + 1);
        TXT_Strlcat(node->os, os, TCP_FINGER_MAX_OS_LEN + 1);
        return 0;
    }

    node = MEM_ZMalloc(sizeof(TCP_FINGER_NODE_S));
    if (NULL == node) {
        RETURN(BS_NO_MEMORY);
    }

    TXT_Strlcpy(node->finger, finger, TCP_FINGER_MAX_FINGER_LEN + 1);
    TXT_Strlcpy(node->os, os, TCP_FINGER_MAX_OS_LEN + 1);
    node->id = id;

    int ret = MAP_Add(tcp_finger->map, node->finger, len, node, 0);
    if (BS_OK != ret) {
        MEM_Free(node);
        return ret;
    }

    return BS_OK;
}

/* WWWW:_MSS:TL:WS:S:N:D:T:F:LT:OS */
static void tcpfinger_FillIPFinger(VOID *ip, char *finger)
{
    IP_HEAD_S *iph = ip;
    char sz[5];

    sprintf(sz, "%02X", iph->ucTtl);
    memcpy(finger + 10, sz, 2);

    if (ntohs(iph->usOff) & IP_DF) {
        finger[20] = '1';
    }

    sprintf(sz, "%02X", iph->ucHLen * 4);
    memcpy(finger + 26, sz, 2);
}

/* WWWW:_MSS:TL:WS:S:N:D:T:F:LT:OS */
static void tcpfinger_FillTcpFinger(VOID *tcp, char *finger)
{
    TCP_HEAD_S *tcph = tcp;
    char sz[16];
    TCP_OPT_INFO_S stInfo;

    TCP_GetOptInfo(tcph, &stInfo);

    sprintf(sz, "%04X", ntohs(tcph->usWindow));
    memcpy(finger, sz, 4);

    if (stInfo.mss > 0) {
        sprintf(sz, "%04X", stInfo.mss);
        memcpy(finger + 5, sz, 4);
    }
    if (stInfo.wscale > 0) {
        sprintf(sz, "%02X", stInfo.wscale);
        memcpy(finger + 13, sz, 2);
    }
    if (stInfo.sack) {
        finger[16] = '1';
    }
    if (stInfo.nop_count) {
        finger[18] = '1';
    }
    if (stInfo.timestamp > 0) {
        finger[22] = '1';
    }
    if (tcph->ucFlag & TCP_FLAG_SYN) {
        finger[24] = 'S';
        if (tcph->ucFlag & TCP_FLAG_ACK) {
            finger[24] = 'A';
        }
    }
}

char * TcpFinger_BuildFinger(void *ipheader, void *tcpheader, char *finger)
{
    TcpFinger_DefaultFinger(finger);
    tcpfinger_FillIPFinger(ipheader, finger);
    tcpfinger_FillTcpFinger(tcpheader, finger);

    return finger;
}

TCP_FINGER_NODE_S * TcpFinger_Search(TCP_FINGER_S *tcp_finger, char *finger)
{
    return MAP_Get(tcp_finger->map, finger, strlen(finger));
}

TCP_FINGER_NODE_S * TcpFinger_Match(TCP_FINGER_S *tcp_finger,
        void *ipheader, void *tcpheader)
{
    char finger[TCP_FINGER_MAX_FINGER_LEN + 1];

    TcpFinger_DefaultFinger(finger);
    tcpfinger_FillIPFinger(ipheader, finger);
    tcpfinger_FillTcpFinger(tcpheader, finger);

    return TcpFinger_Search(tcp_finger, finger);
}

/* WWWW:_MSS:TL:WS:S:N:D:T:F:LT:OS */
static int _tcpfinger_ProcessFileLine(TCP_FINGER_S *tcp_finger,
        char *line, int id)
{
    char *finger;
    char *os;

    finger = TXT_StrimString(line, "\r\n\t ");
    if (*finger == 0) {
        RETURN(BS_ERR);
    }

    int len = strlen(finger);
    if (len <= 28) {
        RETURN(BS_ERR);
    }

    finger[28] = '\0';
    os = &finger[29];

    /* 添加精确指纹 */
    int ret = TcpFinger_AddFinger(tcp_finger, finger, os, id);
    if (ret < 0) {
        return ret;
    }

#if 0
    char vague_finger[28];
    char vague_os[TCP_FINGER_MAX_OS_LEN + 1];

    /* 忽略MSS  WWWW:*:TL:WS:S:N:D:T:F:LT:OS */
    memcpy(vague_finger, finger, 5);
    vague_finger[5] = '*';
    strcpy(vague_finger+6, finger+9);
    snprintf(vague_os, sizeof(vague_os), "%s <vague>", os);

    /* 添加模糊指纹 */
    ret = TcpFinger_AddFinger(tcp_finger, vague_finger, vague_os, id);
    if (0 != ret) {
        return ret;
    }
#endif

    return BS_OK;
}

BS_STATUS TcpFinger_ReadFingerFile(TCP_FINGER_S *tcp_finger, char *filepath, UINT base_id)
{
    FILE *fp;
    char line[256];
    int len;
    int id = base_id + 1;
    int ret;

    fp = fopen(filepath, "rb");
    if (NULL == fp) {
        return BS_CAN_NOT_OPEN;
    }

    while(1) {
        len = FILE_ReadLine(fp, line, sizeof(line), '\n');
        if (len <=0) {
            break;
        }

        ret = -1;
        if (len < sizeof(line)) {
            ret = _tcpfinger_ProcessFileLine(tcp_finger, line, id);
        }

        if (ret != 0) {
            printf("Error: Can't load tcpfp rule %d \r\n", id);
            ErrCode_Print();
        }

        id ++;
    }

    fclose(fp);

    return BS_OK;
}

void TcpFinger_DefaultFinger(char *finger)
{
   strcpy(finger, "0000:_MSS:TT:WS:0:0:0:0:F:LT");
}

