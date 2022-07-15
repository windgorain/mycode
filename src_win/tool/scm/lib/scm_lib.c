/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-11-29
* Description: Service Control Manager	服务控制管理
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/process_utl.h"
#include "utl/mutex_utl.h"

#define _SCM_MAX_ARGC 120
#define _SCM_MAX_PROCESS_NUM 128

#define SCM_CFG_FILE "scm.ini"

typedef struct
{
    DLL_NODE_S stNode;
    CHAR *pcSecName;
    HANDLE hProcess;
}SCM_NODE_S;

static DLL_HEAD_S g_stScmListHead = DLL_HEAD_INIT_VALUE(&g_stScmListHead);
static BOOL_T g_bScmRcvTeam = FALSE;
static MUTEX_S g_stScmMutex;

static HANDLE scm_Execv(IN CHAR *pcSection, IN CHAR *pcPath, IN CHAR *pcParam, IN UINT uiFlag)
{
    CHAR *apcArgv[_SCM_MAX_ARGC + 2];
    UINT uiCount;
    LONG lId;

    apcArgv[0] = pcPath;
    uiCount = TXT_StrToToken(pcParam, " ", apcArgv + 1, _SCM_MAX_ARGC);
    apcArgv[uiCount + 1] = NULL;

    printf("Run %s", pcSection);

    lId = PROCESS_CreateByFile(pcPath, pcParam, uiFlag);
    if (0 == lId)
    {
        printf(" failed\r\n");
        return NULL;
    }

    printf(" success\r\n");

    return PROCESS_GetProcess(lId);
}

static HANDLE scm_StartSection(IN CFF_HANDLE hCff, IN CHAR *pcSection)
{
    CHAR *pcFilePath;
    CHAR *pcParam;
    UINT uiFlag = 0;
    UINT uiHide;

    if (BS_OK != CFF_GetPropAsString(hCff, pcSection, "path", &pcFilePath))
    {
        printf("service %s : Can't get path.\r\n", pcSection);
        return NULL;
    }

    if (BS_OK != CFF_GetPropAsString(hCff, pcSection, "param", &pcParam))
    {
        pcParam = "";
    }

    if (BS_OK != CFF_GetPropAsUint(hCff, pcSection, "hide", &uiHide))
    {
        uiHide = 0;
    }

    if (uiHide)
    {
        uiFlag = PROCESS_FLAG_HIDE;
    }

    return scm_Execv(pcSection, pcFilePath, pcParam, uiFlag);
}

static VOID scm_ProcessSection(IN HANDLE hIni, IN CHAR *pcSection)
{
    SCM_NODE_S *pstNode;
    HANDLE hProcess;
    
    pstNode = MEM_ZMalloc(sizeof(SCM_NODE_S));
    if (NULL == pstNode)
    {
        printf("service %s : No memory.\r\n", pcSection);
        return;
    }

    hProcess = scm_StartSection(hIni, pcSection);
    if (hProcess == NULL)
    {
        MEM_Free(pstNode);
        return;
    }

    pstNode->pcSecName = pcSection;
    pstNode->hProcess = hProcess;

    MUTEX_P(&g_stScmMutex);
    DLL_ADD(&g_stScmListHead, pstNode);    
    MUTEX_V(&g_stScmMutex);

    return;
}

SCM_NODE_S * scm_GetNodeByPID(IN IN HANDLE hProcess)
{
    SCM_NODE_S *pstNode;

    MUTEX_P(&g_stScmMutex);

    DLL_SCAN(&g_stScmListHead, pstNode)
    {
        if (pstNode->hProcess == hProcess)
        {
            break;
        }
    }

    MUTEX_V(&g_stScmMutex);

    return pstNode;
}

static VOID scm_Restart(IN CFF_HANDLE hCff, IN HANDLE hProcess)
{
    SCM_NODE_S *pstNode;
    LONG pidNew;
    UINT uiRestart = 0;
    HANDLE hProcessNew;

    pstNode = scm_GetNodeByPID(hProcess);
    if (NULL == pstNode)
    {
        return;
    }

    printf("Process %s exit.\r\n", pstNode->pcSecName);

    CloseHandle(pstNode->hProcess);
    pstNode->hProcess = NULL;

    if (BS_OK != CFF_GetPropAsUint(hCff, pstNode->pcSecName, "restart", &uiRestart))
    {
        uiRestart = 0;
    }

    if (uiRestart == 0)
    {
        MUTEX_P(&g_stScmMutex);
        DLL_DEL(&g_stScmListHead, pstNode);
        MUTEX_V(&g_stScmMutex);
        return;
    }

    hProcessNew = scm_StartSection(hCff, pstNode->pcSecName);
    if (hProcessNew == NULL)
    {
        MUTEX_P(&g_stScmMutex);
        DLL_DEL(&g_stScmListHead, pstNode);
        MUTEX_V(&g_stScmMutex);
        return;
    }

    pstNode->hProcess = hProcessNew;
}

static VOID scm_Schedule(IN HANDLE hIni)
{
    SCM_NODE_S *pstNode;
    HANDLE ahHandles[_SCM_MAX_PROCESS_NUM];
    UINT i;
    UINT uiIndex;

    printf("Start schedule...\r\n");

    while (1)
    {
        if (g_bScmRcvTeam == TRUE)
        {
            return;
        }

        i = 0;
        MUTEX_P(&g_stScmMutex);
        DLL_SCAN(&g_stScmListHead, pstNode)
        {
            ahHandles[i] = pstNode->hProcess;
            if (i >= _SCM_MAX_PROCESS_NUM)
            {
                break;
            }
            i ++;
        }
        MUTEX_V(&g_stScmMutex);

        uiIndex = WaitForMultipleObjects(i, ahHandles, FALSE, INFINITE);

        if (g_bScmRcvTeam == TRUE)
        {
            return;
        }

        uiIndex -= WAIT_OBJECT_0;

        if (uiIndex >= i)
        {
            continue;
        }

        scm_Restart(hIni, ahHandles[uiIndex]);
    }
}

static VOID scm_KillAll()
{
    SCM_NODE_S *pstNode;

    g_bScmRcvTeam = TRUE;

    MUTEX_P(&g_stScmMutex);

    DLL_SCAN(&g_stScmListHead, pstNode)
    {
        TerminateProcess(pstNode->hProcess, 0);
        CloseHandle(pstNode->hProcess);
        pstNode->hProcess = NULL;
    }

    MUTEX_V(&g_stScmMutex);
}

static INT scm_run(IN UINT argc, IN CHAR **argv)
{
    CFF_HANDLE hCff;
    CHAR *pcSection;

    MUTEX_Init(&g_stScmMutex);

    hCff = CFF_INI_Open("scm.ini", CFF_FLAG_READ_ONLY);
    if (NULL == hCff)
    {
        printf("Can't open %s\r\n", SCM_CFG_FILE);
        return -1;
    }

    CFF_SCAN_TAG_START(hCff, pcSection)
    {
        scm_ProcessSection(hCff, pcSection);
    }CFF_SCAN_END();

    scm_Schedule(hCff);

    scm_KillAll();

    return 0;
}

PLUG_API INT PlugRun(IN UINT argc, IN CHAR **argv)
{
    static BOOL_T bRun = FALSE;

    if (bRun == TRUE)
    {
        return 0;
    }

    bRun = TRUE;
    
    return scm_run(argc, argv);
}

PLUG_API VOID PlugStop()
{
    scm_KillAll();
}

