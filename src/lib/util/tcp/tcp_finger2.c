/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/net.h"
#include "utl/kv_utl.h"
#include "utl/map_utl.h"
#include "utl/ip_utl.h"
#include "utl/tcp_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/tcp_finger2.h"

void _tcpfinger2_free(void *data, VOID *pUserHandle)
{
    MEM_Free(data);
}

BS_STATUS TcpFinger2_Init(TCP_FINGER2_S *tcp_finger)
{
    tcp_finger->map = MAP_HashCreate(0);
    if (NULL == tcp_finger->map) {
        RETURN(BS_NO_MEMORY);
    }

    return BS_OK;
}

void TcpFinger2_Fin(TCP_FINGER2_S *tcp_finger)
{
    if (NULL == tcp_finger) {
        return;
    }

    if (tcp_finger->map) {
        MAP_Destroy(tcp_finger->map, _tcpfinger2_free, NULL);
    }

    return;
}

BS_STATUS TcpFinger2_AddFinger(TCP_FINGER2_S *tcp_finger, char *finger, char *os, int id)
{
    TCP_FINGER2_NODE_S *node;

    node = MEM_ZMalloc(sizeof(TCP_FINGER2_NODE_S));
    if (NULL == node) {
        return BS_NO_MEMORY;
    }

    TXT_Strlcpy(node->finger, finger, TCP_FINGER2_MAX_FINGER_LEN + 1);
    TXT_Strlcpy(node->os, os, TCP_FINGER2_MAX_OS_LEN + 1);
    node->id = id;

    if (BS_OK != MAP_Add(tcp_finger->map, node->finger, strlen(finger), node, 0)) {
        MEM_Free(node);
        return BS_ERR;
    }

    return BS_OK;
}

static void _tcpfinger2_FillTcpFinger(UCHAR *data, int head_len, char *finger)
{
    int offset;
    char buf[1024];
    char *ptr = buf;
    UCHAR kind;

    ptr += sprintf(ptr, "(%02x%02x)", data[14], data[15]);
    offset = 20;
    while (offset < head_len) {
        kind = data[offset];
        if ((kind == 0) || (kind == 1)) {
            ptr += sprintf(ptr, "(%02x)", kind);
            offset += 1;
        } else if ((kind == 2) || (kind == 3)) {
            ptr += sprintf(ptr, "(%02x", kind);
            UINT length = data[offset+1];
            UINT i;
            for (i=0; i<=length; i++) {
                ptr += sprintf(ptr, "%02x", data[offset+i+1]);
            }
            ptr += sprintf(ptr, ")");
            offset += length;
        } else {
            ptr += sprintf(ptr, "(%02x)", kind);
            offset += data[offset+1];
        }
    }

    strlcpy(finger, buf, TCP_FINGER2_MAX_FINGER_LEN + 1);
}

char * TcpFinger2_BuildFinger(TCP_HEAD_S *tcpheader, char *finger)
{
    _tcpfinger2_FillTcpFinger((void*)tcpheader, TCP_HEAD_LEN(tcpheader), finger);

    return finger;
}

TCP_FINGER2_NODE_S * TcpFinger2_Search(TCP_FINGER2_S *tcp_finger, char *finger)
{
    return MAP_Get(tcp_finger->map, finger, strlen(finger));
}

TCP_FINGER2_NODE_S * TcpFinger2_Match(TCP_FINGER2_S *tcp_finger, TCP_HEAD_S *tcpheader)
{
    char finger[TCP_FINGER2_MAX_FINGER_LEN + 1];

    _tcpfinger2_FillTcpFinger((void*)tcpheader, TCP_HEAD_LEN(tcpheader), finger);

    return TcpFinger2_Search(tcp_finger, finger);
}

static int _tcpfinger2_ProcessFileLine(TCP_FINGER2_S *tcp_finger, KV_HANDLE hKv, int id)
{
    char *finger = KV_GetKeyValue(hKv, "\"str_repr\"");
    char *os = KV_GetKeyValue(hKv, "\"os_info\"");

    if ((! finger) || (! os)) {
        return BS_ERR;
    }

    finger = TXT_Strim(finger);
    os = TXT_Strim(os);

    int finger_len = strlen(finger);
    if (finger_len < 2) {
        return BS_ERR;
    }

    finger ++;
    finger[finger_len-2] = '\0';

    return TcpFinger2_AddFinger(tcp_finger, finger, os, id);
}

BS_STATUS TcpFinger2_ReadFingerFile(TCP_FINGER2_S *tcp_finger, char *filepath, UINT base_id)
{
    FILE *fp;
    int len;
    int id = base_id + 1;
    KV_HANDLE hKv;
    char line[2048];
    LSTR_S lstr;
    int ret;

    hKv = KV_Create(0);
    if (! hKv) {
        RETURN(BS_NO_MEMORY);
    }

    fp = fopen(filepath, "rb");
    if (NULL == fp) {
        KV_Destory(hKv);
        RETURN(BS_CAN_NOT_OPEN);
    }

    lstr.pcData = line;
    lstr.pcData ++;

    while(1) {
        len = FILE_ReadLine(fp, line, sizeof(line), '\n');
        if (len <= 0) {
            break;
        }

        ret = -1;

        if ((len < sizeof(line)) && (len > 10)) {
            lstr.uiLen = len - 2;
            KV_Parse(hKv, &lstr, ',', ':');
            ret = _tcpfinger2_ProcessFileLine(tcp_finger, hKv, id);
            KV_Reset(hKv);
        }

        if (ret != 0) {
            printf("Error: Can't load tcpfp rule %d \r\n", id);
        }

        id ++;
    }

    fclose(fp);
    KV_Destory(hKv);

    return BS_OK;
}

