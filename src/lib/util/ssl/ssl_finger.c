#include "bs.h"
#include <math.h>
#include "utl/net.h"
#include "utl/map_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"
#include "utl/tcp_utl.h"
#include "utl/lstr_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/ssl_finger.h"
#include "utl/ssl_decode.h"
#include "utl/cjson.h"


static void _sslfinger_free(void *data, VOID *pUserHandle)
{
    SSL_FINGER_NODE_S* node = data;

    cJSON_Delete(node->json_handle);
    MEM_Free(data);
}

static BS_STATUS _sslfinger_parseline(SSL_FINGER_S* ssl_finger, char *line, int id)
{
    cJSON *json;

    json = cJSON_Parse(line);
    if(NULL == json) {
        return BS_PARSE_FAILED;
    }
    
    int ret = SSLFinger_AddFinger(ssl_finger, json, id);
    if ((ret != 0) && (ret != BS_ALREADY_EXIST)) {
        return ret;
    }
    
    return BS_OK;
}

static BS_STATUS _sslfinger_parse_asn_line(SSL_FINGER_S *ctrl, char *line)
{
    char *ip_mask_str = line;

    char *split = strchr(line, ' ');
    if (! split) {
        RETURN(BS_ERR);
    }
    *split = 0;

    char *asn_str = split + 1;
    UINT asn = TXT_Str2Ui(asn_str);
    if (asn == 0) {
        RETURN(BS_ERR);
    }

    IP_MAKS_S ip_mask;
    IPString_ParseIpMask(ip_mask_str, &ip_mask);
    if (ip_mask.uiIP == 0) {
        RETURN(BS_ERR);
    }

    UCHAR depth = MASK_2_PREFIX(ip_mask.uiMask);

    return LPM_Add(&ctrl->lpm, ip_mask.uiIP, depth, asn);
}

#define SSLFP_MAX_LINE_SIZE (1024*16)

BS_STATUS SSLFinger_ParseFingerFile(SSL_FINGER_S* ssl_finger, char* file, UINT base_id)
{
    FILE *fp;
    char *line = MEM_Malloc(SSLFP_MAX_LINE_SIZE);
    int len;
    BS_STATUS ret=BS_OK;
    int id = base_id + 1;

    if (! line) {
        RETURN(BS_NO_MEMORY);
    }

    fp = fopen(file, "rb");
    if (NULL == fp) {
        MEM_Free(line);
        RETURN(BS_CAN_NOT_OPEN);
    }

    while(1) {
        len = FILE_ReadLine(fp, line, SSLFP_MAX_LINE_SIZE, '\n');
        if (len <= 0) {
            break;
        }

        if (len < SSLFP_MAX_LINE_SIZE)  {
            ret = _sslfinger_parseline(ssl_finger, line, id);
            if (ret != 0) {
                printf("Error: Can't load sslfp rule %d \r\n", id);
                ErrCode_Print();
            }
        } else {
            printf("Error: rule %d too long \r\n", id);
        }

        id ++;
    }

    fclose(fp);

    MEM_Free(line);

    return 0;
}

