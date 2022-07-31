/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _KLCKO_OSHELPER_H
#define _KLCKO_OSHELPER_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    long long err; /* 当func不存在时,返回的err值 */
    void *sym;
}KLCKO_SYM_S;

void * KlcKoOsHelper_GetHelper(int helper_id);
u64 KlcKoOsHelper_GetHelperErr(int helper_id);
int KlcKoOsHelper_SetHelper(int helper_id, void *func, u64 err);
int KlcKoOsHelper_Init(void);

#ifdef __cplusplus
}
#endif
#endif //KLCKO_OSHELPER_H_
