/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#ifdef INTEL_HYPERSCAN
#include <hs.h>
#endif
#include "utl/box_utl.h"
#include "utl/ac_smx.h"
#include "utl/dpi_utl.h"
#include "utl/lstr_utl.h"
#include "utl/tcp_utl.h"
#include "utl/udp_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"
#include "utl/socket_utl.h"
#include "utl/kv_utl.h"
#include "utl/txt_utl.h"
#include "utl/lpm_utl.h"
#include "utl/ss_utl.h"
#include "utl/dns_utl.h"
#include "utl/dnsname_trie.h"
#include "utl/passwd_utl.h"
#include "utl/trie_utl.h"

enum dpi_pattern_match_ret {
    PATTERN_MATCH,     
    PATTERN_NO_MATCH,  
    PATTERN_NEED_MORE, 

    PATTERN_MATCH_MAX
};


typedef struct {
    UINT protocol_map[256];  
    UINT tcp_port_map[65536];  
    UINT udp_port_map[65536];  
    UINT payload_tport_map[65536]; 
    UINT payload_uport_map[65536]; 
    BOX_S iptup3_map;
    LPM_S ipmask_map;
    DPI_IPDOMAINNAME_S ipdomain_s; 
    
    ACSMX_S *tcp_acsm;
    ACSMX_S *udp_acsm;
#ifdef INTEL_HYPERSCAN
    hs_database_t *hs_db; 
    hs_scratch_t *scratch;
#endif 
    BOX_S id_name_map; 
    BOX_S name_id_map; 

    TRIE_HANDLE trie;
    NAP_HANDLE rules_tbl;
    NAP_HANDLE app_tbl;
}DPI_S;

typedef struct {
    UCHAR *data;
    int len;
    CHAR protocol;
    USHORT port;
    DPI_HANDLE hDpi;
    DPI_RULE_S* best_rule;
}DPI_ACSEARCH_DATA_S;

typedef int (*PF_DPI_LOAD_MAP)(DPI_S *dpi, KV_HANDLE kv, UINT line);

static void dpi_free_id_name_node(BOX_S *box, void *data, void *ud)
{
    DPI_ID_NAME_CATEGORY_S *node = container_of(data, DPI_ID_NAME_CATEGORY_S, appid);
    MEM_Free(node);
}

static void dpi_free_iptup3_node(BOX_S *box, void *data, void *ud)
{
    DPI_IPTUP3_ID_S *node = container_of(data, DPI_IPTUP3_ID_S, key);
    MEM_Free(node);
}

static int dpi_LoadIdNameMap(DPI_S *dpi, KV_HANDLE kv, UINT line)
{
    char *str_id = KV_GetKeyValue(kv, "id");
    char *name = KV_GetKeyValue(kv, "name");
    char *category = KV_GetKeyValue(kv, "category");

    if ((str_id == NULL) || (name == NULL) || (category == NULL)) {
        RETURN(BS_NOT_FOUND);
    }

    int id = TXT_Str2Ui(str_id);

    BS_DBGASSERT(id > 0);

    return DPI_AddIDName(dpi, id, name, category);
}

static int dpi_LoadRuleLine(DPI_S *dpi, KV_HANDLE kv, UINT rule_id)
{
    char *str_protocol = KV_GetKeyValue(kv, "protocol");
    char *str_port = KV_GetKeyValue(kv, "port");
    char *str_ip = KV_GetKeyValue(kv, "ip");
    char *hostname = KV_GetKeyValue(kv, "host");
    char *app = KV_GetKeyValue(kv, "app");
    char* content = KV_GetKeyValue(kv, "content");
    UCHAR protocol = 0;
    USHORT port = 0;
    IP_MASK_S ip_mask = {0};
    int appid;

    if (! app) {
        fprintf(stderr, "Error: DPI rule %d has not app \r\n", rule_id);
        RETURN(BS_NOT_FOUND);
    }

    appid = DPI_GetIDByName(dpi, app);
    if (appid <= 0) {
        fprintf(stderr, "Error: DPI rule %d can't get appid for app %s \r\n", rule_id, app);
        RETURN(BS_NOT_FOUND);
    }

    if (str_protocol) {
        protocol = TXT_Str2Ui(str_protocol);
    }

    if (str_port) {
        port = TXT_Str2Ui(str_port);
    }

    if (str_ip) {
        if (strchr(str_ip, '/')) {
            if (BS_OK != IPString_ParseIpMask(str_ip, &ip_mask)) {
                BS_DBGASSERT(0);
                RETURN(BS_ERR);
            }
        } else {
            ip_mask.uiIP = Socket_Ipsz2IpHost(str_ip);
            ip_mask.uiMask = 0xffffffff;
        }
    }

    DPI_RULE_S * rule = NAP_AllocByIndex(dpi->rules_tbl, rule_id);
    if (! rule) {
        RETURN(BS_NO_MEMORY);
    }
    rule->appid = appid;
    rule->id = rule_id;

    if (ip_mask.uiIP) {
        
        if (port == 0) {
            return DPI_AddIPMask(dpi, ip_mask.uiIP, ip_mask.uiMask, rule_id);
        } else {
            return DPI_AddIPTup3(dpi, protocol, htonl(ip_mask.uiIP), htons(port), rule_id);
        }
    }

    
    if (port && !hostname && !content) {
        if (protocol == IPPROTO_TCP) {
            return DPI_AddTcpPort(dpi, htons(port), rule_id);
        } else if (protocol == IPPROTO_UDP) {
            return DPI_AddUdpPort(dpi, htons(port), rule_id);
        } else {
            RETURN(BS_NOT_SUPPORT);
        }
    }
    
    
    if (protocol && !hostname && !content) {
        return DPI_AddProtocol(dpi, protocol, rule_id);
    }

    
    if (hostname) {
        return DPI_AddHostname(dpi, hostname, rule_id);
    }
    
    if(content) {
        return DPI_AddContent(dpi, kv, rule);
    }

    RETURN(BS_ERR);
}

