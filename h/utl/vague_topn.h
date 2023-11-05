/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _VAGUE_TOPN_H
#define _VAGUE_TOPN_H

#include "utl/box_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define VAGUE_TOPN_LEVEL_NUM 3

typedef unsigned int (*PF_VAGUE_TOPN_HASH_FUNC)(int level, void *key);

typedef struct {
    unsigned int freq;
    unsigned int score;
    unsigned int hash_factor;
    unsigned char epoch;
    unsigned char key[0];
}VAGUE_TOPN_NODE_S;

typedef struct {
    int key_size;
    int n; 
    int current_num; 
    VAGUE_TOPN_NODE_S *nodes;
    BOX_S box;
}VAGUE_TOPN_RESULT_S;

typedef struct {
    unsigned int hash_size; 
    unsigned int mask; 
    unsigned int pass_line; 
    unsigned int epoch:8;
    unsigned int *hash[VAGUE_TOPN_LEVEL_NUM]; 
    VAGUE_TOPN_RESULT_S result;
}VAGUE_TOPN_S;

#define VAGUE_TOPN_RESULT_NODE_SIZE(_key_size) (sizeof(VAGUE_TOPN_NODE_S) + (_key_size))
#define VAGUE_TOPN_RESULT_MEM_SIZE(_n,_key_size) (VAGUE_TOPN_RESULT_NODE_SIZE(_key_size) * (_n))
#define VAGUE_TOPN_CTRL_MEM_SIZE(_hash_size) (sizeof(unsigned int)*(_hash_size)*VAGUE_TOPN_LEVEL_NUM)

#define VAGUE_TOPN_RESULT_CURRENT_NUM(_result) ((_result)->current_num)

int VagueTopn_Create(INOUT VAGUE_TOPN_S *topn,
        int hash_size, int key_size, int n);
void VagueTopn_Destroy(VAGUE_TOPN_S *topn);
void VagueTopn_Reset(VAGUE_TOPN_S *topn);
void VagueTopn_Score(VAGUE_TOPN_S *topn, void *key, int score, UINT hash_factor);
VAGUE_TOPN_NODE_S * VagueTopn_GetByIndex(VAGUE_TOPN_S *topn, int index);

#ifdef __cplusplus
}
#endif
#endif 
