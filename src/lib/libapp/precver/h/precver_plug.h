/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _PRECVER_PLUG_H
#define _PRECVER_PLUG_H
#ifdef __cplusplus
extern "C"
{
#endif

int PRecverPlug_Init();
int PRecverPlug_LoadPlug(char *plug_name);
PLUG_HDL PRecverPlug_GetPlug(char *plug_name);

BOOL_T PRecverPlug_CfgIsExist(char *plug_name);
char * PRecverPlug_CfgGetNext(char *curr/* NULL表示获取第一个 */);

#ifdef __cplusplus
}
#endif
#endif //PRECVER_PLUG_H_
