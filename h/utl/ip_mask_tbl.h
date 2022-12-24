/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _IP_MASK_TBL_H
#define _IP_MASK_TBL_H

#include "utl/hash_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    HASH_NODE_S stHashNode;
    UINT ip; /*net order*/
    UCHAR depth;
}IPMASKTBL_NODE_S;

typedef struct
{
    HASH_HANDLE hHash;
}IPMASKTBL_S;

typedef struct {
    UINT bf[1024*1024*2]; /* 每个位表示一个24位掩码,如果设置为1则表示其或其子网可能在tbl中存在,否则不存在 */
}IPMASKTBL_BF_S;

typedef BS_WALK_RET_E (*PF_IPMASKTBL_WALK_FUNC)(IN IPMASKTBL_NODE_S *pstIpMaskTblNode, IN HANDLE hUserHandle);
typedef void (*PF_IPMASKTBL_MERGE_USER_DATA)(IPMASKTBL_NODE_S *node_to, IPMASKTBL_NODE_S *node_from);

int IPMASKTBL_Init(IN IPMASKTBL_S *ipmasktbl);
VOID IPMASKTBL_Fini(IN IPMASKTBL_S *ipmasktbl, PF_HASH_FREE_FUNC pfFree, void *user_handle);
BS_STATUS IPMASKTBL_Add(IN IPMASKTBL_S *ipmasktbl, IN IPMASKTBL_NODE_S *pstIpMaskTblNode);
IPMASKTBL_NODE_S * IPMASKTBL_Get(IN IPMASKTBL_S *ipmasktbl, UINT ip/*netorder*/, UCHAR depth);
VOID IPMASKTBL_Del(IN IPMASKTBL_S *ipmasktbl, IN IPMASKTBL_NODE_S *pstNode);
VOID IPMASKTBL_DelAll(IN IPMASKTBL_S *ipmasktbl, PF_HASH_FREE_FUNC pfFree, void *user_handle);
IPMASKTBL_NODE_S * IPMASKTBL_Match(IN IPMASKTBL_S *ipmasktbl, IN UINT uiDstIp /*net order*/);
IPMASKTBL_NODE_S * IPMASKTBL_BfMatch(IN IPMASKTBL_S *ipmasktbl, IPMASKTBL_BF_S *ipmasktbl_bf, IN UINT uiDstIp /*net order*/);
VOID IPMASKTBL_Walk(IN IPMASKTBL_S *ipmasktbl, IN PF_IPMASKTBL_WALK_FUNC pfWalkFunc, IN HANDLE hUserHandle);
/* 字典序方式访问下一个 */
IPMASKTBL_NODE_S * IPMASKTBL_GetNext( IN IPMASKTBL_S *ipmasktbl, IN IPMASKTBL_NODE_S *pstFibCurrent/* 如果为NULL表示获取第一个 */);
/* 遍历合并每个通配子集的user_data */
void IPMASKTBL_MergeUserData(IPMASKTBL_S *ipmasktbl, PF_IPMASKTBL_MERGE_USER_DATA pfFunc);

void IPMASKTBL_BfInit(IPMASKTBL_BF_S *ipmasktbl_bf);
void IPMASKTBL_BfSet(IPMASKTBL_BF_S *ipmasktbl_bf, UINT ip/*netorder*/, UCHAR depth);
int IPMASKTBL_BfTest(IPMASKTBL_BF_S *ipmasktbl_bf, UINT ip/*netorder*/, UCHAR depth);

#ifdef __cplusplus
}
#endif
#endif //IP_MASK_TBL_H_
