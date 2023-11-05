/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _BRIDGE_MUC_H
#define _BRIDGE_MUC_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    RCU_NODE_S rcu_node;
    void *bridge_ctrl;
}BRIDGE_MUC_S;

int BridgeMuc_Init();
BRIDGE_MUC_S * BridgeMuc_Get(int muc_id);

#ifdef __cplusplus
}
#endif
#endif 
