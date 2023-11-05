/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/dns_utl.h"
#include "utl/box_utl.h"
#include "utl/ip_utl.h"
#include "utl/udp_utl.h"
#include "utl/socket_utl.h"
#include "comp/comp_logcenter.h"

static void _dns_parse_domain_ip(DNS_WALK_QR_S *qr, void *ud)
{
    USER_HANDLE_S *uh = ud;
    DNS_IP_DOMAINNAME_S *domain_ip = uh->ahUserHandle[0];
    UINT *ip;
    char *type = "dnsparse";
    char *desc = "ip counters out of range ";
    char str[256] = {0};

    if (qr->rrtype == DNS_RRTYPE_QUERY) {
        DNS_GetRRDomainName((void*)qr->dns_header, qr->dns_pkt_len, qr->rr_data, domain_ip->domain_name);
        return;
    }

    if (qr->rrtype != DNS_RRTYPE_ANSWER) {
        return;
    }

    DNS_RR_S *rr = qr->rr_struct;

    if (rr->usType != htons(DNS_TYPE_A)) {
        return;
    }

    if (rr->usRDLen != htons(4)) {
        return;
    }

    if (domain_ip->ip_num >= DNS_IP_MAX) {
        scnprintf(str, sizeof(str),"{\"type\":\"%s\",\"description\":\"%s%d\"}\n", type,desc,DNS_IP_MAX);
        LogCenter_OutputString(0, str);
        return;
    }

    ip = (void*)(rr + 1);

    domain_ip->ips[domain_ip->ip_num] = *ip; 
    domain_ip->ip_num ++;

    return;
}


int DNS_ParseDomainNameIP(DNS_HEADER_S *pstDnsHeader, UINT uiDataLen, OUT DNS_IP_DOMAINNAME_S *domain_ip)
{
    USER_HANDLE_S ud;

    ud.ahUserHandle[0] = domain_ip;

	USHORT isRespones = pstDnsHeader->usQR;  
	USHORT usRcode = pstDnsHeader->usRcode;  
	USHORT usOpcode = pstDnsHeader->usOpcode; 

	if(DNS_FLAG_RESPONES != isRespones) {
		
        RETURN(BS_ERR);
	}

	if(DNS_FLAG_OPCODE_STANDARY != usOpcode) {
		
        RETURN(BS_ERR);
	}

	if(DNS_FLAG_REPLY_CODE != usRcode) {
		
        RETURN(BS_ERR);
	}

    DNS_WalkQR(pstDnsHeader, uiDataLen, _dns_parse_domain_ip, &ud);

    return 0;
}
