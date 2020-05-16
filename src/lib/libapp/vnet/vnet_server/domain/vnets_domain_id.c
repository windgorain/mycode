
#include "bs.h"

#include "utl/bitmap1_utl.h"

#include "vnets_domain_inner.h"

#define _VNETS_DOMAIN_ID_NUM 8192

static BITMAP_S g_stVnetsDomainIdBitmap;

BS_STATUS _VNETS_DomainId_Init()
{
    if (BS_OK != BITMAP_Create(&g_stVnetsDomainIdBitmap,_VNETS_DOMAIN_ID_NUM))
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

UINT _VNETS_DomainId_Get()
{
    UINT uiIndex = 0;

    uiIndex = BITMAP1_GetAUnsettedBitIndexCycle(&g_stVnetsDomainIdBitmap);
    if (uiIndex == 0)
    {
        return 0;
    }

    BITMAP_SET(&g_stVnetsDomainIdBitmap, uiIndex);

    return uiIndex;
}

VOID _VNETS_DomainId_FreeID(IN UINT uiId)
{
    if ((uiId < 1) || (uiId > _VNETS_DOMAIN_ID_NUM))
    {
        return;
    }

    BITMAP1_CLR(&g_stVnetsDomainIdBitmap, uiId);
}

