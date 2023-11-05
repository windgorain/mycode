/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2020-07-15
* Description: 
* History:     
******************************************************************************/

#ifndef __HIREDIS_READ_H
#define __HIREDIS_READ_H
#include <stdio.h> 

#define REDIS_ERR -1
#define REDIS_OK 0


#define REDIS_ERR_IO 1 
#define REDIS_ERR_EOF 3 
#define REDIS_ERR_PROTOCOL 4 
#define REDIS_ERR_OOM 5 
#define REDIS_ERR_TIMEOUT 6 
#define REDIS_ERR_OTHER 2 

#define REDIS_REPLY_STRING 1
#define REDIS_REPLY_ARRAY 2
#define REDIS_REPLY_INTEGER 3
#define REDIS_REPLY_NIL 4
#define REDIS_REPLY_STATUS 5
#define REDIS_REPLY_ERROR 6
#define REDIS_REPLY_DOUBLE 7
#define REDIS_REPLY_BOOL 8
#define REDIS_REPLY_MAP 9
#define REDIS_REPLY_SET 10
#define REDIS_REPLY_ATTR 11
#define REDIS_REPLY_PUSH 12
#define REDIS_REPLY_BIGNUM 13
#define REDIS_REPLY_VERB 14


#define REDIS_READER_MAX_BUF (1024*16)


#define REDIS_READER_MAX_ARRAY_ELEMENTS ((1LL<<32) - 1)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct redisReadTask {
    int type;
    long long elements; 
    int idx; 
    void *obj; 
    struct redisReadTask *parent; 
    void *privdata; 
} redisReadTask;

typedef struct redisReplyObjectFunctions {
    void *(*createString)(const redisReadTask*, char*, size_t);
    void *(*createArray)(const redisReadTask*, size_t);
    void *(*createInteger)(const redisReadTask*, long long);
    void *(*createDouble)(const redisReadTask*, double, char*, size_t);
    void *(*createNil)(const redisReadTask*);
    void *(*createBool)(const redisReadTask*, int);
    void (*freeObject)(void*);
} redisReplyObjectFunctions;

typedef struct redisReader {
    int err; 
    char errstr[128]; 

    char *buf; 
    size_t pos; 
    size_t len; 
    size_t maxbuf; 
    long long maxelements; 

    redisReadTask **task;
    int tasks;

    int ridx; 
    void *reply; 

    redisReplyObjectFunctions *fn;
    void *privdata;
} redisReader;


redisReader *redisReaderCreateWithFunctions(redisReplyObjectFunctions *fn);
void redisReaderFree(redisReader *r);
int redisReaderFeed(redisReader *r, const char *buf, size_t len);
int redisReaderGetReply(redisReader *r, void **reply);

#define redisReaderSetPrivdata(_r, _p) (int)(((redisReader*)(_r))->privdata = (_p))
#define redisReaderGetObject(_r) (((redisReader*)(_r))->reply)
#define redisReaderGetError(_r) (((redisReader*)(_r))->errstr)

#ifdef __cplusplus
}
#endif

#endif
