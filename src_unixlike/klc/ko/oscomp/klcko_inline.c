/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_lib.h"

#undef inline
#define inline
#define static

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include <linux/crypto.h>
#include <crypto/skcipher.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

int KlcKoInline_Init(void)
{
    return 0;
}

