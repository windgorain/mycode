/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/bitmap_utl.h"
#include "utl/match_utl.h"

static BOOL_T uintmatch_Match(void *pattern_in, void *key_in)
{
    UINT *pattern = pattern_in;
    UINT *key = key_in;

    if (*pattern != *key) {
        return FALSE;
    }

    return TRUE;
}

MATCH_HANDLE UintMatch_Create(UINT max)
{
    return Match_Create(max, sizeof(UINT), uintmatch_Match);
}

