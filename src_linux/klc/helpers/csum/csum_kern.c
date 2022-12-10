/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klc/klc_base.h"
#include "utl/csum_utl.h"
#include "helpers/csum_klc.h"

#define KLC_MODULE_NAME CSUM_MODULE_NAME
KLC_DEF_MODULE();

/* 查找一致性hash的虚拟节点 */
SEC_NAME_FUNC(CSUM_IP_HEALDER)
USHORT csum_ip_header(void *hdr, int hdr_len)
{
    return CSUM_IpHeader(hdr, hdr_len);
}


