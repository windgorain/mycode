
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

#include "../inc/vnets_domain.h"

static int vnets_domaincmd_ShowNode(IN UINT uiNodeID, IN HANDLE hUserHandle)
{
    EXEC_OutInfo(" %-5d\r\n", uiNodeID);
    return 0;
}

static int vnets_domaincmd_ShowDomain(IN VNETS_DOMAIN_RECORD_S *pstParam, IN HANDLE hUserHandle)
{
    EXEC_OutInfo(" %-4d %-4d  %s\r\n",
        pstParam->uiDomainID,
        pstParam->eType,
        pstParam->szDomainName);

    return 0;
}


PLUG_API BS_STATUS VNETS_DomainCmd_ShowDomain(IN UINT ulArgc, IN CHAR ** argv)
{
    EXEC_OutString(" ID   Type  Name\r\n"
                              "-------------------------------------------------------------------------\r\n");

    VNETS_Domain_WalkDomain(vnets_domaincmd_ShowDomain, NULL);

    EXEC_OutString("\r\n");
    return BS_OK;
}


PLUG_API BS_STATUS VNETS_DomainCmd_CreateDomainView(int argc, char **argv, void *pEnv)
{
    UINT uiDomainId;

    uiDomainId = VNETS_Domain_GetDomainID(argv[1]);
    if (uiDomainId == 0)
    {
        EXEC_OutString(" No such domain.\r\n");
        return BS_NO_SUCH;
    }

    return BS_OK;
}

UINT VNETS_DomainCmd_GetDomainIdByEnv(IN VOID *pEnv)
{
    CHAR *pcDomainName;
    UINT uiDomainId;

    pcDomainName = CMD_EXP_GetCurrentModeValue(pEnv);

    uiDomainId = VNETS_Domain_GetDomainID(pcDomainName);

    return uiDomainId;
}


PLUG_API BS_STATUS VNETS_DomainCmd_ShowNode
(
    IN UINT ulArgc,
    IN UCHAR **argv,
    IN VOID *pEnv
)
{
    UINT uiDomainId;

    uiDomainId = VNETS_DomainCmd_GetDomainIdByEnv(pEnv);
    if (uiDomainId == 0)
    {
        EXEC_OutString(" No such domain.\r\n");
        return BS_NO_SUCH;
    }

    EXEC_OutString(" NodeID \r\n"
                              "------------------------------------------------------------\r\n");

    VNETS_Domain_WalkNode(uiDomainId, vnets_domaincmd_ShowNode, NULL);

    EXEC_OutString("\r\n");

    return BS_OK;
}


BS_STATUS _VNETS_DomainCmd_Init()
{
    return BS_OK;
}

