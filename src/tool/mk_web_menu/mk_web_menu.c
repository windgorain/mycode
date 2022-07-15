/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2011-8-1
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/pathtree_utl.h"

static PATHTREE_NODE_S *g_pstMkWebMenuPathTree = NULL;

static BS_WALK_RET_E _mwm_WalkTreeNode
(
    IN PATHTREE_NODE_S *pstNode,
    IN UINT ulDeepth,
    IN BOOL_T bIsBack,
    IN VOID * pUserHandle
)
{
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    FILE *fp = (FILE*)pstUserHandle->ahUserHandle[0];
    UINT ulOldDeepth = HANDLE_UINT(pstUserHandle->ahUserHandle[1]);

    pstUserHandle->ahUserHandle[1] = UINT_HANDLE(ulDeepth);

    if (ulDeepth == 1)
    {
        return BS_WALK_CONTINUE;
    }

    if ((ulDeepth > ulOldDeepth) && (ulOldDeepth > 1))
    {
        FILE_WriteStr(fp, "<ul>");
    }

    if (ulDeepth < ulOldDeepth)
    {
        FILE_WriteStr(fp, "</ul>");
    }

    if (bIsBack == FALSE)
    {
        if (PATHTREE_IS_LEAF(pstNode) && (PATHTREE_USER_HANDLE(pstNode) != NULL))
        {
            FILE_WriteStr(fp, "<li><span class=\"file\"><a href=\"");
            FILE_WriteStr(fp, PATHTREE_USER_HANDLE(pstNode));
            FILE_WriteStr(fp, "\" target=\"contentFrame\"><?php _e(\"");
            FILE_WriteStr(fp, PATHTREE_DIR_NAME(pstNode));
            FILE_WriteStr(fp, "\");?></a></span>");
        }
        else
        {
            FILE_WriteStr(fp, "<li class=\"closed\"><span class=\"folder\"><?php _e(\"");
            FILE_WriteStr(fp, PATHTREE_DIR_NAME(pstNode));
            FILE_WriteStr(fp, "\");?></span>");
        }
    }
    else
    {
        FILE_WriteStr(fp, "</li>");
    }

    return BS_WALK_CONTINUE;
}

static BS_STATUS _mwm_CreateMenuList()
{
    USER_HANDLE_S stUserHandle;
    FILE *fp;

    fp = FILE_Open("menu_list.php", TRUE, "wb+");
    if (NULL == fp)
    {
        return BS_CAN_NOT_OPEN;
    }

    stUserHandle.ahUserHandle[0] = fp;
    stUserHandle.ahUserHandle[1] = UINT_HANDLE(1);

    PathTree_DepthBackWalk(g_pstMkWebMenuPathTree, _mwm_WalkTreeNode, &stUserHandle);

    fclose(fp);

    return BS_OK;
}

BS_STATUS _mwm_RegMenu(IN CHAR *pszWebMenuFilePath)
{
    FILE_MEM_S *pstMemMap;
    CHAR *pszLineHead;
    UINT ulLineLen;
    CHAR *pcSplit;
    CHAR *pszHerf;

    BS_DBGASSERT(g_pstMkWebMenuPathTree != NULL);

    pstMemMap = FILE_Mem(pszWebMenuFilePath);
    if (NULL == pstMemMap)
    {
        return BS_ERR;
    }

    TXT_SCAN_LINE_BEGIN(pstMemMap->pucFileData, pszLineHead, ulLineLen)
    {
        /* 格式为: /dir1/dir2/dir3:xxx.htm */
        pszLineHead[ulLineLen] = '\0';
        pcSplit = strchr(pszLineHead, ':');
        if (NULL == pcSplit)
        {
            continue;
        }
        *pcSplit = '\0';
        pszHerf = pcSplit + 1;
        PathTree_AddPath(g_pstMkWebMenuPathTree, pszLineHead, pszHerf);        
    }TXT_SCAN_LINE_END();

    return BS_OK;
}

void MWM_Run(IN INT iArgc, IN CHAR **ppArgv)
{
    INT i;
    
    g_pstMkWebMenuPathTree = PathTree_Create();
    if (NULL == g_pstMkWebMenuPathTree)
    {
        printf("Can't create path tree.\r\n");
        return;
    }

    for (i=1; i<iArgc; i++)
    {
        _mwm_RegMenu(ppArgv[i]);
    }

    _mwm_CreateMenuList();

    return;
}


