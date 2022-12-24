/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/cff_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/socket_utl.h"
#include "utl/plug_utl.h"
#include "utl/jhash_utl.h"
#include "../h/pwatcher_session.h"
#include "../h/pwatcher_def.h"
#include "../h/pwatcher_event.h"

typedef struct {
    UCHAR protocol;
    USHORT sport;
    USHORT dport;
    UINT sip;
    UINT dip;
}PWATCHER_PKT_HASH1_S;

/* 五元组的hash */
UINT PWatcher_GetPktHash1(PWATCHER_PKT_DESC_S *pkt)
{
    PWATCHER_PKT_HASH1_S key;

    if (pkt->hash1) {
        return pkt->hash1;
    }

    memset(&key, 0, sizeof(key));

    key.protocol = pkt->ip_protocol;
    key.sport = PWatcher_GetPktSrcPort(pkt);
    key.dport = PWatcher_GetPktDstPort(pkt);
    if (pkt->ipv6) {
        UINT *sip = PWatcher_GetPktSrcIp(pkt);
        UINT *dip = PWatcher_GetPktDstIp(pkt);
        key.sip = sip[0] ^ sip[1] ^ sip[2] ^ sip[3];
        key.dip = dip[0] ^ dip[1] ^ dip[2] ^ dip[3];
    } else {
        key.sip = PWatcher_GetPktSrcIp4(pkt);
        key.dip = PWatcher_GetPktDstIp4(pkt);
    }

    pkt->hash1 = JHASH_GeneralBuffer(&key, sizeof(key), 0);
    if (pkt->hash1 == 0) {
        pkt->hash1 = 0x80000000;
    }

    return pkt->hash1;
}

typedef struct {
    UCHAR protocol;
    USHORT port;
    UINT ip;
}PWATCHER_PKT_HASH2_S;

/* 五元组的正反一致的hash */
UINT PWatcher_GetPktHash2(PWATCHER_PKT_DESC_S *pkt)
{
    PWATCHER_PKT_HASH2_S key;

    if (pkt->hash2) {
        return pkt->hash2;
    }

    memset(&key, 0, sizeof(key));

    key.protocol = pkt->ip_protocol;
    key.port = PWatcher_GetPktDstPort(pkt) ^ PWatcher_GetPktSrcPort(pkt);
    if (pkt->ipv6) {
        UINT *sip = PWatcher_GetPktSrcIp(pkt);
        UINT *dip = PWatcher_GetPktDstIp(pkt);
        key.ip = sip[0] ^ sip[1] ^ sip[2] ^ sip[3] ^ dip[0] ^ dip[1] ^ dip[2] ^ dip[3];
    } else {
        key.ip = PWatcher_GetPktDstIp4(pkt) ^ PWatcher_GetPktSrcIp4(pkt);;
    }

    pkt->hash2 = JHASH_GeneralBuffer(&key, sizeof(key), 0);
    if (pkt->hash2 == 0) {
        pkt->hash2 = 0x80000000;
    }

    return pkt->hash2;
}

