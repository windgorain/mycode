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
MYBPF_RUNTIME_S * ULCAPP_GetRuntime(void);
int ULCAPP_LoadFile(char *filename, char *instance);
int ULCAPP_ReplaceFile(char *instance, char *filename, UINT keep_map);
int ULCAPP_UnloadInstance(char *instance);
int ULCAPP_RuntimeSave(HANDLE hFile);
void ULCAPP_ShowMap(void);
void ULCAPP_DumpMap(int map_fd);
void ULCAPP_ShowProg(void);
void ULCAPP_Tcmd(int argc, char **argv);

#ifdef __cplusplus
}
#endif
#endif //ULCAPP_RUNTIME_H_
