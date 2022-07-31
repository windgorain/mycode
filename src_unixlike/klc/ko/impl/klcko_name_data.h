/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _KLCKO_NAME_DATA_H
#define _KLCKO_NAME_DATA_H
#ifdef __cplusplus
extern "C"
{
#endif

/* 如果存在则直接获取,否则申请一个 */
void * KlcKoNameData_FindOrAlloc(char *name, unsigned int size);
/* 申请一个 */
void * KlcKoNameData_Alloc(char *name, unsigned int size);
int KlcKoNameData_FreeByName(char *name);
int KlcKoNameData_Free(void *node);
void * KlcKoNameData_Find(char *name);
KUTL_KNODE_S * KlcKoNameData_FindKNode(char *name);
KUTL_KNODE_S * KlcKoNameData_GetKNodeByNode(void *node);

#ifdef __cplusplus
}
#endif
#endif //KLCKO_NAME_DATA_H_
