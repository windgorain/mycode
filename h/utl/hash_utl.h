/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-20
* Description: 
* History:     
******************************************************************************/

#ifndef __HASH_UTL_H_
#define __HASH_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif 

typedef HANDLE HASH_HANDLE;

typedef struct
{
    DLL_NODE_S stNode;
    UINT hash_factor;
}HASH_NODE_S;

typedef UINT (*PF_HASH_INDEX_FUNC)(IN VOID *pstHashNode); 
typedef INT  (*PF_HASH_CMP_FUNC)(IN VOID * pstHashNode, IN VOID * pstNodeToFind);
typedef int (*PF_HASH_WALK_FUNC)(IN HASH_HANDLE hHashId, IN VOID *pstNode, IN VOID * pUserHandle);
typedef VOID  (*PF_HASH_FREE_FUNC)(IN HASH_HANDLE hHashId, IN VOID *pstHashNode, IN VOID * pUserHandle);

HASH_HANDLE HASH_CreateInstance(void *memcap, IN UINT ulHashBucketNum, IN PF_HASH_INDEX_FUNC pfFunc);
VOID HASH_DestoryInstance(IN HASH_HANDLE hHashId);
void HASH_SetResizeWatter(HASH_HANDLE hHashId, UINT high_watter_percent, UINT low_watter_percent);
void HASH_AddWithFactor(IN HASH_HANDLE hHashId, IN VOID *pstNode, UINT hash_factor);
VOID HASH_Add(IN HASH_HANDLE hHashId, IN VOID *pstNode);
VOID HASH_Del(IN HASH_HANDLE hHashId, IN VOID *pstNode);
VOID HASH_DelAll(IN HASH_HANDLE hHashId, IN PF_HASH_FREE_FUNC pfFreeFunc, IN VOID *pUserHandle);
VOID * HASH_FindWithFactor(IN HASH_HANDLE hHashId, UINT hash_factor, IN PF_HASH_CMP_FUNC pfCmpFunc, IN VOID *pstNodeToFind);
VOID * HASH_Find(IN HASH_HANDLE hHashId, IN PF_HASH_CMP_FUNC pfCmpFunc, IN VOID *pstNodeToFind);
UINT HASH_Count(IN HASH_HANDLE hHashId);
VOID HASH_Walk(IN HASH_HANDLE hHashId, IN PF_HASH_WALK_FUNC pfWalkFunc, IN VOID * pUserHandle);

HASH_NODE_S * HASH_GetNext(HASH_HANDLE hHash, HASH_NODE_S *curr_node );

HASH_NODE_S * HASH_GetNextDict(HASH_HANDLE hHash, PF_HASH_CMP_FUNC pfCmpFunc, HASH_NODE_S *curr_node );

#ifdef __cplusplus
    }
#endif 

#endif 


