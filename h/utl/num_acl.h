/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _NUM_ACL_H
#define _NUM_ACL_H
#ifdef __cplusplus
extern "C"
{
#endif

#define NUM_ACL_UNDEF  0  /* not defined */
#define NUM_ACL_PERMIT 1
#define NUM_ACL_DENY   2
#define NUM_ACL_BYPASS 3

typedef struct {
    UINT size; /* data数组的个数 */
    UCHAR *data;
}NUM_ACL_S;

void NumAcl_Init(NUM_ACL_S *num_acl, UCHAR *data, UINT size);
int NumAcl_ParseFile(NUM_ACL_S *num_acl, char *file);
int NumAcl_Match(NUM_ACL_S *num_acl, UINT num);

#ifdef __cplusplus
}
#endif
#endif //NUM_ACL_H_
