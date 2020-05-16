/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-8-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sif_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/mime_utl.h"
#include "utl/json_utl.h"
#include "comp/comp_if.h"
#include "comp/comp_kfapp.h"

typedef struct
{
    DLL_NODE_S stLinkNode;
    PF_IFNET_SAVE_FUNC pfSaveFunc;
}_IFNET_SAVE_NODE_S;

static IF_INDEX _ifnet_GetIfIndex(IN CHAR *pcIfName);
static IF_INDEX _ifnet_CreateIfByName(IN CHAR *pcIfType, IN CHAR *pcIfName);

static DLL_HEAD_S g_stIfnetSaveList = DLL_HEAD_INIT_VALUE(&g_stIfnetSaveList);

static IF_INDEX _ifnet_GetIfIndexByCmdEnv(IN VOID *pEnv)
{
    CHAR *pcIfName;
    IF_INDEX ifIndex;

    pcIfName = CMD_EXP_GetCurrentModeName(pEnv);
    if (NULL == pcIfName)
    {
        return 0;
    }

    ifIndex = _ifnet_GetIfIndex(pcIfName);
    if (IF_INVALID_INDEX == ifIndex)
    {
        return 0;
    }

    return ifIndex;
}


/* cmd */
/* 命令行格式: interface (interface-type) (index) */
/* 此接口会根据接口类型和Index创建接口或者进入已存在接口*/
/* Index可以是数字,也可以是字符串*/
/* 接口名就是接口类型加Index, 比如wan-pcap-0, 同时接口名也会作为模式名 */
/* 此接口类型名称必须作为视图名 */
static BS_STATUS _ifnet_cmd_EnterInterface
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    CHAR *pcIfType;
    CHAR *pcIndex;
    CHAR szInterfaceName[IF_MAX_NAME_LEN + 1];
    IF_INDEX ifIndex;
    UINT uiFlag;

    if (ulArgc < 3)
    {
        return BS_BAD_PARA;
    }

    pcIfType = argv[1];
    pcIndex = argv[2];

    if (strlen(pcIfType) + strlen(pcIndex) > sizeof(szInterfaceName) - 1)
    {
        EXEC_OutString("Interface name is too long.\r\n");
        return BS_OUT_OF_RANGE;
    }

    if (strchr(pcIndex, '-') != NULL)
    {
        EXEC_OutString("Index can't inclue \'-\'.\r\n");
        return BS_BAD_REQUEST;
    }

    uiFlag = SIF_GetIfTypeFlag(pcIfType);

    snprintf(szInterfaceName, sizeof(szInterfaceName), "%s-%s", pcIfType, pcIndex);

    ifIndex = _ifnet_GetIfIndex(szInterfaceName);
    if ((ifIndex == 0) && ((uiFlag & IF_TYPE_FLAG_STATIC) == 0))
    {
        if ((uiFlag & IF_TYPE_FLAG_STATIC)
            || ((uiFlag & IF_TYPE_FLAG_RUNNING_STATIC)
                && (PLUGCT_GetLoadStage() >= PLUG_STAGE_RUNNING)))
        {
            EXEC_OutString("Interface not exist.\r\n");
            return BS_ERR;
        }

        ifIndex = _ifnet_CreateIfByName(pcIfType, szInterfaceName);
    }

    if (ifIndex == 0)
    {
        EXEC_OutString("Interface not exist.\r\n");
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS _ifnet_blackhole_LinkOutput (IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 主机序 */)
{
    MBUF_Free(pstMbuf);
    return BS_OK;
}

static BS_STATUS _ifnet_RegEvent(IN PF_IF_EVENT_FUNC pfEventFunc, IN USER_HANDLE_S *pstUserHandle)
{
    return SIF_RegEvent(pfEventFunc, pstUserHandle);
}

static BS_STATUS _ifnet_SetProtoType(IN CHAR *pcProtoType, IN IF_PROTO_PARAM_S *pstParam)
{
    return SIF_SetProtoType(pcProtoType, pstParam);
}

static IF_PROTO_PARAM_S * _ifnet_GetProtoType(IN CHAR *pcProtoType)
{
    return SIF_GetProtoType(pcProtoType);
}

static BS_STATUS _ifnet_SetLinkType(IN CHAR *pcLinkType, IN IF_LINK_PARAM_S *pstParam)
{
    return SIF_SetLinkType(pcLinkType, pstParam);
}

static IF_LINK_PARAM_S * _ifnet_GetLinkType(IN CHAR *pcLinkType)
{
    return SIF_GetLinkType(pcLinkType);
}

static BS_STATUS _ifnet_SetPhyType(IN CHAR *pcPhyType, IN IF_PHY_PARAM_S *pstParam)
{
    return SIF_SetPhyType(pcPhyType, pstParam);
}

static IF_PHY_PARAM_S * _ifnet_GetPhyType(IN CHAR *pcPhyType)
{
    return SIF_GetPhyType(pcPhyType);
}

static VOID _ifnet_NotifySave(IF_INDEX ifIndex, IN HANDLE hFile)
{
    _IFNET_SAVE_NODE_S *pstNode;

    DLL_SCAN(&g_stIfnetSaveList, pstNode)
    {
        pstNode->pfSaveFunc(ifIndex, hFile);
    }
}

static VOID _ifnet_RegCmd(IN CHAR *pcIfType)
{
    CHAR szCmd[512];
    CMD_EXP_REG_CMD_PARAM_S stCmdParam = {0};

    snprintf(szCmd, sizeof(szCmd), "interface %s %%STRING<1-%d>",
            pcIfType, IF_MAX_NAME_INDEX_LEN);

    stCmdParam.uiType = DEF_CMD_EXP_TYPE_VIEW;
    stCmdParam.uiProperty = DEF_CMD_EXP_PROPERTY_TEMPLET;
    stCmdParam.pcViews = "ifnet-view";
    stCmdParam.pcCmd = szCmd;
    stCmdParam.pcViewName = pcIfType;
    stCmdParam.pfFunc = _ifnet_cmd_EnterInterface;

    CMD_EXP_RegCmd(&stCmdParam);
}

static UINT _ifnet_AddIfType(IN CHAR *pcIfType, IN IF_TYPE_PARAM_S *pstParam)
{
    UINT uiType;

    uiType = SIF_AddIfType(pcIfType, pstParam);
    if (uiType == 0)
    {
        return 0;
    }

    if ((pstParam->uiFlag & IF_TYPE_FLAG_HIDE) == 0)
    {
        _ifnet_RegCmd(pcIfType);
    }

    return uiType;
}

static CHAR * _ifnet_GetTypeNameByType(IN UINT uiType)
{
    return SIF_GetTypeNameByType(uiType);
}

static IF_INDEX _ifnet_CreateIf(IN CHAR *pcIfType)
{
    return SIF_CreateIf(pcIfType);
}

static IF_INDEX _ifnet_CreateIfByName(IN CHAR *pcIfType, IN CHAR *pcIfName)
{
    return SIF_CreateIfByName(pcIfType, pcIfName);
}

static VOID _ifnet_DeleteIf(IN IF_INDEX ifIndex)
{
    SIF_DeleteIf(ifIndex);
}

static CHAR * _ifnet_GetIfName(IN IF_INDEX ifIndex, OUT CHAR szIfName[IF_MAX_NAME_LEN + 1])
{
    return SIF_GetIfName(ifIndex, szIfName);
}

static BS_STATUS _ifnet_Ioctl(IN IF_INDEX ifIndex, IN IF_IOCTL_CMD_E enCmd, IN HANDLE hData)
{
    return SIF_Ioctl(ifIndex, enCmd, hData);
}

static BS_STATUS _ifnet_PhyIoctl(IN IF_INDEX ifIndex, IN IF_PHY_IOCTL_CMD_E enCmd, IN HANDLE hData)
{
    return SIF_PhyIoctl(ifIndex, enCmd, hData);
}

static IF_INDEX _ifnet_GetIfIndex(IN CHAR *pcIfName)
{
    return SIF_GetIfIndex(pcIfName);
}

static BS_STATUS _ifnet_PhyOutput(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf)
{
    return SIF_PhyOutput(ifIndex, pstMbuf);
}

static BS_STATUS _ifnet_LinkInput(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf)
{
    return SIF_LinkInput(ifIndex, pstMbuf);
}

static BS_STATUS _ifnet_LinkOutput (IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 网络序 */)
{
    return SIF_LinkOutput(ifIndex, pstMbuf, usProtoType);
}

static BS_STATUS _ifnet_ProtoInput(IN IF_INDEX ifIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType/* 网络序 */)
{
    return SIF_ProtoInput(ifIndex, pstMbuf, usProtoType);
}

static UINT _ifnet_AllocUserDataIndex()
{
    return SIF_AllocUserDataIndex();
}

static BS_STATUS _ifnet_SetUserData(IN IF_INDEX ifIndex, IN UINT uiIndex, IN HANDLE hData)
{
    return SIF_SetUserData(ifIndex, uiIndex, hData);
}

static BS_STATUS _ifnet_GetUserData(IN IF_INDEX ifIndex, IN UINT uiIndex, IN HANDLE *phData)
{
    return SIF_GetUserData(ifIndex, uiIndex, phData);
}

static IF_INDEX _ifnet_GetNext(IN IF_INDEX ifIndexCurrent/* 0表示从头开始 */)
{
    return SIF_GetNext(ifIndexCurrent);
}

static BS_STATUS _ifnet_RegSave(IN PF_IFNET_SAVE_FUNC pfSaveFunc)
{
    _IFNET_SAVE_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(_IFNET_SAVE_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->pfSaveFunc = pfSaveFunc;

    SPLX_P();
    DLL_ADD(&g_stIfnetSaveList, pstNode);
    SPLX_V();

    return BS_OK;
}

static COMP_IF_S g_stIfComp;

static VOID _ifnet_CompInit()
{
    g_stIfComp.pfRegSave = _ifnet_RegSave;
    g_stIfComp.pfRegEvent = _ifnet_RegEvent;
    g_stIfComp.pfSetProtoType = _ifnet_SetProtoType;
    g_stIfComp.pfGetProtoType = _ifnet_GetProtoType;
    g_stIfComp.pfSetLinkType = _ifnet_SetLinkType;
    g_stIfComp.pfGetLinkType = _ifnet_GetLinkType;
    g_stIfComp.pfSetPhyType = _ifnet_SetPhyType;
    g_stIfComp.pfGetPhyType = _ifnet_GetPhyType;
    g_stIfComp.pfAddIfType = _ifnet_AddIfType;
    g_stIfComp.pfGetTypeNameByType = _ifnet_GetTypeNameByType;
    g_stIfComp.pfCreateIf = _ifnet_CreateIf;
    g_stIfComp.pfCreateIfByName = _ifnet_CreateIfByName;
    g_stIfComp.pfDeleteIf = _ifnet_DeleteIf;
    g_stIfComp.pfGetIfName = _ifnet_GetIfName;
    g_stIfComp.pfIoctl = _ifnet_Ioctl;
    g_stIfComp.pfPhyIoctl = _ifnet_PhyIoctl;
    g_stIfComp.pfGetIfIndex = _ifnet_GetIfIndex;
    g_stIfComp.pfPhyOutput = _ifnet_PhyOutput;
    g_stIfComp.pfLinkInput = _ifnet_LinkInput;
    g_stIfComp.pfLinkOutput = _ifnet_LinkOutput;
    g_stIfComp.pfProtoInput = _ifnet_ProtoInput;
    g_stIfComp.pfAllocUserDataIndex = _ifnet_AllocUserDataIndex;
    g_stIfComp.pfSetUserData = _ifnet_SetUserData;
    g_stIfComp.pfGetUserData = _ifnet_GetUserData;
    g_stIfComp.pfGetNext = _ifnet_GetNext;
    g_stIfComp.comp.comp_name = COMP_IF_NAME;

    COMP_Reg(&g_stIfComp.comp);
}

static BS_STATUS _ifnet_kf_Modify(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcName;
    CHAR *pcPropertyValue;
    IF_INDEX ifIndex;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        JSON_SetFailed(pstParam->pstJson, "Empty name");
        return BS_ERR;
    }

    ifIndex = _ifnet_GetIfIndex(pcName);
    if (IF_INVALID_INDEX == ifIndex)
    {
        JSON_SetFailed(pstParam->pstJson, "Not exist");
        return BS_NOT_FOUND;
    }

    pcPropertyValue = MIME_GetKeyValue(hMime, "Shutdown");
    if (NULL == pcPropertyValue)
    {
        pcPropertyValue = "0";
    }

    if (pcPropertyValue[0] == '0')
    {
        _ifnet_Ioctl(ifIndex, IFNET_CMD_SET_SHUTDOWN, FALSE);
    }
    else
    {
        _ifnet_Ioctl(ifIndex, IFNET_CMD_SET_SHUTDOWN, UINT_HANDLE(TRUE));
    }
    
    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _ifnet_kf_Get(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcName;
    IF_INDEX ifIndex;
    BOOL_T bShutdown;
    CHAR szInfo[512];
    LSTR_S stLstr;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        JSON_SetFailed(pstParam->pstJson, "Empty name");
        return BS_OK;
    }

    ifIndex = _ifnet_GetIfIndex(pcName);
    if (IF_INVALID_INDEX == ifIndex)
    {
        JSON_SetFailed(pstParam->pstJson, "Not exist");
        return BS_OK;
    }

    cJSON_AddStringToObject(pstParam->pstJson, "Name", pcName);

    _ifnet_Ioctl(ifIndex, IFNET_CMD_GET_SHUTDOWN, &bShutdown);
    if (bShutdown)
    {
        cJSON_AddStringToObject(pstParam->pstJson, "Shutdown", "1");
    }
    else
    {
        cJSON_AddStringToObject(pstParam->pstJson, "Shutdown", "0");
    }

    stLstr.pcData = szInfo;
    stLstr.uiLen = sizeof(szInfo);
    if (BS_OK != _ifnet_PhyIoctl(ifIndex, IF_PHY_IOCTL_CMD_GET_PRIVATE_INFO, &stLstr))
    {
        szInfo[0] = '\0';
    }

    cJSON_AddStringToObject(pstParam->pstJson, "Private_Info", szInfo);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _ifnet_kf_List(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    cJSON *pstArray;
    cJSON *pstResJson;
    IF_INDEX ifIndex = 0;
    CHAR szIfName[IF_MAX_NAME_LEN + 1];
    CHAR szType[IF_MAX_NAME_LEN + 1];
    UINT uiFlag;

    pstArray = cJSON_CreateArray();
    if (NULL == pstArray)
    {
        JSON_SetFailed(pstParam->pstJson, "Not enough memory");
        return BS_ERR;
    }

    while ((ifIndex = _ifnet_GetNext(ifIndex)) != 0)
    {
        _ifnet_GetIfName(ifIndex, szIfName);

        if (BS_OK != IF_GetTypeNameByIfName(szIfName, szType))
        {
            continue;
        }

        uiFlag = SIF_GetIfTypeFlag(szType);
        if (uiFlag & IF_TYPE_FLAG_HIDE)
        {
            continue;
        }

        pstResJson = cJSON_CreateObject();
        if (NULL == pstResJson)
        {
            continue;
        }

        cJSON_AddStringToObject(pstResJson, "Name", szIfName);

        cJSON_AddItemToArray(pstArray, pstResJson);
    }

    cJSON_AddItemToObject(pstParam->pstJson, "data", pstArray);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _ifnet_KfInit()
{
//    COMP_KFAPP_RegFunc("Interface.IsExist", _localuser_kf_IsExist, NULL);
//    COMP_KFAPP_RegFunc("Interface.Add", _localuser_kf_Add, NULL);
    COMP_KFAPP_RegFunc("Interface.Modify", _ifnet_kf_Modify, NULL);
    COMP_KFAPP_RegFunc("Interface.Get", _ifnet_kf_Get, NULL);
//    COMP_KFAPP_RegFunc("Interface.Delete", _localuser_kf_Del, NULL);
    COMP_KFAPP_RegFunc("Interface.List", _ifnet_kf_List, NULL);

    return BS_OK;
}

static BS_STATUS _ifnet_Init()
{
    IF_LINK_PARAM_S stLinkParam = {0};
    IF_PROTO_PARAM_S stProtoParam = {0};
    IF_TYPE_PARAM_S stTypeParam = {0};

    _ifnet_SetLinkType(IF_ETH_LINK_TYPE_MAME, &stLinkParam);
    _ifnet_SetProtoType(IF_PROTO_IP_TYPE_MAME, &stProtoParam);

    stLinkParam.pfLinkOutput = _ifnet_blackhole_LinkOutput;
    SIF_SetLinkType(COMP_IF_BLACK_HOLE_TYPE, &stLinkParam);

    stTypeParam.pcLinkType = COMP_IF_BLACK_HOLE_TYPE;
    stTypeParam.uiFlag = IF_TYPE_FLAG_HIDE;
    SIF_AddIfType(COMP_IF_BLACK_HOLE_TYPE, &stTypeParam);

    return BS_OK;
}

BS_STATUS IFNET_Init()
{
    COMP_KFAPP_Init();
    
    if (BS_OK != SIF_Init(SIF_FLAG_LOCK))
    {
        return BS_ERR;
    }

    _ifnet_Init();

    _ifnet_CompInit();

    _ifnet_KfInit();

    return BS_OK;
}

static VOID _ifnet_SaveIf(IF_INDEX ifIndex, IN HANDLE hFile)
{
    CHAR szIfName[IF_MAX_NAME_LEN + 1];
    CHAR szType[IF_MAX_NAME_LEN + 1];
    CHAR *pcIndex;
    UINT uiFlag;
    BOOL_T bShutdown;

    if (NULL == _ifnet_GetIfName(ifIndex, szIfName))
    {
        return;
    }

    pcIndex = IF_GetPhyIndexStringByIfName(szIfName);
    if (NULL == pcIndex)
    {
        return;
    }

    if (BS_OK != IF_GetTypeNameByIfName(szIfName, szType))
    {
        return;
    }

    uiFlag = SIF_GetIfTypeFlag(szType);

    if (uiFlag & IF_TYPE_FLAG_HIDE)
    {
        return;
    }

    CMD_EXP_OutputMode(hFile, "interface %s %s", szType, pcIndex);

    SIF_Ioctl(ifIndex, IFNET_CMD_GET_SHUTDOWN, &bShutdown);
    if (bShutdown)
    {
        CMD_EXP_OutputCmd(hFile, "shutdown");
    }

    /* 通知其他模块进行接口下命令的save */
    _ifnet_NotifySave(ifIndex, hFile);

    CMD_EXP_OutputModeQuit(hFile);
}

PLUG_API BS_STATUS IFNET_Save(IN HANDLE hFile)
{
    IF_INDEX ifIndex = 0;

    while ((ifIndex = _ifnet_GetNext(ifIndex)) != 0)
    {
        _ifnet_SaveIf(ifIndex, hFile);
    }

    return BS_OK;
}

static VOID _ifnet_ShowIf(IF_INDEX ifIndex)
{
    CHAR szIfName[IF_MAX_NAME_LEN + 1];
    CHAR szIfInfo[512];
    LSTR_S stLstr;

    if (NULL == _ifnet_GetIfName(ifIndex, szIfName))
    {
        return;
    }

    stLstr.pcData = szIfInfo;
    stLstr.uiLen = sizeof(szIfInfo);

    if (BS_OK != _ifnet_PhyIoctl(ifIndex, IF_PHY_IOCTL_CMD_GET_PRIVATE_INFO, &stLstr))
    {
        stLstr.pcData = "";
    }

    EXEC_OutInfo("Interface: %s\r\n"
                    "%s\r\n",
                    szIfName, stLstr.pcData);
}

PLUG_API BS_STATUS IFNET_ShowInterface
(
    IN UINT ulArgc,
    IN UCHAR **argv,
    IN VOID *pEnv
)
{
    IF_INDEX ifIndex = 0;

    while ((ifIndex = _ifnet_GetNext(ifIndex)) != 0)
    {
        _ifnet_ShowIf(ifIndex);
    }

    return BS_OK;
}

PLUG_API BS_STATUS IFNET_ShutdownInterface
(
    IN UINT ulArgc,
    IN UCHAR **argv,
    IN VOID *pEnv
)
{
    IF_INDEX ifIndex;
    BOOL_T bShutdown = TRUE;

    ifIndex = _ifnet_GetIfIndexByCmdEnv(pEnv);
    if (IF_INVALID_INDEX == ifIndex)
    {
        return BS_ERR;
    }

    if (argv[0][0] == 'n')
    {
        bShutdown = FALSE;
    }

    return SIF_Ioctl(ifIndex, IFNET_CMD_SET_SHUTDOWN, UINT_HANDLE(bShutdown));
}

