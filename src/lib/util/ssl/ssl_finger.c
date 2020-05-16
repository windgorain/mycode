#include "bs.h"
#include "utl/net.h"
#include "utl/map_utl.h"
#include "utl/ip_utl.h"
#include "utl/tcp_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/ssl_finger.h"
#include "utl/ssl_decode.h"
#include "utl/cjson.h"


void _sslfinger_free(void *data, VOID *pUserHandle)
{
    SSL_FINGER_NODE_S* node = data;

    cJSON_Delete(node->json_handle);
    MEM_Free(data);
}

BS_STATUS _sslfinger_parseline(char* line, SSL_FINGER_S* ssl_finger)
{
    cJSON *json;

    json = cJSON_Parse(line);
    if(NULL == json)
    {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return BS_JSON_PARSE_FAIL;
    }
    
    if(SSLFinger_AddFinger(json, ssl_finger)!=BS_OK) {
        cJSON_Delete(json);
        return BS_ERR;
    }
    
    return BS_OK;
}

BS_STATUS SSLFinger_ParseFingerFile(char* file, SSL_FINGER_S* ssl_finger)
{
    FILE *fp;
    char line[4096];
    int len;
    BS_STATUS ret=BS_OK;

    fp = fopen(file, "rb");
    if (NULL == fp) {
        return BS_CAN_NOT_OPEN;
    }

    do {
        len = FILE_ReadLine(fp, line, sizeof(line), '\n');
        if ((len < sizeof(line)) && (len > 0)) {
            //printf("len=%d\r\n", len);
            ret = _sslfinger_parseline(line, ssl_finger);
        }
    } while((len > 0) && (ret == BS_OK));

    return ret;

}


BS_STATUS SSLFinger_Init(SSL_FINGER_S *ssl_finger)
{
    ssl_finger->map = MAP_Create();
    if (NULL == ssl_finger->map) {
        RETURN(BS_NO_MEMORY);
    }
    
    
    return BS_OK;
}

void SSLFinger_Fin(SSL_FINGER_S *ssl_finger)
{
    if (NULL == ssl_finger) {
        return;
    }

    if (ssl_finger->map) {
        MAP_Destroy(ssl_finger->map, _sslfinger_free, NULL);
    }

    return;
}

BS_STATUS SSLFinger_AddFinger(cJSON* json, SSL_FINGER_S *ssl_finger)
{
    cJSON* object;
    SSL_FINGER_NODE_S *node;

    node = MEM_ZMalloc(sizeof(SSL_FINGER_NODE_S));
    if (NULL == node) {
        return BS_NO_MEMORY;
    }
    object = cJSON_GetObjectItem(json, "str_repr"); 
    if(object) {
        TXT_Strlcpy(node->str_repr, object->valuestring, SSL_FINGER_MAX_FINGER_LEN + 1);
        node->json_handle = json; 

        if (BS_OK != MAP_Add(ssl_finger->map, node->str_repr, strlen(node->str_repr), node)) {
            MEM_Free(node);
            return BS_ERR;
        }
    }else {
        MEM_Free(node);
        return BS_ERR;
    }

    return BS_OK;
}

USHORT grease_[] = {0x0a0a,0x1a1a,0x2a2a,0x3a3a,0x4a4a,0x5a5a,0x6a6a,0x7a7a,
               0x8a8a,0x9a9a,0xaaaa,0xbaba,0xcaca,0xdada,0xeaea,0xfafa};

USHORT ext_data_extract_[] = {0x0001,0x0005,0x0007,0x0008,0x0009,0x000a,0x000b,
                              0x000d,0x000f,0x0010,0x0011,0x0013,0x0014,0x0018,
                              0x001b,0x001c,0x002b,0x002d,0x0032,0x5500};

