/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-6
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#ifdef IN_WINDOWS

#include "utl/vnic_lib.h"
#include "utl/txt_utl.h"
#include "utl/vnic_tap.h"

#define TAP_WIN_COMPONENT_ID "tap0901"

typedef struct tap_reg
{
  char *guid;
  HKEY unit_key;
  struct tap_reg *next;
}VNETC_VNIC_PHY_REGTAP_S;

static VOID free_tap_reg(IN VNETC_VNIC_PHY_REGTAP_S *pstRegList)
{
    VNETC_VNIC_PHY_REGTAP_S *pstNext;

    if (NULL == pstRegList)
    {
        return;
    }

    while(pstRegList != NULL)
    {
        pstNext = pstRegList->next;
        RegCloseKey (pstRegList->unit_key);
        MEM_Free(pstRegList->guid);
        MEM_Free(pstRegList);
        pstRegList = pstNext;
    }
}

static VNETC_VNIC_PHY_REGTAP_S * _vnic_dev_FindReg(VNETC_VNIC_PHY_REGTAP_S *first, char *guid)
{
    VNETC_VNIC_PHY_REGTAP_S *pstTmp;

    if (NULL == first) {
        return NULL;
    }

    pstTmp = first;

    while(pstTmp != NULL) {
        if (strcmp(guid, pstTmp->guid) == 0) {
            return pstTmp;
        }
        pstTmp = pstTmp->next;
    }

    return NULL;
}

VNETC_VNIC_PHY_REGTAP_S * get_tap_reg ()
{
    HKEY adapter_key;
    LONG status;
    DWORD len;
    struct tap_reg *first = NULL;
    struct tap_reg *last = NULL;
    int i = 0;

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ADAPTER_KEY, 0, KEY_READ, &adapter_key);
    if (status != 0)
    {
        BS_WARNNING(("Error opening registry key: %s", ADAPTER_KEY));
        return NULL;
    }

    while (1)
    {
        char enum_name[256];
        char unit_string[256];
        HKEY unit_key;
        char component_id_string[] = "ComponentId";
        char component_id[256];
        char net_cfg_instance_id_string[] = "NetCfgInstanceId";
        char net_cfg_instance_id[256];
        DWORD data_type;
        struct tap_reg *reg;

        len = sizeof (enum_name);

        status = RegEnumKeyEx(adapter_key, i, enum_name, &len, NULL, NULL, NULL, NULL);
        if (status == ERROR_NO_MORE_ITEMS)
        {
            break;
        }
        else if (status != 0)
        {
            BS_WARNNING(("Error enumerating registry subkeys of key: %s", ADAPTER_KEY));
        }

        snprintf(unit_string, sizeof(unit_string), "%s\\%s", ADAPTER_KEY, enum_name);

        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, unit_string, 0, KEY_ALL_ACCESS, &unit_key);
        if (status == ERROR_SUCCESS) {
            reg = NULL;
            len = sizeof (component_id);
            status = RegQueryValueEx(unit_key, component_id_string, NULL, &data_type, component_id, &len);
            if ((status == 0) && (data_type == REG_SZ)) {
                len = sizeof (net_cfg_instance_id);
                status = RegQueryValueEx(unit_key, net_cfg_instance_id_string, NULL, &data_type, net_cfg_instance_id, &len);
                if (status == ERROR_SUCCESS && data_type == REG_SZ) {
                    if (strcmp(component_id, TAP_WIN_COMPONENT_ID) == 0) {
                        reg = MEM_ZMalloc(sizeof(struct tap_reg));
                        reg->guid = TXT_Strdup(net_cfg_instance_id);
                        reg->unit_key = unit_key;

                        
                        if (!first) {
                            first = reg;
                        }
                        if (last) {
                            last->next = reg;
                        }
                        last = reg;
                    }
                }
            }
            
            if (reg == NULL) {
                RegCloseKey (unit_key);
            }
        }
        ++i;
    }

    RegCloseKey (adapter_key);
    return first;
}

static CHAR * get_unspecified_device_guid(IN int device_number, IN struct tap_reg *tap_reg_src)
{
  const struct tap_reg *tap_reg = tap_reg_src;
  int i;

    
    if (!tap_reg)
    {
        return NULL;
    }

    
    for (i = 0; i < device_number; i++)
    {
        tap_reg = tap_reg->next;
        if (!tap_reg)
        {
            return NULL;
        }
    }

    return tap_reg->guid;
}

