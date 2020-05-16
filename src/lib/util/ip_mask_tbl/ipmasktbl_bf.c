/*================================================================
*   Created by LiXingang
*   Description: ip mask tbl bloom filter, 每个位表示一个24位网段
*                用于快速判断ip/mask是否可能存在
================================================================*/
#include "bs.h"
#include "utl/bit_opt.h"
#include "utl/net.h"
#include "utl/ip_mask_tbl.h"

void IPMASKTBL_BfInit(IPMASKTBL_BF_S *ipmasktbl_bf)
{
    memset(ipmasktbl_bf, 0, sizeof(IPMASKTBL_BF_S));
}

void IPMASKTBL_BfSet(IPMASKTBL_BF_S *ipmasktbl_bf, UINT ip/*netorder*/, UCHAR depth)
{
    UINT mask;
    UINT start, end;
    UINT i;

    if (depth > 24) {
        depth = 24;
    }

    mask = PREFIX_2_MASK(depth);
    ip = ntohl(ip);
    ip = ip & mask;

    IpMask_2_Range(ip, mask, &start, &end);

    start >>= 8;
    end >>= 8;

    for (i=start; i<=end; i++) {
        ArrayBit_Set(ipmasktbl_bf->bf, i);
    }
}

int IPMASKTBL_BfTest(IPMASKTBL_BF_S *ipmasktbl_bf, UINT ip/*netorder*/, UCHAR depth)
{
    UINT mask;
    UINT start, end;

    if (depth > 24) {
        depth = 24;
    }

    mask = PREFIX_2_MASK(depth);
    ip = ntohl(ip);
    ip = ip & mask;

    IpMask_2_Range(ip, mask, &start, &end);

    start >>= 8;
    end >>= 8;

    if (! ArrayBit_Test(ipmasktbl_bf->bf, start)) {
        return 0;
    }

    if (start == end) {
        return 1;
    }

    return ArrayBit_Test(ipmasktbl_bf->bf, end);
}


