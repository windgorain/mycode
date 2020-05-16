/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-21
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/exec_utl.h"
#include "utl/ic_utl.h"
#include "utl/txt_utl.h"
#include "utl/buffer_utl.h"
#include "utl/thread_utl.h"
#include "utl/thread_named.h"

#define EXEC_UD_MAX 4

/* structs */
typedef struct
{
    /* 终端方提供的接口 */
    PF_EXEC_OUT_STRING_FUNC pfOutFunc;  /* 输出字符串接口 */
    PF_EXEC_GET_CHAR_FUNC pfGetCharFunc;
    HANDLE uds[EXEC_UD_MAX];

    /* 内部数据 */
    IC_HANDLE     hIcHandle;
    BUFFER_HANDLE hBuffer;
}EXEC_S;

static EXEC_S *g_exec_local = NULL;

static BS_STATUS exec_sendData(EXEC_S *exec, CHAR *pszInfo)
{
    if ((exec) && (exec->pfOutFunc)) {
        exec->pfOutFunc(exec, pszInfo);
    }

	return BS_OK;
}

static VOID exec_BufferWrite(IN void *pcData, IN UINT uiLen,
        IN USER_HANDLE_S *pstUserHandle)
{
    exec_sendData(pstUserHandle->ahUserHandle[0], pcData);
}

static void exec_Output(EXEC_S *exec, char *info, int len)
{
    BUFFER_Write(exec->hBuffer, info, len);
}

static void exec_Flush(EXEC_S *exec)
{
    BUFFER_Flush(exec->hBuffer);
}

static void exec_OutString(char *info, void *user_data)
{
    EXEC_OutString(info);
}

static VOID exec_Tm(IN CHAR *pcMsg, IN USER_HANDLE_S *ud)
{
    EXEC_S *exec = ud->ahUserHandle[0];
    exec_Output(exec, pcMsg, strlen(pcMsg));
    exec_Flush(exec);
}

HANDLE EXEC_Create(PF_EXEC_OUT_STRING_FUNC pfOutStringFunc,
        PF_EXEC_GET_CHAR_FUNC pfGetCharFunc)
{
    EXEC_S *exec = NULL;
    USER_HANDLE_S stUserHandle;

    exec = MEM_ZMalloc(sizeof(EXEC_S));
    if (! exec) {
        return NULL;
    }

    stUserHandle.ahUserHandle[0] = exec;
    exec->hBuffer = BUFFER_Create(511, exec_BufferWrite, &stUserHandle);
    if (NULL == exec->hBuffer) {
        MEM_Free(exec);
        return NULL;
    }

    exec->pfOutFunc = pfOutStringFunc;
    exec->pfGetCharFunc = pfGetCharFunc;

    return exec;
}

BS_STATUS EXEC_Delete(HANDLE hExec)
{
    EXEC_S *exec = hExec;
    
    if (! exec) {
        RETURN(BS_NULL_PARA);
    }

    if (g_exec_local == hExec) {
        g_exec_local = NULL;
    }

    if (hExec == THREAD_GetExec()) {
        THREAD_SetExec(NULL);
    }

    if (exec->hIcHandle) {
        IC_UnReg(exec->hIcHandle);
        exec->hIcHandle = NULL;
    }

    BUFFER_Destory(exec->hBuffer);
    MEM_Free(exec);

    return BS_OK;
}

void EXEC_SetUD(HANDLE hExec, int index, HANDLE ud)
{
    EXEC_S *exec = hExec;

    if (index >= EXEC_UD_MAX) {
        BS_WARNNING(("Out of range"));
        return;
    }
    exec->uds[index] = ud;
}

HANDLE EXEC_GetUD(HANDLE hExec, int index)
{
    EXEC_S *exec = hExec;

    if (index >= EXEC_UD_MAX) {
        BS_WARNNING(("Out of range"));
        return NULL;
    }
    return exec->uds[index];
}

BS_STATUS EXEC_OutString(IN CHAR *pszInfo)
{
    EXEC_S *exec = EXEC_GetExec();
    
    if (! exec) {
        /* 没有EXEC,  则直接输出 */
        exec_sendData(NULL, pszInfo);
        return BS_OK;
    }

    exec_Output(exec, pszInfo, strlen(pszInfo));

    return BS_OK;
}

BS_STATUS EXEC_OutInfo(IN CHAR *pcFmt, ...)
{
    TXT_ARGS_PRINT(exec_OutString, NULL);
    return BS_OK;
}

VOID EXEC_Flush()
{
    EXEC_S *exec = EXEC_GetExec();

    if (exec) {
        exec_Flush(exec);
    }
}

UCHAR EXEC_GetChar()
{
    EXEC_S *exec = EXEC_GetExec();
    
    if ((NULL != exec) && (NULL != exec->pfGetCharFunc)) {
        return exec->pfGetCharFunc(exec);
    }

    return 0;
}

/* terminal monitor */
BS_STATUS EXEC_TM(IN UINT uiArgc, IN CHAR **pcArgv)
{
    EXEC_S *exec;
    USER_HANDLE_S ud;

    exec = EXEC_GetExec();
    if (! exec) {
        RETURN(BS_ERR);
    }

    ud.ahUserHandle[0] = exec;
    exec->hIcHandle = IC_Reg(exec_Tm, &ud);
    if (! exec->hIcHandle) {
        RETURN(BS_NO_MEMORY);
    }

    return BS_OK;
}

/* no terminal monitor */
BS_STATUS EXEC_NoTM(IN UINT uiArgc, IN CHAR **pcArgv)
{
    EXEC_S *exec;

    exec = EXEC_GetExec();
    if (NULL == exec) {
        RETURN(BS_ERR);
    }

    if (exec->hIcHandle != NULL) {
        IC_UnReg(exec->hIcHandle);
        exec->hIcHandle = NULL;
    }

    return BS_OK;
}


