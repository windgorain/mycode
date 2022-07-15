/*================================================================
*   Created by LiXingang: 2018.11.29
*   Description: 
*
================================================================*/
#ifndef _FAKECERT_LOG_H
#define _FAKECERT_LOG_H
#ifdef __cplusplus
extern "C"
{
#endif

#define FAKECERT_LOG_FLAG_PROCESS 0x1
#define FAKECERT_LOG_FLAG_EVENT   0x2
#define FAKECERT_LOG_FLAG_ERROR   0x4

extern UINT g_fakecert_log_flag;

#define FAKECERT_LOG_PROCESS(_X) do { \
        if (g_fakecert_log_flag & FAKECERT_LOG_FLAG_PROCESS) {fakecert_log _X;} \
    }while (0)

#define FAKECERT_LOG_EVENT(_X) do { \
        if (g_fakecert_log_flag & FAKECERT_LOG_FLAG_EVENT) {fakecert_log _X;} \
    }while (0)

#define FAKECERT_LOG_ERROR(_X) do { \
        if (g_fakecert_log_flag & FAKECERT_LOG_FLAG_ERROR) {fakecert_log _X;} \
    }while (0)

int fakecert_log_conf_init();
void fakecert_log(IN CHAR *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif //FAKECERT_LOG_H_
