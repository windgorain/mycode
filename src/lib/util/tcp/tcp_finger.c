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
    tcp_finger->map = MAP_Create();
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

BS_STATUS TcpFinger_AddFinger(TCP_FINGER_S *tcp_finger, char *finger, char *os)
{
    TCP_FINGER_NODE_S *node;

    node = MEM_ZMalloc(sizeof(TCP_FINGER_NODE_S));
    if (NULL == node) {
        return BS_NO_MEMORY;
    }

    TXT_Strlcpy(node->finger, finger, TCP_FINGER_MAX_FINGER_LEN + 1);
    TXT_Strlcpy(node->os, os, TCP_FINGER_MAX_OS_LEN + 1);

    if (BS_OK != MAP_Add(tcp_finger->map, finger, strlen(finger), node)) {
        MEM_Free(node);
        return BS_ERR;
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
static void _tcpfinger_ProcessFileLine(TCP_FINGER_S *tcp_finger, char *line)
{
    char *finger;
    char vague_finger[28];
    char vague_os[TCP_FINGER_MAX_OS_LEN + 1];
    char *os;

    finger = TXT_StrimString(line, "\r\n\t ");
    if (*finger == 0) {
        return;
    }

    os = TXT_ReverseStrchr(line, ':');
    if (os == NULL) {
        return;
    }
    *os = '\0';
    os ++;

    if (strlen(finger) != 28) {
        return;
    }

    /* 添加精确指纹 */
    TcpFinger_AddFinger(tcp_finger, finger, os);

    /* 忽略MSS  WWWW:*:TL:WS:S:N:D:T:F:LT:OS */
    memcpy(vague_finger, finger, 5);
    vague_finger[5] = '*';
    strcpy(vague_finger+6, finger+9);
    snprintf(vague_os, sizeof(vague_os), "%s <vague>", os);

    /* 添加模糊指纹 */
    TcpFinger_AddFinger(tcp_finger, vague_finger, vague_os);
}

BS_STATUS TcpFinger_ReadFingerFile(TCP_FINGER_S *tcp_finger, char *filepath)
{
    FILE *fp;
    char line[256];
    int len;

    fp = fopen(filepath, "rb");
    if (NULL == fp) {
        return BS_CAN_NOT_OPEN;
    }

    do {
        len = FILE_ReadLine(fp, line, sizeof(line), '\n');
        if (len < sizeof(line)) {
            _tcpfinger_ProcessFileLine(tcp_finger, line);
        }
    } while(len > 0);

    fclose(fp);

    return BS_OK;
}

void TcpFinger_DefaultFinger(char *finger)
{
   strcpy(finger, "0000:_MSS:TT:WS:0:0:0:0:F:LT");
}

