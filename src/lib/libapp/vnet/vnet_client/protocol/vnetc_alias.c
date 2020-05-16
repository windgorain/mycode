
#include "bs.h"

#include "utl/txt_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_p_nodeinfo.h"

static CHAR g_szVnetcAlias[VNET_CONF_MAX_ALIAS_LEN + 1] = "";

BS_STATUS VNETC_Alias_SetAlias(IN CHAR *pcAlias)
{
    if (NULL == pcAlias)
    {
        return BS_NULL_PARA;
    }

    TXT_Strlcpy(g_szVnetcAlias, pcAlias, sizeof(g_szVnetcAlias));

    VNETC_P_NodeInfo_SendInfo();

    return BS_OK;
}

CHAR * VNETC_Alias_GetAlias()
{
    return g_szVnetcAlias;
}

