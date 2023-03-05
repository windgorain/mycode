/*================================================================
*   Created by LiXingang
*   Description: 令牌桶. 限速用
*
================================================================*/
#include "bs.h"
#include "utl/rdtsc_utl.h"
#include "utl/token_bucket.h"

void TokenBucket_Init(INOUT TOKEN_BUCKET_S *bucket, uint64_t burst_limit, uint64_t speed)
{
    memset(bucket, 0, sizeof(TOKEN_BUCKET_S));
    bucket->burst_limit = burst_limit;
    bucket->speed = speed;
}

/* 返回获得多少tocken */
uint64_t TokenBucket_Acquire(TOKEN_BUCKET_S *bucket, uint64_t n)
{
    uint64_t now = RDTSC_Get();
    uint64_t count;

    if (now - bucket->ts >= RDTSC_HZ) {
        bucket->token_count = bucket->burst_limit;
    } else {
        bucket->token_count += (now - bucket->ts) * bucket->speed / RDTSC_HZ;
        if (bucket->token_count > bucket->burst_limit) {
            bucket->token_count = bucket->burst_limit;
        }
    }

    bucket->ts = now;
    count = MIN(n, bucket->token_count);
    bucket->token_count -= count;

    return count;
}


