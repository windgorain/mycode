/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/bitmap_utl.h"
#include "utl/cff_utl.h"
#include "utl/match_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"

static BOOL_T ipmatch_Match(void *pattern_in, void *key_in)
{
    IP_MATCH_PATTERN_S *pattern = pattern_in;
    IP_MATCH_KEY_S *key = key_in;

    if (pattern->dip != (key->dip & pattern->dip_mask)) {
        return FALSE;
    }
    if (pattern->sip != (key->sip & pattern->sip_mask)) {
        return FALSE;
    }
    if ((pattern->dport != 0) && (pattern->dport != key->dport)) {
        return FALSE;
    }
    if ((pattern->sport != 0) && (pattern->sport != key->sport)) {
        return FALSE;
    }
    if ((pattern->protocol != 0) && (pattern->protocol != key->protocol)) {
        return FALSE;
    }

    return TRUE;
}

MATCH_HANDLE IPMatch_Create(UINT max)
{
    return Match_Create(max, sizeof(IP_MATCH_PATTERN_S), ipmatch_Match);
}

static void ipmatch_ConfByFileEach(MATCH_HANDLE head,
        CFF_HANDLE hCff, char *tag)
{
    int var;
    char *string;
    IP_MAKS_S ipmask;
    IP_MATCH_PATTERN_S pattern = {0};

    int index = 0;
    CFF_GetPropAsInt(hCff, tag, "index", &index);

    var = 0;
    CFF_GetPropAsInt(hCff, tag, "protocol", &var);
    pattern.protocol = var;

    var = 0;
    CFF_GetPropAsInt(hCff, tag, "sport", &var);
    pattern.sport = var;

    var = 0;
    CFF_GetPropAsInt(hCff, tag, "dport", &var);
    pattern.dport = var;

    string = NULL;
    CFF_GetPropAsString(hCff, tag, "sip", &string);
    if (string) {
        IPString_ParseIpMask(string, &ipmask);
        pattern.sip = ipmask.uiIP;
        pattern.sip_mask = ipmask.uiMask;
    }

    string = NULL;
    CFF_GetPropAsString(hCff, tag, "dip", &string);
    if (string) {
        IPString_ParseIpMask(string, &ipmask);
        pattern.dip = ipmask.uiIP;
        pattern.dip_mask = ipmask.uiMask;
    }

    Match_SetPattern(head, index, &pattern);
}

void IPMatch_LoadConfig(MATCH_HANDLE head, char *file)
{
    CFF_HANDLE hCff;

    hCff = CFF_INI_Open(file, 0);
    if (NULL == hCff) {
        return;
    }

    char *tag;
    CFF_SCAN_TAG_START(hCff, tag) {
        ipmatch_ConfByFileEach(head, hCff, tag);
    }CFF_SCAN_END();

    CFF_Close(hCff);
}

