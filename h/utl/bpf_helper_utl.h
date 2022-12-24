/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _BPF_HELPER_UTL_H
#define _BPF_HELPER_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

#define BPF_BASE_HELPER_MAX 256
#define BPF_USER_HELPER_COUNT 256
#define BPF_USER_HELPER_MIN 10000
#define BPF_USER_HELPER_MAX (BPF_USER_HELPER_MIN + BPF_USER_HELPER_COUNT)

typedef UINT64 (*PF_BPF_HELPER_FUNC)(UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5);

extern const void * g_bpf_base_helpers[BPF_BASE_HELPER_MAX];
extern void * g_bpf_user_helpers[BPF_USER_HELPER_COUNT];

void * BpfHelper_BaseHelper();
void * BpfHelper_UserHelper();

UINT BpfHelper_BaseSize();
UINT BpfHelper_UserSize();

PF_BPF_HELPER_FUNC BpfHelper_GetFunc(UINT id);
int BpfHelper_SetUserFunc(UINT id, void *func);

#ifdef __cplusplus
}
#endif
#endif //BPF_HELPER_UTL_H_
