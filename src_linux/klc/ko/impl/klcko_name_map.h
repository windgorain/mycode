/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _KLCKO_NAME_MAP_H
#define _KLCKO_NAME_MAP_H
#ifdef __cplusplus
extern "C"
{
#endif

void * KlcKoNameMap_Get(char *name);
void KlcKoNameMap_DelModule(char *module_prefix);

#ifdef __cplusplus
}
#endif
#endif //KLCKO_NAME_MAP_H_