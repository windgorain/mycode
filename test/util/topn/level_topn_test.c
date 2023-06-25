/*================================================================
 *   Created by LiXingang
 *   Description: 
 *
 ================================================================*/
#include "bs.h"
#include "utl/rand_utl.h"
#include "utl/level_topn.h"

int main (int argc, char **argv)
{
    int i;
    UINT seed;
    UINT rand;
    LEVEL_TOPN_S *topn;

    topn = LevelTopn_Create(32*1024, 32);
    if (! topn) {
        return -1;
    }

    for (i=0; i<32; i++) {
        LevelTopn_SetLevel(topn, i, (1UL << i));
    }

    seed = RAND_Get();

    for (i=0; i<16*1024*1024; i++) {
        i ++;
        rand = RAND_FastGet(&seed);
        LevelTopn_Input(topn, rand, i);
    }

    LevelTopn_Print(topn);

    LevelTopn_Destroy(topn);

    return 0;
}
