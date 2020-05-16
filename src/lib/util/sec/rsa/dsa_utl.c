/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/dsa_utl.h"

DSA * DSA_BuildKey(int bits)
{
    DSA *ret;

    if ((ret = DSA_new()) == NULL)
        return NULL;

    if (! DSA_generate_parameters_ex(ret, bits, NULL, 0, NULL, NULL, NULL)) {
        DSA_free(ret);
        return NULL;
    }

    return ret;
}