static int dpi_LoadMapFile(DPI_S *dpi, char *file, PF_DPI_LOAD_MAP func, int encrypt, UINT baseid)
{
    FILE *fp;
    char buf[1024];
    char *line;
    LSTR_S lstr;
    UINT line_num = baseid;
    int ret;
    char szCipher[1024];

    fp = fopen(file, "rb");
    if (!fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    KV_PARAM_S kv_param = {0};
    kv_param.uiFlag = KV_FLAG_PERMIT_MUL_KEY;
    KV_HANDLE kv = KV_Create(&kv_param);
    if (! kv) {
        fclose(fp);
        RETURN(BS_NO_MEMORY);
    }

    while (NULL != fgets(buf, sizeof(buf), fp)) {
        line = TXT_Strim(buf);
        if(encrypt)
        {
            PW_HexDecrypt(line, szCipher, sizeof(szCipher));
            line = szCipher;
        }
        lstr.pcData = line;
        lstr.uiLen = strlen(line);
        KV_Parse(kv, &lstr, ';', ':');
        ret = func(dpi, kv, line_num);
        if(ret != 0) {
            fprintf(stderr, "dpi load file %s line %d fail %d\r\n", file, line_num, ret);
        }
        line_num ++;
        KV_Reset(kv);
    }

    KV_Destory(kv);
    fclose(fp);

    return BS_OK;
}

#ifdef IN_DEBUG
void DPI_EncryptIDNameMapFile(char* clear_file, char*cipher_file)
{
    FILE* fp, *cipher_fp;
    char* line;
    char buf[1024];
    char szCipher[1024];

    fp = fopen(clear_file, "rb");
    if(!fp) {
        return;
    }
    cipher_fp = fopen(cipher_file, "w");
    if(!cipher_fp) {
        fclose(fp);
        return;
    }

    while (NULL != fgets(buf, sizeof(buf), fp)) {
        line = TXT_Strim(buf);
        PW_HexEncrypt(line, szCipher, sizeof(szCipher));
        fwrite(szCipher, 1, strlen(szCipher), cipher_fp);
        fwrite("\n", 1, strlen("\n"), cipher_fp);

    }

    fclose(fp);
    fclose(cipher_fp);

    return;

}
#endif


static UINT dpi_MatchRuleByIP(DPI_S *dpi, UCHAR protocol, UINT ip, USHORT port)
{
    IP_TUP3_KEY_S key = {0};
    DPI_IPTUP3_ID_S *node;
    UINT rule_id = 0;

    
    key.ip = ip;
    key.port = port;
    key.protocol = protocol;
    node = Box_FindData(&dpi->iptup3_map, &key);
    if (node) {
        return node->rule_id;
    }

    
    if (protocol == IPPROTO_TCP) {
        rule_id = dpi->tcp_port_map[port];
    } else if (protocol == IPPROTO_UDP) {
        rule_id = dpi->udp_port_map[port];
    }
    if (rule_id) {
        return rule_id;
    }

    
    UINT64 next_hop;
    if (BS_OK == LPM_Lookup(&dpi->ipmask_map, ntohl(ip), &next_hop)) {
        return next_hop;
    }

    return 0;
}

static UINT dpi_MatchRuleByProtocol(DPI_S *dpi, UCHAR protocol)
{
    UINT rule_id = 0;

    rule_id = dpi->protocol_map[protocol];
    if (rule_id) {
        return rule_id;
    }

    return 0;
}

static UINT dpi_MatchRuleByHostname(DPI_S *dpi, char *hostname)
{
    HANDLE hID;
    char invert_hostname[255] = {0};

    MEM_Invert(hostname, strlen(hostname), invert_hostname);

    TRIE_COMMON_S *trieComm = Trie_Match(dpi->trie, (UCHAR *)invert_hostname,
            strlen(invert_hostname), TRIE_MATCH_WILDCARD);
    if(!trieComm) return 0;
    hID = trieComm->ud;
    
    return HANDLE_UINT(hID);
}

#if 0
static int dpi_GetAppIDByRuleID(DPI_S *dpi, UINT rule_id)
{
    if (! rule_id) {
        return 0;
    }

    DPI_RULE_S *rule = NAP_GetNodeByIndex(dpi->rules_tbl, rule_id);
    if (! rule) {
        return 0;
    }

    return rule->appid;
}
#endif

int DPI_AddIPDomainname(DPI_HANDLE hDpi, UINT ip, char *domainname)
{
    DPI_S *dpi = hDpi;
    
    DPI_IP_DOMAIN_S key = {0};
    key.ip = ip;
    TXT_Strlcpy(key.domain, domainname, DPI_DOMAINNAME_MAX_SIZE);
    if (0 == BloomFilter_TrySet(&dpi->ipdomain_s.ipdomain_bloom.bloomfilter, &key, sizeof(key))) {
        return 1;
    }
    return 0;
}

DPI_HANDLE DPI_New()
{
    DPI_S *ctrl = MEM_ZMalloc(sizeof(DPI_S));
    if (! ctrl) {
        return NULL;
    }

    IDBox_Init(&ctrl->id_name_map, NULL, 4*1024, 8);
    StrBox_Init(&ctrl->name_id_map, NULL, 4*1024, 8);
    IPTup3Box_Init(&ctrl->iptup3_map, NULL, 4*1024, 8);

    ctrl->tcp_acsm = ACSMX_New(); 
    ctrl->udp_acsm = ACSMX_New(); 
    
    LPM_Init(&ctrl->ipmask_map, 1024*512, NULL);
    LPM_SetLevel(&ctrl->ipmask_map, 7, 8);

    NAP_PARAM_S param = {0};
    param.enType = NAP_TYPE_HASH;
    param.uiNodeSize = sizeof(DPI_RULE_S);

    ctrl->rules_tbl = NAP_Create(&param);

    BloomFilter_Init(&ctrl->ipdomain_s.ipdomain_bloom.bloomfilter, DPI_DOMAINNAME_BLOOMFILTER_SIZE);
    BloomFilter_SetStepsToClearAll(&ctrl->ipdomain_s.ipdomain_bloom.bloomfilter, 60);
    BloomFilter_SetAutoStep(&ctrl->ipdomain_s.ipdomain_bloom.bloomfilter, 1);
    
    ctrl->trie = Trie_Create(TRIE_TYPE_4BITS);

    return ctrl;
}

void DPI_Delete(DPI_HANDLE hDpi)
{
    DPI_S *dpi = hDpi;

    Box_Fini(&dpi->id_name_map, dpi_free_id_name_node, NULL);
    Box_Fini(&dpi->name_id_map, NULL, NULL);
    Box_Fini(&dpi->iptup3_map, dpi_free_iptup3_node, NULL);
    LPM_Final(&dpi->ipmask_map);
    if (dpi->rules_tbl) {
        NAP_FreeAll(dpi->rules_tbl);
    }
    if(dpi->tcp_acsm) {
        ACSMX_Free(dpi->tcp_acsm, NULL);
    }
    if(dpi->udp_acsm) {
        ACSMX_Free(dpi->udp_acsm, NULL);
    }
    BloomFilter_Final(&dpi->ipdomain_s.ipdomain_bloom.bloomfilter);
    
    Trie_Destroy(dpi->trie, NULL);
    MEM_Free(dpi);
}

int DPI_AddIDName(DPI_HANDLE hDpi, int appid, char *name, char *category)
{
    DPI_S *dpi = hDpi;
    int id_index, name_index;

    id_index = Box_Find(&dpi->id_name_map, &appid);
    if (id_index >= 0) {
        RETURN(BS_ALREADY_EXIST);
    }

    DPI_ID_NAME_CATEGORY_S *node = MEM_Malloc(sizeof(DPI_ID_NAME_CATEGORY_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    node->appid = appid;
    strlcpy(node->name, name, DPI_NAME_SIZE); 
    strlcpy(node->category,category,DPI_CATEGORY_SIZE);

    id_index = Box_Add(&dpi->id_name_map, &node->appid);
    if (id_index < 0) {
        MEM_Free(node);
        RETURN(BS_NO_MEMORY);
    }

    name_index = Box_Add(&dpi->name_id_map, node->name);
    if (name_index < 0) {
        Box_DelByIndex(&dpi->name_id_map, id_index);
        MEM_Free(node);
        RETURN(BS_NO_MEMORY);
    }

    return BS_OK;
}

int DPI_GetIDByName(DPI_HANDLE dpi_handle, char *name)
{
    DPI_S *dpi = dpi_handle;

    void *data = Box_FindData(&dpi->name_id_map, name);
    if (! data) {
        RETURN(BS_NO_SUCH);
    }

    DPI_ID_NAME_CATEGORY_S * node = container_of(data, DPI_ID_NAME_CATEGORY_S, name);
    
    return node->appid;
}

char * DPI_GetNameByID(DPI_HANDLE dpi_handle, int id)
{
    DPI_S *dpi = dpi_handle;

    void *data = Box_FindData(&dpi->id_name_map, &id);
    if (! data) {
        return NULL;
    }

    DPI_ID_NAME_CATEGORY_S * node = container_of(data, DPI_ID_NAME_CATEGORY_S, appid);
    
    return node->name;
}

char * DPI_GetCategoryByID(DPI_HANDLE dpi_handle, int id)
{
    DPI_S *dpi = dpi_handle;

    void *data = Box_FindData(&dpi->id_name_map, &id);
    if (! data) {
        return NULL;
    }

    DPI_ID_NAME_CATEGORY_S * node = container_of(data, DPI_ID_NAME_CATEGORY_S, appid);
    
    return node->category;
}

int DPI_AddProtocol(DPI_HANDLE hDpi, USHORT ip_protocol, UINT rule_id)
{
    DPI_S *dpi = hDpi;
    dpi->protocol_map[ip_protocol] = rule_id;

    return 0;
}

int DPI_AddTcpPort(DPI_HANDLE hDpi, USHORT port, UINT rule_id)
{
    DPI_S *dpi = hDpi;
    dpi->tcp_port_map[port] = rule_id;
    return 0;
}

int DPI_AddUdpPort(DPI_HANDLE hDpi, USHORT port, UINT rule_id)
{
    DPI_S *dpi = hDpi;
    dpi->udp_port_map[port] = rule_id;
    return 0;
}

int DPI_AddIPTup3(DPI_HANDLE hDpi, UCHAR protocol, UINT ip, USHORT port, UINT rule_id)
{
    DPI_S *dpi = hDpi;
    IP_TUP3_KEY_S key = {0};
    DPI_IPTUP3_ID_S *node;
    int index;

    key.protocol = protocol;
    key.ip = ip;
    key.port = port;

    index = Box_Find(&dpi->iptup3_map, &key);
    if (index >= 0) {
        RETURN(BS_ALREADY_EXIST);
    }

    node = MEM_ZMalloc(sizeof(DPI_IPTUP3_ID_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    node->key = key;
    node->rule_id = rule_id;
    index = Box_Add(&dpi->iptup3_map, &node->key);
    if (index < 0) {
        MEM_Free(node);
        return index;
    }

    return BS_OK;
}

int DPI_AddIPMask(DPI_HANDLE hDpi, UINT ip, UCHAR depth, UINT rule_id)
{
    DPI_S *dpi = hDpi;
    return LPM_Add(&dpi->ipmask_map, ip, depth, rule_id);
}

int DPI_AddHostname(DPI_HANDLE hDpi, char *hostname, UINT rule_id)
{
    DPI_S *dpi = hDpi;
    int i;
    char c;
    char invert_hostname[256] = {0};

    MEM_Invert(hostname, strlen(hostname), invert_hostname);
    for(i=0; invert_hostname[i] != '\0'; i++) {
        c = invert_hostname[i];
        if (c == '*') {
            invert_hostname[i] = '\0';
        }
    }

    Trie_Insert(dpi->trie, (UCHAR *)invert_hostname, strlen(invert_hostname), TRIE_NODE_FLAG_WILDCARD, UINT_HANDLE(rule_id));

    return BS_OK;
}

static int dpi_content_parse(IN CHAR* value, CHAR* buffer, INT length)
{
    int i = 0;
    char *start_ptr, *end_ptr, *tmp;
    char* token[64];
    int num;
    UCHAR charactor;
    int encap_len = 0;

    
    do {
        start_ptr = strchr(value, '|');
        if(!start_ptr) {
            memcpy(buffer+encap_len, value, length);
            return length;
        }

        start_ptr++;
        end_ptr = strchr(start_ptr, '|');
        if(!end_ptr) {
            memcpy(buffer+encap_len, value, length);
            return length;
        }
        *end_ptr = 0;
        if(start_ptr != value) {
            memcpy(buffer+encap_len, value, (start_ptr-value-1));
            encap_len += (start_ptr-value-1);
        }

        while((*start_ptr == ' ') && (start_ptr < end_ptr)) {
            start_ptr++;
        }

        num = TXT_StrToToken(start_ptr," ", token, 64);
        if(num <=0) {
            printf("no valid data\r\n");
            return 0;
        }
        
        for(i=0; i<num; i++) {
            tmp = TXT_Strim(token[i]);
            if(strlen(tmp) != 2) {
                printf("not valid acsii %s\r\n", tmp);
                return 0;
            }
            if(BS_OK == HEX_2_UCHAR(tmp, &charactor)) {
                
                encap_len += scnprintf(buffer+encap_len, length-encap_len, "%c", charactor);
            }
        }
        
        length = length-(end_ptr-start_ptr);
        value=end_ptr+1;
    }while(length>0);

    return encap_len;
}

static DPI_PATTERN_STR_S* dpi_add_content_pattern(IN LSTR_S* content, IN DPI_RULE_S* rule)
{
    DPI_PATTERN_STR_S *pattern = NULL;
    
    pattern = MEM_ZMalloc(sizeof(DPI_PATTERN_STR_S));

    pattern->str = MEM_ZMalloc(content->uiLen);

    pattern->length = dpi_content_parse(content->pcData, pattern->str, content->uiLen);
    
    if(!pattern->length) {
        MEM_Free(pattern->str);
        MEM_Free(pattern);
        return NULL;
    }

    rule->patterns[rule->pattern_num].pattern = pattern;
    rule->patterns[rule->pattern_num].type = DPI_PATTERN_CONTENT_TYPE;
    rule->pattern_num++;

    return pattern;
}

static DPI_PATTERN_PCRE_S * dpi_add_pcre_pattern(IN LSTR_S* pcre, IN DPI_RULE_S* rule)
{
    DPI_PATTERN_PCRE_S *pattern = NULL;
    int erroroffset;
    const char* error;

    pattern = MEM_ZMalloc(sizeof(DPI_PATTERN_PCRE_S));
    pattern->pcre = pcre_compile(pcre->pcData, 0, &error, &erroroffset, NULL);
    if(!pattern->pcre) {
        printf("pattern [%s] not pcre\r\n", pcre->pcData);
        MEM_Free(pattern);
        return NULL;
    }
    rule->patterns[rule->pattern_num].pattern = pattern;
    rule->patterns[rule->pattern_num].type = DPI_PATTERN_PCRE_TYPE;
    rule->pattern_num++;

    return pattern;
}


static int dpi_add_pcre_rule(IN CHAR *pcKey, IN CHAR *pcValue, IN HANDLE hUserHandle)
{
    USER_HANDLE_S *ud = hUserHandle;
    KV_HANDLE hKvHandle = ud->ahUserHandle[0];
    DPI_S* dpi = ud->ahUserHandle[1];
    DPI_RULE_S* rule = ud->ahUserHandle[2];
    DPI_PATTERN_PCRE_S* pattern=NULL;
    char* val_field;
    KV_HANDLE kv = 0;
    CHAR* line;
    LSTR_S pcre;
    LSTR_S value, option;
    LSTR_S lstr;
    char* offset = NULL;
    char* nocase = NULL;
    
    if(strcmp(pcKey, "pcre") != 0) {
        return 0;
    }

    
    if(rule->protocol == 0) {
        val_field = KV_GetKeyValue(hKvHandle, "protocol");
        if(!val_field) {
            printf("content rule[%s] must specify protocol\r\n", pcValue);
            return BS_STOP;
        }
        rule->protocol = TXT_Str2Ui(val_field);
    }

    if (rule->port == 0) {
        val_field = KV_GetKeyValue(hKvHandle, "port");
        if(val_field) {
            rule->port = TXT_Str2Ui(val_field);
            if (rule->protocol == IPPROTO_TCP) {
                dpi->payload_tport_map[rule->port] = 1;
            } else if (rule->protocol == IPPROTO_UDP) {
                dpi->payload_uport_map[rule->port] = 1;
            }
        }
    }

    pcre.pcData = pcValue;
    pcre.uiLen = strlen(pcValue);
    LSTR_Split(&pcre, ',', &value, &option);
    if(option.uiLen) {
        KV_PARAM_S kv_param = {0};
        kv_param.uiFlag = KV_FLAG_PERMIT_MUL_KEY;
        kv = KV_Create(&kv_param);
        line = TXT_Strim(option.pcData);
        lstr.pcData = line;
        lstr.uiLen = strlen(line);
        KV_Parse(kv, &lstr, ',', ':');
        
        
        offset = KV_GetKeyValue(kv, "offset");
        nocase = KV_GetKeyValue(kv,"nocase");
    }

    pattern = dpi_add_pcre_pattern(&value, rule);

    if(pattern) {
        pattern->offset = offset ? TXT_Str2Ui(offset) : 0; 
        pattern->nocase = nocase ? TXT_Str2Ui(nocase) : 0;
        
    }else {
        printf("no memory for add pattern[%s]\r\n", pcValue);
    }

    if(kv) {
        KV_Destory(kv);
    }

    return 0;

}

static int dpi_add_content_rule(IN CHAR *pcKey, IN CHAR *pcValue, IN HANDLE hUserHandle)
{
    USER_HANDLE_S *ud = hUserHandle;
    KV_HANDLE hKvHandle = ud->ahUserHandle[0];
    DPI_S* dpi = ud->ahUserHandle[1];
    DPI_RULE_S* rule = ud->ahUserHandle[2];
    DPI_PATTERN_STR_S* pattern=NULL;
    char* val_field;
    KV_HANDLE kv = 0;
    CHAR* line;
    LSTR_S content;
    LSTR_S value, option;
    LSTR_S lstr;
    char* offset = NULL;
    char* nocase = NULL;
    
    if(strcmp(pcKey, "content") != 0) {
        return 0;
    }

    
    if(rule->protocol == 0) {
        val_field = KV_GetKeyValue(hKvHandle, "protocol");
        if(!val_field) {        
            printf("content rule[%s] must specify protocol\r\n", pcValue);
            return BS_STOP;
        }
        rule->protocol = TXT_Str2Ui(val_field);
    }

    if (rule->port == 0) {
        val_field = KV_GetKeyValue(hKvHandle, "port");
        if(val_field) {        
            rule->port = TXT_Str2Ui(val_field);
            if (rule->protocol == IPPROTO_TCP) {
                dpi->payload_tport_map[rule->port] = 1;
            } else if (rule->protocol == IPPROTO_UDP) {
                dpi->payload_uport_map[rule->port] = 1;
            }
        }
    }

    content.pcData = pcValue;
    content.uiLen = strlen(pcValue);
    LSTR_Split(&content, ',', &value, &option);
    if(option.uiLen) {
        KV_PARAM_S kv_param = {0};
        kv_param.uiFlag = KV_FLAG_PERMIT_MUL_KEY;
        kv = KV_Create(&kv_param);
        line = TXT_Strim(option.pcData);
        lstr.pcData = line;
        lstr.uiLen = strlen(line);
        KV_Parse(kv, &lstr, ',', ':');
        
        
        offset = KV_GetKeyValue(kv, "offset");
        nocase = KV_GetKeyValue(kv,"nocase");
    }

    pattern = dpi_add_content_pattern(&value, rule);

    if(pattern) {
        pattern->offset = offset ? TXT_Str2Ui(offset) : 0; 
        pattern->nocase = nocase ? TXT_Str2Ui(nocase) : 0;
        
    }else {
        printf("no memory for add pattern[%s]\r\n", pcValue);
    }

    if(kv) {
        KV_Destory(kv);
    }

    return 0;

}

static int dpi_add_pattern2rule(IN CHAR *pcKey, IN CHAR *pcValue, IN HANDLE hUserHandle)
{
    if(strcmp(pcKey, "content") == 0) {
        dpi_add_content_rule(pcKey, pcValue, hUserHandle);
    }else if(strcmp(pcKey, "pcre") == 0) {
        dpi_add_pcre_rule(pcKey, pcValue, hUserHandle);
    }
    
    return 0;
}

static int dpi_SelectPattern_Add2ACSM(DPI_HANDLE hDpi, DPI_RULE_S* rule)
{
    int i, max_length = 0;
    DPI_PATTERN_STR_S *content = NULL;
    DPI_PATTERN_STR_S *acnode = NULL;
    DPI_S* dpi = hDpi;
    ACSMX_S* acsm;

    for(i=0; i<rule->pattern_num; i++) {
        if(rule->patterns[i].type == DPI_PATTERN_CONTENT_TYPE) {
            content = rule->patterns[i].pattern;
            if(content->length > max_length) {
                acnode = content;
            }
        }
    }

    if(rule->protocol == IPPROTO_TCP) {
        acsm = dpi->tcp_acsm;
    }else {
        acsm = dpi->udp_acsm;
    }

    if(acnode) {
        
        acnode->flag = PATTERN_COMPILE_FLAG;
        ACSMX_AddPattern(acsm, (UCHAR*)acnode->str, acnode->length, rule);
    }

    return 0;
}



int DPI_AddContent(DPI_HANDLE hDpi,  KV_HANDLE kv, DPI_RULE_S* rule)
{
    USER_HANDLE_S ud;
    ud.ahUserHandle[0] = kv;
    ud.ahUserHandle[1] = hDpi;
    ud.ahUserHandle[2] = rule;

    KV_Walk(kv, dpi_add_pattern2rule, &ud);
  
    dpi_SelectPattern_Add2ACSM(hDpi, rule);

    return 0;
}

int DPI_LoadIDNameMapFile(DPI_HANDLE hDpi, char *file, int encrypt, int isToMap)
{
    if (!isToMap) {
        return dpi_LoadMapFile(hDpi, file, dpi_LoadIdNameMap, encrypt, 0);
    }

    return 0;
}

#ifdef INTEL_HYPERSCAN
static int _dpi_hs_Compile(DPI_S* dpi)
{
    NAP_HANDLE rules_tbl = dpi->rules_tbl;
    int count;
    DPI_RULE_S *rule;
    DPI_PATTERN_S* pattern;
    
    const char** exp;
    int ele = 0;
    unsigned int *ids, *flags;
    unsigned int i,j;
    hs_compile_error_t * compile_error;
    hs_error_t error;

    if(!rules_tbl) {
        return 0;
    }
    count = NAP_GetCount(rules_tbl);
    if(count == 0) {
        return 0;
    }

    exp = MEM_ZMalloc(count * sizeof(char*));
    
    ids = MEM_ZMalloc(count * sizeof(INT));
    flags = MEM_ZMalloc(count * sizeof(INT));
    for(i=1; i<=count; i++) {
        rule = NAP_GetNodeByIndex(rules_tbl,i);
        if(rule) {
            for(j=0;j<rule->pattern_num;j++) {
                pattern = &rule->patterns[j];
                if(((DPI_PATTERN_STR_S*)(pattern->pattern))->flag == PATTERN_COMPILE_FLAG) {
                    exp[ele] = ((DPI_PATTERN_STR_S*)(pattern->pattern))->str;
                    ids[ele] = i;
                    flags[ele] = HS_FLAG_SINGLEMATCH;
                    ele++;
                }
            }
        }
    }
    

    if(ele) {
        error = hs_compile_multi(exp, flags, ids, ele, HS_MODE_BLOCK, NULL, &dpi->hs_db,  &compile_error);
        
        if (compile_error != NULL) {
            printf("hs_compile_multi() failed: %s (expression: %d)\n",
                       compile_error->message, compile_error->expression);
            hs_free_compile_error(compile_error);
        }

        if (error != HS_SUCCESS) {
            printf("hs_compile_multi() failed: error %d\n", error);
        }else {
            error = hs_alloc_scratch(dpi->hs_db, &dpi->scratch);
            if(error != HS_SUCCESS) {
            printf("alloc scratch fail\r\n");
            }
        }
    }

    MEM_Free(exp);
    MEM_Free(ids);
    MEM_Free(flags);
    
    return 0;
}
#endif

int DPI_LoadRuleFile(DPI_HANDLE hDpi, char *file, int encrypt, UINT baseid, int isToMap)
{
    int ret = 0;
    DPI_S* dpi = hDpi;

    if (!isToMap) {
        ret = dpi_LoadMapFile(dpi, file, dpi_LoadRuleLine, encrypt, baseid);
    }

    if (ret == BS_OK) {
        if (dpi->tcp_acsm) {
            ret = ACSMX_Compile(dpi->tcp_acsm);
        } 

        if (dpi->udp_acsm) {
            ret = ACSMX_Compile(dpi->udp_acsm);
        }
    }

#if 0
    if(BS_OK == ret) {
#ifdef INTEL_HYPERSCAN
        ret = _dpi_hs_Compile(dpi);
#else
        ret = ACSMX_Compile(dpi->tcp_acsm);
#endif
    }
#endif
    return ret;
}

DPI_RULE_S* DPI_MatchIP(DPI_HANDLE hDpi, UCHAR protocol, UINT ip, USHORT port)
{
    DPI_S *dpi = hDpi;
    UINT rule_id = dpi_MatchRuleByIP(dpi, protocol, ip, port);
    if (! rule_id) {
        return NULL;
    }
    return NAP_GetNodeByIndex(dpi->rules_tbl, rule_id);
}

DPI_RULE_S * DPI_MatchHostname(DPI_HANDLE hDpi, char *hostname)
{
    DPI_S *dpi = hDpi;
    UINT rule_id = dpi_MatchRuleByHostname(dpi, hostname);
    if (! rule_id) {
        return NULL;
    }

    return NAP_GetNodeByIndex(dpi->rules_tbl, rule_id);
}

DPI_RULE_S* DPI_MatchProtocol(DPI_HANDLE hDpi, UCHAR protocol)
{
    DPI_S *dpi = hDpi;
    UINT rule_id = dpi_MatchRuleByProtocol(dpi, protocol);
    if (! rule_id) {
        return NULL;
    }
    return NAP_GetNodeByIndex(dpi->rules_tbl, rule_id);
}

static int dpi_CheckContentPattern(DPI_PATTERN_STR_S* pattern, int offset, UCHAR *data, UINT len)
{
    if (pattern->offset) {
        if ((pattern->offset != offset) || (pattern->offset + pattern->length > len) ||
                memcmp(data + pattern->offset, pattern->str, pattern->length)) {
            return 0;
        }
    }else {
        if (!Sunday_Search(data, len, (UCHAR*)pattern->str, pattern->length)) {
            return 0;
        }
    }

    return 1;
}

static int dpi_CheckPcrePattern(DPI_PATTERN_PCRE_S* pattern, UCHAR *data, UINT len)
{
    int rc;
    int ovector[3];

    if((rc=pcre_exec(pattern->pcre, NULL, (const char*)data, len, 0, 0, ovector, 3)) <=0) {
        return 0;
    }

    return 1;
}

static int dpi_MatchRule(DPI_RULE_S* rule, int offset, UCHAR *data, UINT len)
{
    int i;

    for(i=0; i<rule->pattern_num; i++) {
        switch(rule->patterns[i].type) {
            case DPI_PATTERN_CONTENT_TYPE:
                if(dpi_CheckContentPattern(rule->patterns[i].pattern, offset, data, len) == 0) {
                    return 0;
                }
                break;
            case DPI_PATTERN_PCRE_TYPE:
                if(dpi_CheckPcrePattern(rule->patterns[i].pattern, data, len) == 0) {
                    return 0;
                }
                break;
            default:
                BS_DBGASSERT(0);
                return 0;
        }
    }

    return 1;
}

static int dpi_MatchFound(void* id, int index, void *data)
{
    ACSMX_PATTERN_S* acmatch = id;
    DPI_RULE_S* rule;
    DPI_ACSEARCH_DATA_S *ud = data;


    for(; acmatch; acmatch=acmatch->next) {
        rule = acmatch->udata->id;
        
#if 0
        if (rule->protocol && (rule->protocol != rule->protocol)) {
            continue;
        }
#endif

        if (rule->port && (ud->port != rule->port)) {
            continue;
        }
        if (dpi_MatchRule(rule, index, ud->data, ud->len)) {
            if (! ud->best_rule) {
                ud->best_rule = rule;
            } else if (ud->best_rule->id < rule->id) {
                ud->best_rule = rule;
            }
        }
    }

    return 0;
}

VOID * DPI_MatchContent(DPI_HANDLE hdpi, UCHAR protocol, USHORT port, UCHAR* data, int len)
{    
    DPI_S* dpi = hdpi;
    ACSMX_S* acsm = NULL;
    int start_state = 0;
    DPI_ACSEARCH_DATA_S ud = {0};

    if(protocol == IPPROTO_TCP) {
        acsm = dpi->tcp_acsm;
    }else if(protocol == IPPROTO_UDP) {
        acsm = dpi->udp_acsm;
    }

    if(acsm) {
        ud.data = data;
        ud.len = len;
        ud.protocol = protocol;
        ud.port = port;
        ACSMX_Search(acsm, data, len, dpi_MatchFound, &ud, &start_state);
    }

    return ud.best_rule;
}

#ifdef INTEL_HYPERSCAN
static int dpi_MatchFound_HS(unsigned int id, unsigned long long from,
                             unsigned long long to, unsigned int flags, void *ctx)
{
    DPI_ACSEARCH_DATA_S* userdata = ctx;
    DPI_S* dpi = userdata->hDpi;

    userdata->best_rule = NAP_GetNodeByIndex(dpi->rules_tbl, id);

    printf("hs match id=%d\r\n", id);
    return 1; 
}

VOID* DPI_MatchContent_HS(DPI_HANDLE hdpi, UCHAR protocol, USHORT port, UCHAR* data, int len)
{
    hs_error_t err;
    DPI_S* dpi = hdpi;
    DPI_ACSEARCH_DATA_S ud = {0};

    ud.data = data;
    ud.len = len;
    ud.hDpi = hdpi;
    ud.protocol = protocol;
    ud.port = port;
    err = hs_scan(dpi->hs_db, data, len, 0, dpi->scratch, dpi_MatchFound_HS, &ud);

    return ud.best_rule;
}

#endif

BOOL_T DPI_Is_Content_Inspected_By_Port(DPI_HANDLE hDpi, UCHAR protocol, USHORT port)
{
    DPI_S *dpi = (DPI_S*) hDpi;

    if (protocol == IPPROTO_TCP) {
        return dpi->payload_tport_map[port];
    } else if (protocol == IPPROTO_UDP) {
        return dpi->payload_uport_map[port];
    }

    return 0;
}
