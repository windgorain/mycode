/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _PWATCHER_MUC_H
#define _PWATCHER_MUC_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    RCU_NODE_S rcu_node;
    void *zone;
}PWATCHER_MUC_S;

int PWatcherMuc_Init();
PWATCHER_MUC_S * PWatcherMuc_Get(int muc_id);

#ifdef __cplusplus
}
#endif
#endif 
