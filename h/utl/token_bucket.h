/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _TOKEN_BUCKET_H
#define _TOKEN_BUCKET_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    uint64_t burst_limit; /* 最大bust数目 */
    uint64_t speed;       /* 限速 */
    uint64_t token_count; /* 令牌数目 */
    uint64_t ts;      /* 上次添加令牌的时间 */
}TOKEN_BUCKET_S;

void TokenBucket_Init(INOUT TOKEN_BUCKET_S *bucket, uint64_t burst, uint64_t speed);
uint64_t TokenBucket_Acquire(TOKEN_BUCKET_S *bucket, uint64_t n);

#ifdef __cplusplus
}
#endif
#endif //TOKEN_BUCKET_H_