BOOL_T ssl_type_in_grase(USHORT type)
{
    int i;

    for(i=0; i< sizeof(grease_)/sizeof(USHORT); i++) {
        if(type ==grease_[i]) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL_T ssl_type_in_extract(USHORT type)
{
    int i;

    for(i=0; i< sizeof(ext_data_extract_)/sizeof(USHORT); i++) {
        if(type == ext_data_extract_[i]) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL_T ssl_type_in_decode(USHORT type)
{
    if(ssl_type_in_grase(type)) {
        return TRUE;
    }
    
    if(ssl_type_in_extract(type)) {
        return TRUE;
    }

    return FALSE;

}


USHORT degrease_type_code(USHORT type)
{
    if(ssl_type_in_grase(type)) {
        return 0x0a0a;
    }
    
    return type;
}

int degrease_ext_data(USHORT ext_type, SSL_DECODE_EXTENSION_S*ext,
                      char* finger_buf, int buf_len)
{
    USHORT ext_value;
    int i, length=0;
    
    if(ext_type == 0x000a) {
        for(i=0; i<ext->len; i+=2) {
            ext_value = *(USHORT*)((char*)ext->data+i);
            if(ssl_type_in_grase(ext_value)) {
                ext_value = 0x0a0a;
            }
            length += snprintf(finger_buf+length, buf_len-length, "%04x", ntohs(ext_value));
        }
    }else if(ext_type == 0x002b) {
        ext_value = *(char*)ext->data;
        length += snprintf(finger_buf+length, buf_len-length, "%02x", ext_value);
        for(i=1; i<ext->len; i+=2) {
            ext_value = *(USHORT*)((char*)ext->data+i);
            if(ssl_type_in_grase(ext_value)) {
                ext_value = 0x0a0a;
            }
            length += snprintf(finger_buf+length, buf_len-length, "%04x", ntohs(ext_value));
        }
    }else {
        for(i=0; i<ext->len; i++) {
            ext_value = *((char*)ext->data+i);
            length += snprintf(finger_buf+length, buf_len-length, "%02x", ext_value);
        }
    }

    length += snprintf(finger_buf+length, buf_len-length, ")");

    return length;
}

int ssl_finger_parse_extension(SSL_DECODE_EXTENSION_S* ext, char* finger_buf, int buf_len)
{
    int length = 0;
    USHORT tmp_ext_type = degrease_type_code(ext->type);
   
    length = snprintf(finger_buf, buf_len, "(%04x", tmp_ext_type);

    if(ssl_type_in_decode(tmp_ext_type)) {
        length += snprintf(finger_buf+length, buf_len-length, "%04x", ext->len);
        length += degrease_ext_data(tmp_ext_type,ext, finger_buf+length, buf_len-length); 
    }else {
        length += snprintf(finger_buf+length, buf_len-length, ")");  
    }

    return length;
}

int ssl_finger_parse(SSL_CLIENT_HELLO_INFO_S *client_hello_info, char* finger_buf, int buf_len)
{
    int i, length = 0;
    USHORT data, *cipher_suites;
    SSL_DECODE_EXTENSION_S iteration;
    SSL_DECODE_EXTENSION_S *ext;

    length = snprintf(finger_buf, buf_len, "(%04x)", htons(client_hello_info->handshake_ssl_ver)); 

    cipher_suites=client_hello_info->cipher_suites;
    length += snprintf(finger_buf+length, buf_len-length, "(");
    for(i=0; i< client_hello_info->cipher_suites_len; i+=2) {
        data = degrease_type_code(ntohs(*cipher_suites));
        length += snprintf(finger_buf+length, buf_len-length, "%04x", data);
        cipher_suites++;
    }
    length += snprintf(finger_buf+length, buf_len-length, ")");

    SSLDecode_InitExtensionIter(&iteration);
    length += snprintf(finger_buf+length, buf_len-length, "(");
    while((ext=SSLDecode_GetNextExtension(client_hello_info->extensions, client_hello_info->extensions_len, &iteration))!=NULL) {
        length+=ssl_finger_parse_extension(ext, finger_buf+length, buf_len-length);
        }
    length += snprintf(finger_buf+length, buf_len-length, ")");
    return length;
}



int SSLFinger_BuildFinger(void *ipheader, void *tcpheader, char *finger)
{
    UCHAR* buf;
    int buf_len, tcph_len, finger_len=0;
    SSL_CLIENT_HELLO_INFO_S client_hello_info;
    TCP_HEAD_S *tcph = tcpheader;
    IP_HEAD_S* iph = ipheader;

    tcph_len = (tcph->ucHeadLenAndOther>>4)*4;
    buf = ((UCHAR*)tcpheader)+tcph_len;
    buf_len = ntohs(iph->usTotlelen)-(iph->ucHLen*4)-tcph_len;
     
    if(!SSLDecode_ParseClientHello(buf, buf_len, &client_hello_info)) {
        finger_len = ssl_finger_parse(&client_hello_info, finger, SSL_FINGER_MAX_FINGER_LEN);
        //printf("finger_str[%d]=%s\r\n",finger_len, finger);
    }

    return finger_len;
}

int SSLFinger_Match(SSL_FINGER_S* finger, char*str_repr, int length)
{
    SSL_FINGER_NODE_S* node;
    cJSON* object;

    node = MAP_Get(finger->map, str_repr, length);
    if(node) {
        object = cJSON_GetObjectItem(node->json_handle, "process_info");
        if(object) {
            printf("%s\r\n", cJSON_Print(object));
        }
        return BS_OK;
    }
    
    return BS_ERR;
}


 