static HANDLE _vnic_dev_OpenTap(OUT CHAR *pcGuidOut, IN UINT uiSize)
{
    VNETC_VNIC_PHY_REGTAP_S *pstRegTaps;
    CHAR *pcGuid;
    INT iDevNum = 0;
    CHAR szAdapterName[128] = "";
    CHAR device_path[256];
    VNIC_HANDLE hVnic = NULL;

    pstRegTaps = get_tap_reg();
    if (NULL == pstRegTaps)
    {
        return NULL;
    }

    while (1)
    {
        pcGuid = get_unspecified_device_guid(iDevNum, pstRegTaps);
        if (NULL == pcGuid)
        {
            break;
        }

        snprintf(device_path, sizeof(device_path), "\\\\.\\Global\\%s.tap", pcGuid);

        hVnic = VNIC_Create(device_path);
        if (NULL != hVnic)
        {
            break;
        }

        iDevNum ++;
    }

    if (NULL != hVnic)
    {
        TXT_Strlcpy(pcGuidOut, pcGuid, uiSize);
    }

    free_tap_reg(pstRegTaps);

    return hVnic;
}

static HANDLE _vnic_dev_TryOpen(OUT CHAR *pcGuidOut, IN UINT uiSize)
{
    HANDLE hVnicAgent;

    hVnicAgent = _vnic_dev_OpenTap(pcGuidOut, uiSize);
    if (hVnicAgent != NULL)
    {
        return hVnicAgent;
    }

    return NULL;
}


static VOID _vnic_dev_InstallTap()
{
    CHAR *pcExePath;
    CHAR szCmd[1024];

    pcExePath = SYSINFO_GetExePath();

    snprintf(szCmd, sizeof(szCmd), "\"%s\\vndis\\addtap.bat\"", pcExePath);

    PROCESS_CreateByFile(szCmd, "", PROCESS_FLAG_WAIT_FOR_OVER | PROCESS_FLAG_HIDE);
}

static HANDLE _vnic_dev_Open(OUT CHAR *pcGuidOut, IN UINT uiSize)
{
    HANDLE hVnicAgent;

    
    hVnicAgent = _vnic_dev_TryOpen(pcGuidOut, uiSize);
    if (NULL != hVnicAgent)
    {
        return hVnicAgent;
    }

    
    _vnic_dev_InstallTap();

    
    hVnicAgent = _vnic_dev_TryOpen(pcGuidOut, uiSize);
    if (NULL != hVnicAgent)
    {
        return hVnicAgent;
    }

    return NULL;
}

VNIC_HANDLE VNIC_Dev_Open()
{
    CHAR szGuid[128];
    VNIC_HANDLE hVnic;

    hVnic = _vnic_dev_Open(szGuid, sizeof(szGuid));
    if (hVnic == 0)
    {
        return NULL;
    }

    VNIC_SetAdapterGuid(hVnic, szGuid);

    return hVnic;
}

VOID VNIC_Dev_SetTun(VNIC_HANDLE hVnic, UINT ip, UINT mask)
{
	unsigned int ep[3] = {0};

    ep[0] = ip;
    ep[2] = mask;
    ep[1] = ep[0] & ep[2];

    VNIC_Ioctl(hVnic, TAP_WIN_IOCTL_CONFIG_TUN, ep, sizeof(ep), ep, sizeof(ep), &len);
}

BS_STATUS VNIC_Dev_RegMediaStatus(VNIC_HANDLE hVnic, BOOL_T bUp)
{
    char *guid;
    VNETC_VNIC_PHY_REGTAP_S *list;
    VNETC_VNIC_PHY_REGTAP_S *find;

    guid = VNIC_GetAdapterGuid(hVnic);
    if (NULL == guid) {
        RETURN(BS_ERR);
    }

    list = get_tap_reg();
    if (NULL == list) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    find = _vnic_dev_FindReg(list, guid);
    if (find == NULL) {
        free_tap_reg(list);
        RETURN(BS_NO_SUCH);
    }

    if (bUp) {
        RegSetValueEx(find->unit_key, "MediaStatus", 0, REG_SZ, "1", 1);
    } else {
        RegSetValueEx(find->unit_key, "MediaStatus", 0, REG_SZ, "0", 1);
    }

    free_tap_reg(list);

    return BS_OK;
}

BS_STATUS VNIC_Dev_RegAllowNonAdmin(VNIC_HANDLE hVnic, BOOL_T bAllow)
{
    char *guid;
    VNETC_VNIC_PHY_REGTAP_S *list;
    VNETC_VNIC_PHY_REGTAP_S *find;

    guid = VNIC_GetAdapterGuid(hVnic);
    if (NULL == guid) {
        RETURN(BS_ERR);
    }

    list = get_tap_reg();
    if (NULL == list) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    find = _vnic_dev_FindReg(list, guid);
    if (find == NULL) {
        free_tap_reg(list);
        RETURN(BS_NO_SUCH);
    }

    if (bAllow) {
        RegSetValueEx(find->unit_key, "AllowNonAdmin", 0, REG_SZ, "1", 1);
    } else {
        RegSetValueEx(find->unit_key, "AllowNonAdmin", 0, REG_SZ, "0", 1);
    }

    free_tap_reg(list);

    return BS_OK;
}
#endif
