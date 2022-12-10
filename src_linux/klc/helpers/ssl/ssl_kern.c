#include "klc/klc_base.h"
#include "helpers/ssl_klc.h"

#define KLC_MODULE_NAME SSL_KLC_MODULE_NAME
KLC_DEF_MODULE();

static inline SSL_DECODE_EXTENSION_S * _sslklc_get_next_extension(void *extensions,
        int extensions_len, SSL_DECODE_EXTENSION_S *iteration)
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
    iteration->type = KLC_GET(ext->type);
    iteration->type = ntohs(iteration->type);
    iteration->len = KLC_GET(ext->len);
    iteration->len = ntohs(iteration->len);
    iteration->data = buf + sizeof(SSL_EXTENSION_S);
    left_len -= sizeof(SSL_EXTENSION_S);
    if (left_len < iteration->len) {
        return NULL;
    }

    return iteration;
}

static inline void _sslklc_init_extension_iter(SSL_DECODE_EXTENSION_S *iteration)
{
    memset(iteration, 0, sizeof(SSL_DECODE_EXTENSION_S));
}

static inline LSTR_S * _sslklc_get_ext_sni(SSL_DECODE_EXTENSION_S *ext, LSTR_S *sni)
{
    SSL_EXTENSION_SNI_S *sni_ext;

    if (ext->type != TLSEXT_TYPE_server_name) {
        return NULL;
    }

    if (ext->len < sizeof(SSL_EXTENSION_SNI_S)) {
        return NULL;
    }

    sni_ext = ext->data;
    if (KLC_GET(sni_ext->server_name_type) != TLSEXT_NAMETYPE_host_name) {
        return NULL;
    }

    USHORT len = KLC_GET(sni_ext->server_name_list_len);
    if (ntohs(len) + sizeof(USHORT) > ext->len) {
        return NULL;
    }

    len = KLC_GET(sni_ext->server_name_len);
    if (ntohs(len) + sizeof(USHORT)*2 + 1 > ext->len) {
        return NULL;
    }

    sni->pcData = (void*)(sni_ext + 1);
    sni->uiLen = ntohs(len);

    return sni;
}

SEC_NAME_FUNC(SSL_KLC_GET_SNI)
LSTR_S * _sslklc_get_sni(void *extensions, int extensions_len, LSTR_S *sni)
{
    SSL_DECODE_EXTENSION_S iteration;
    SSL_DECODE_EXTENSION_S *ext = NULL;

    _sslklc_init_extension_iter(&iteration);

    while ((ext = _sslklc_get_next_extension(extensions, extensions_len, &iteration)) != NULL) {
        if (NULL != _sslklc_get_ext_sni(ext, sni)) {
            return sni;
        }
    }

    return NULL;
}


