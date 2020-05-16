/*================================================================
*   Created by LiXingang: 2018.11.14
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/ssl_decode.h"

/* Record content type */
#define SSL_RECORD_CONTENT_TYPE_HSK 22

/* Record handshake type */
#define SSL_HANDSHAKE_TYPE_CLIENT_HELLO 1

#pragma pack(1)
typedef struct {
    UCHAR  content_type;
    USHORT ver;
    USHORT len;
}SSL_RECORD_S;

typedef struct {
    UCHAR handshake_type;
    UCHAR len[3];
    USHORT ver;
    UINT   time;
    UCHAR  random[28];
    UCHAR  session_id_len;
}SSL_CLIENT_HELLO_S;

typedef struct {
    USHORT type;
    USHORT len;
}SSL_EXTENSION_S;

typedef struct {
    USHORT server_name_list_len;
    UCHAR  server_name_type;
    USHORT server_name_len;
}SSL_EXTENSION_SNI_S;
#pragma pack()

int SSLDecode_IsRecordComplete(void *buf, int buf_len)
{
    SSL_RECORD_S *record;
    int payload_len;

    if (buf_len < sizeof(SSL_RECORD_S)) {
        return 0;
    }

    record = buf;
    payload_len = buf_len - sizeof(SSL_RECORD_S);

    if (htons(record->len) > payload_len) {
        return 0;
    }

    return 1;
}

int SSLDecode_ParseClientHello(UCHAR *buf, int buf_len, SSL_CLIENT_HELLO_INFO_S *client_hello_info)
{
    SSL_RECORD_S *record;
    SSL_CLIENT_HELLO_S *client_hello;
    int tmp_len = buf_len;
    UCHAR *tmp_buf = buf;

    memset(client_hello_info, 0, sizeof(SSL_CLIENT_HELLO_INFO_S));

    if (! SSLDecode_IsRecordComplete(tmp_buf, tmp_len)) {
        return -1;
    }

    record = (void*)tmp_buf;
    if (record->content_type != SSL_RECORD_CONTENT_TYPE_HSK) {
        return -1;
    }

    tmp_len -= sizeof(SSL_RECORD_S);
    tmp_buf += sizeof(SSL_RECORD_S);

    if (tmp_len < sizeof(SSL_CLIENT_HELLO_S)) {
        return -1;
    }

    /* parse client hello header */
    client_hello = (void*)tmp_buf;
    if (client_hello->handshake_type != SSL_HANDSHAKE_TYPE_CLIENT_HELLO) {
        return -1;
    }

    client_hello_info->len = client_hello->len[0];
    client_hello_info->len = (client_hello_info->len << 8) | client_hello->len[1];
    client_hello_info->len = (client_hello_info->len << 8) | client_hello->len[2];

    if (client_hello_info->len < sizeof(SSL_CLIENT_HELLO_S) - 4) {
        return -1;
    }
    if (client_hello_info->len > tmp_len - 4) {
        return -1;
    }
    tmp_len -= sizeof(SSL_CLIENT_HELLO_S);
    tmp_buf += sizeof(SSL_CLIENT_HELLO_S);

    client_hello_info->random = tmp_buf;
    tmp_len -= client_hello->session_id_len;
    tmp_buf += client_hello->session_id_len;

    client_hello_info->handshake_type = client_hello->handshake_type;
    client_hello_info->handshake_ssl_ver = ntohs(client_hello->ver);
    client_hello_info->time = ntohl(client_hello->time);
    client_hello_info->session_id_len = client_hello->session_id_len;

    /* cipher suites */
    if (tmp_len < 2) {
        return -1;
    }
    client_hello_info->cipher_suites_len = *(USHORT*)tmp_buf;
    client_hello_info->cipher_suites_len = ntohs(client_hello_info->cipher_suites_len);
    tmp_len -= 2;
    tmp_buf += 2;
    if (tmp_len < client_hello_info->cipher_suites_len) {
        return -1;
    }
    client_hello_info->cipher_suites = (void*)tmp_buf;
    tmp_len -= client_hello_info->cipher_suites_len;
    tmp_buf += client_hello_info->cipher_suites_len;

    /* compression methods */
    client_hello_info->compression_methods_len = *tmp_buf;
    tmp_len -= 1;
    tmp_buf += 1;
    if (tmp_len < client_hello_info->compression_methods_len) {
        return -1;
    }
    tmp_len -= client_hello_info->compression_methods_len;
    tmp_buf += client_hello_info->compression_methods_len;

    /* extensions */
    if (tmp_len < 2) {
        return -1;
    }
    client_hello_info->extensions_len = *(USHORT*)tmp_buf;
    client_hello_info->extensions_len = ntohs(client_hello_info->extensions_len);
    tmp_len -= 2;
    tmp_buf += 2;
    if (tmp_len < client_hello_info->extensions_len) {
        return -1;
    }
    client_hello_info->extensions = (void*)tmp_buf;

    return 0;
}

