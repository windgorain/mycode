/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ULC_VERIFIER_H
#define _ULC_VERIFIER_H
#ifdef __cplusplus
extern "C"
{
#endif

int ULC_VERIFIER_ReplaceMapFdWithMapPtr(ULC_PROG_NODE_S *prog);
int ULC_VERIFIER_ConvertCtxAccess(ULC_PROG_NODE_S *prog);
int ULC_VERIFIER_FixupBpfCalls(ULC_PROG_NODE_S *prog);

#ifdef __cplusplus
}
#endif
#endif //ULC_VERIFIER_H_
