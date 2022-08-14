/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _MUC_CORE_H
#define _MUC_CORE_H
#ifdef __cplusplus
extern "C"
{
#endif

#define MUC_DESC_LEN 127

typedef struct {
    UINT used:1;
    UINT start:1;
    UINT restored:1; /* muc是否已经配置恢复 */
    int id;
    char description[MUC_DESC_LEN + 1];
    void *user_data[MUC_UD_MAX];
}MUC_S;

int MucCore_Init(void *env);
int MucCore_Create(int muc_id);
int MucCore_Destroy(int muc_id);
MUC_S * MucCore_Get(int id);
int MucCore_Start(MUC_S *muc);
int MucCore_Stop(MUC_S *muc);
int MucCore_EnterCmd(MUC_S *muc, void *env);

#ifdef __cplusplus
}
#endif
#endif //MUC_CORE_H_