void SSLDecode_InitExtensionIter(SSL_DECODE_EXTENSION_S *iteration)
{
    memset(iteration, 0, sizeof(SSL_DECODE_EXTENSION_S));
}

SSL_DECODE_EXTENSION_S * SSLDecode_GetNextExtension(void *extensions, int extensions_len, SSL_DECODE_EXTENSION_S *iteration)
{
    SSL_EXTENSION_S *ext;
    int left_len = extensions_len;
    UCHAR *buf = extensions;

    if (iteration == NULL) {
        return NULL;
    }
    
    if (NULL != iteration->data) {
        left_len -= ((UCHAR*)iteration->data - (UCHAR*)extensions);
        left_len -= iteration->len;
        buf = (UCHAR*)iteration->data + iteration->len;
    }

    if (left_len < sizeof(SSL_EXTENSION_S)) {
        return NULL;
    }

    ext = (void*)buf;
    iteration->type = ntohs(ext->type);
    iteration->len = ntohs(ext->len);
    iteration->data = buf + sizeof(SSL_EXTENSION_S);
    left_len -= sizeof(SSL_EXTENSION_S);
    if (left_len < iteration->len) {
        return NULL;
    }

    return iteration;
}

static LSTR_S * ssldecode_get_ext_sni(SSL_DECODE_EXTENSION_S *ext, LSTR_S *sni)
{
    SSL_EXTENSION_SNI_S *sni_ext;

    if (ext->type != TLSEXT_TYPE_server_name) {
        return NULL;
    }

    if (ext->len < sizeof(SSL_EXTENSION_SNI_S)) {
        return NULL;
    }

    sni_ext = ext->data;
    if (sni_ext->server_name_type != TLSEXT_NAMETYPE_host_name) {
        return NULL;
    }

    if (ntohs(sni_ext->server_name_list_len) + sizeof(USHORT) > ext->len) {
        return NULL;
    }

    if (ntohs(sni_ext->server_name_len) + sizeof(USHORT)*2 + 1 > ext->len) {
        return NULL;
    }

    sni->pcData = (void*)(sni_ext + 1);
    sni->uiLen = ntohs(sni_ext->server_name_len);

    return sni;
}

LSTR_S * SSLDecode_GetSNI(void *extensions, int extensions_len, LSTR_S *sni)
{
    SSL_DECODE_EXTENSION_S iteration;
    SSL_DECODE_EXTENSION_S *ext = NULL;

    SSLDecode_InitExtensionIter(&iteration);

    while ((ext= SSLDecode_GetNextExtension(extensions, extensions_len, &iteration)) != NULL) {
        if (NULL != ssldecode_get_ext_sni(ext, sni)) {
            return sni;
        }
    }

    return NULL;
}

char * SSLDecode_GetSNIString(void *extensions, int extensions_len, char *sni, int sni_size)
{
    LSTR_S stSni;
    int len;

    if (NULL == SSLDecode_GetSNI(extensions, extensions_len, &stSni)) {
        return NULL;
    }

    len = stSni.uiLen;
    if (sni_size <= len) {
        len = sni_size - 1;
    }

    memcpy(sni, stSni.pcData, len);
    sni[len] = 0;
    
    return sni;
}
