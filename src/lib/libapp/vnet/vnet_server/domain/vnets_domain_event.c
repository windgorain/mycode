
#include "bs.h"

#include "utl/hash_utl.h"

#include "../inc/vnets_domain.h"
#include "../inc/vnets_wan_plug.h"
#include "../inc/vnets_master.h"
#include "../inc/vnets_mac_tbl.h"

#include "vnets_domain_inner.h"

static PF_VNET_DOMAIN_EVENT_FUNC g_apfVnetDomainEventFuncTbl[] =
{
    VNETS_MACTBL_DomainEventInput,
    VNETS_WAN_PLUG_DomainEvent,
};

BS_STATUS _VNETS_DomainEvent_Issu(IN VNETS_DOMAIN_RECORD_S *pstParam, IN UINT uiEvent)
{
    UINT i;
    BS_STATUS eRet = BS_OK;

    for (i=0; i<sizeof(g_apfVnetDomainEventFuncTbl)/sizeof(PF_VNET_DOMAIN_EVENT_FUNC); i++)
    {
        eRet = g_apfVnetDomainEventFuncTbl[i](pstParam, uiEvent);
        if (BS_OK != eRet)
        {
            break;
        }
    }

    return eRet;
}