BS_STATUS SSLFinger_LoadASNFile(SSL_FINGER_S *ctrl, char *file)
{
    FILE *fp;
    char line[256];
    char *line_ptr;

    fp = fopen(file, "rb");
    if (! fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    while ((line_ptr = fgets(line, sizeof(line), fp))) {
        _sslfinger_parse_asn_line(ctrl, line);
    }

    fclose(fp);

    return 0;
}

BS_STATUS SSLFinger_Init(SSL_FINGER_S *ctrl)
{
    ctrl->map = MAP_Create(0);
    if (NULL == ctrl->map) {
        RETURN(BS_NO_MEMORY);
    }
    
    LPM_Init(&ctrl->lpm, 1024*64, NULL);
    LPM_SetLevel(&ctrl->lpm, 4, 8);
    
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

    LPM_Final(&ssl_finger->lpm);

    return;
}

BS_STATUS SSLFinger_AddFinger(SSL_FINGER_S *ssl_finger, cJSON *json, int id)
{
    cJSON* object;
    SSL_FINGER_NODE_S *node;

    node = MEM_ZMalloc(sizeof(SSL_FINGER_NODE_S));
    if (NULL == node) {
        RETURN(BS_NO_MEMORY);
    }

    node->id = id;

    object = cJSON_GetObjectItem(json, "str_repr"); 
    if(object) {
        TXT_Strlcpy(node->str_repr, object->valuestring, SSL_FINGER_MAX_FINGER_LEN + 1);
        node->json_handle = json; 

        int ret = MAP_Add(ssl_finger->map, node->str_repr, strlen(node->str_repr), node, 0);
        if (BS_OK != ret) {
            MEM_Free(node);
            return ret;
        }
    }else {
        MEM_Free(node);
        RETURN(BS_ERR);
    }

    return BS_OK;
}


static int ssl_finger_parse_extension(SSL_DECODE_EXTENSION_S* ext, char* finger_buf, int buf_len)
{
    int length = 0;
    USHORT tmp_ext_type = SSLDecode_DegreaseTypeCode(ext->type);
   
    length = scnprintf(finger_buf, buf_len, "(%04x", tmp_ext_type);

    if(SSLDecode_TypeInDecode(tmp_ext_type)) {
        length += scnprintf(finger_buf+length, buf_len-length, "%04x", ext->len);
        length += SSLDecode_DegreaseExtData(tmp_ext_type,ext, finger_buf+length, buf_len-length); 
    }else {
        length += scnprintf(finger_buf+length, buf_len-length, ")");  
    }

    return length;
}

static int ssl_finger_parse(SSL_CLIENT_HELLO_INFO_S *client_hello_info, char* finger_buf, int buf_len)
{
    int i, length = 0;
    USHORT data, *cipher_suites;
    SSL_DECODE_EXTENSION_S iteration;
    SSL_DECODE_EXTENSION_S *ext;

    length = scnprintf(finger_buf, buf_len, "(%04x)", htons(client_hello_info->handshake_ssl_ver)); 

    cipher_suites=client_hello_info->cipher_suites;
    length += scnprintf(finger_buf+length, buf_len-length, "(");
    for(i=0; i< client_hello_info->cipher_suites_len; i+=2) {
        data = SSLDecode_DegreaseTypeCode(ntohs(*cipher_suites));
        length += scnprintf(finger_buf+length, buf_len-length, "%04x", data);
        cipher_suites++;
    }
    length += scnprintf(finger_buf+length, buf_len-length, ")");

    SSLDecode_InitExtensionIter(&iteration);
    length += scnprintf(finger_buf+length, buf_len-length, "(");
    while((ext=SSLDecode_GetNextExtension(client_hello_info->extensions,
                    client_hello_info->extensions_len, &iteration))!=NULL) {
        length+=ssl_finger_parse_extension(ext, finger_buf+length, buf_len-length);
    }
    length += scnprintf(finger_buf+length, buf_len-length, ")");
    return length;
}

int SSLFinger_BuildFingerByClientHello(SSL_CLIENT_HELLO_INFO_S *client_hello, char *finger)
{
    return ssl_finger_parse(client_hello, finger, SSL_FINGER_MAX_FINGER_LEN);
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

SSL_FINGER_NODE_S * SSLFinger_Match(SSL_FINGER_S* finger,
        char*str_repr, int length)
{
    SSL_FINGER_NODE_S* node;
    node = MAP_Get(finger->map, str_repr, length);
    return node;
}

static inline UINT _sslfinger_GetASN(SSL_FINGER_S *ctrl, UINT ip)
{
    UINT64 asn;

    if (0 != LPM_Lookup(&ctrl->lpm, ip, &asn)) {
        return 0;
    }

    return asn;
}

static inline UINT _sslfinger_GetTotalCount(cJSON *object)
{
    cJSON *item = cJSON_GetObjectItem(object, "total_count");
    if (! item) {
        return 1;
    }
    
    UINT count = cJSON_GetNumberValue(item);
    if (count == 0) {
        count = 1;
    }

    return count;
}

static inline UINT _sslfinger_GetProcCount(cJSON *object)
{
    cJSON *item = cJSON_GetObjectItem(object, "count");
    if (! item) {
        return 1;
    }
    
    UINT count = cJSON_GetNumberValue(item);
    if (count == 0) {
        count = 1;
    }

    return count;
}

static inline UINT _sslfinger_GetASNCount(cJSON *object, char *asn_str)
{
    cJSON *obj = cJSON_GetObjectItem(object, "classes_ip_as");
    if (! obj) {
        return 0;
    }

    cJSON *item = cJSON_GetObjectItem(obj, asn_str);
    if (! item) {
        return 0;
    }
    
    return cJSON_GetNumberValue(item);
}

static char * _sslfinger_GetPortApp(USHORT port)
{
    switch (port) {
        case 443:
            return "https";
        case 448:
            return "database";
        case 465:
        case 585:
        case 993:
        case 995:
            return "email";
        case 563:
            return "nntp";
        case 614:
            return "shell";
        case 636:
            return "ldap";
        case 989:
        case 990:
            return "ftp";
        case 991:
            return "nas";
        case 992:
            return "telnet";
        case 994:
            return "irc";
        case 1443:
        case 8443:
            return "alt-https";
        case 2376:
            return "docker";
        case 8001:
        case 9000:
        case 9001:
        case 9002:
        case 9101:
            return "tor";
        default:
            break;
    }

    return "unknown";
}

static inline UINT _sslfinger_GetPortCount(cJSON *object, char *port_app)
{
    cJSON *obj= cJSON_GetObjectItem(object, "classes_port_applications");
    if (! obj) {
        return 0;
    }

    cJSON *item = cJSON_GetObjectItem(obj, port_app);
    if (! item) {
        return 0;
    }

    return cJSON_GetNumberValue(item);
}

static inline UINT _sslfinger_GetDomainCount(cJSON *object, char *domain)
{
    if (! domain) {
        return 0;
    }

    cJSON *obj = cJSON_GetObjectItem(object, "classes_hostname_domains");
    if (! obj) {
        return 0;
    }

    cJSON *item = cJSON_GetObjectItem(obj, domain);
    if (! item) {
        return 0;
    }

    return cJSON_GetNumberValue(item);
}

static char * _sslfinger_GetDomain(char *hostname)
{
    LSTR_S lstr;

    if (! hostname) {
        return NULL;
    }

    lstr.pcData = hostname;
    lstr.uiLen = strlen(hostname);

    char *dot = LSTR_ReverseStrchr(&lstr, '.');
    if (! dot) {
        return hostname;
    }

    lstr.uiLen = (dot - hostname) - 1;
    dot = LSTR_ReverseStrchr(&lstr, '.');
    if (! dot) {
        return hostname;
    }

    return dot + 1;
}

cJSON * SSLFinger_Analysis(SSL_FINGER_S *ctrl, SSL_FINGER_NODE_S *node,
        UINT ip/* 主机序 */, USHORT port, char *hostname, OUT double *out_score)
{
    long double base_prior;
    long double proc_prior = log(.1);
    long double prob_process_given_fp, score;
    long double max_score = -1.0;
    long double score_sum = 0.0;
    cJSON *procs = cJSON_GetObjectItem(node->json_handle, "process_info");
    cJSON *proc;
    cJSON *max_proc = NULL;
    char asn_str[32] = "";
    UINT tmp_value;
    char *domain = _sslfinger_GetDomain(hostname);
    char *port_app = _sslfinger_GetPortApp(port);

    UINT asn = _sslfinger_GetASN(ctrl, ip);
    if (asn) {
        sprintf(asn_str, "%u", asn);
    }

    UINT fp_tc = _sslfinger_GetTotalCount(node->json_handle);

    cJSON_ArrayForEach(proc, procs) {
        UINT p_count = _sslfinger_GetProcCount(proc);
        prob_process_given_fp = (long double)p_count/fp_tc;
        base_prior = log(1.0/fp_tc);
        score = log(prob_process_given_fp);
        score = fmax(score, proc_prior);

        tmp_value = _sslfinger_GetASNCount(proc, asn_str);
        if (tmp_value) {
            score += log((long double)tmp_value/fp_tc)*0.13924;
        } else {
            score += base_prior*0.13924;
        }

        tmp_value = _sslfinger_GetDomainCount(proc, domain);
        if (tmp_value) {
            score += log((long double)tmp_value/fp_tc)*0.15590;
        } else {
            score += base_prior*0.15590;
        }

        tmp_value = _sslfinger_GetPortCount(proc, port_app);
        if (tmp_value) {
            score += log((long double)tmp_value/fp_tc)*0.00528;
        } else {
            score += base_prior*0.00528;
        }

        score = exp(score);
        score_sum += score;

        if (score > max_score) {
            max_score = score;
            max_proc = proc;
        }
    }

    if (score_sum > 0.0) {
        max_score /= score_sum;
    }

    if (! max_proc) {
        return NULL;
    }

    if (out_score) {
        *out_score = max_score;
    }

    return max_proc;
}
 
