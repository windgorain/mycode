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
    UINT max_count; /* 最大令牌数目 */
    UINT count;     /* 当前令牌数目 */
    UINT reserved;
    UINT64 ts;      /* 上次添加令牌的时间 */
}TOKEN_BUCKET_S;

void TokenBucket_Init(INOUT TOKEN_BUCKET_S *bucket, UINT max_count);

/* 成功返回TRUE, 失败返回FALSE */
BOOL_T TokenBucket_Acquire(TOKEN_BUCKET_S *bucket, UINT n);

#ifdef __cplusplus
}
#endif
#endif //TOKEN_BUCKET_H_
