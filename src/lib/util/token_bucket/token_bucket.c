/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/rdtsc_utl.h"
#include "utl/token_bucket.h"

void TokenBucket_Init(INOUT TOKEN_BUCKET_S *bucket, UINT max_count)
{
    memset(bucket, 0, sizeof(TOKEN_BUCKET_S));
    bucket->max_count = max_count;
}

/* 成功返回TRUE, 失败返回FALSE */
BOOL_T TokenBucket_Acquire(TOKEN_BUCKET_S *bucket, UINT n)
{
    UINT64 now = RDTSC_Get();

    if (now - bucket->ts >= RDTSC_HZ) {
        bucket->count = bucket->max_count;
    } else {
        bucket->count += (now - bucket->ts) * bucket->max_count / RDTSC_HZ;
        if (bucket->count > bucket->max_count) {
            bucket->count = bucket->max_count;
        }
    }

    bucket->ts = now;

    if (bucket->count < n) {
        return FALSE;
    }

    bucket->count -= n;

    return TRUE;
}


