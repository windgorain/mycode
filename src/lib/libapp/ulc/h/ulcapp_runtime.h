/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ULCAPP_RUNTIME_H
#define _ULCAPP_RUNTIME_H
#ifdef __cplusplus
extern "C"
{
#endif

int ULCAPP_RuntimeInit(void);
int ULCAPP_LoadFile(char *filename, char *instance);
int ULCAPP_ReplaceFile(char *instance, char *filename, UINT keep_map);
int ULCAPP_UnloadInstance(char *instance);
int ULCAPP_RuntimeSave(HANDLE hFile);

#ifdef __cplusplus
}
#endif
#endif //ULCAPP_RUNTIME_H_
