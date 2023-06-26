/*================================================================
 *   Created by LiXingang
 *   Description: 
 *
 ================================================================*/
#include "bs.h"
#include "utl/rand_utl.h"
#include "utl/level_topn.h"

#define LT_N (32*1024)
#define LT_X (16*1024*1024)
#define LT_LEVEL (1024 * 1024)

int main (int argc, char **argv)
{
    int i;
    UINT seed;
    UINT rand;
    LEVEL_TOPN_S *topn;

    topn = LevelTopn_Create(LT_N, LT_LEVEL, 0xffffffffULL/LT_LEVEL);
    if (! topn) {
        return -1;
    }

    seed = RAND_Get();

    for (i=0; i<LT_X; i++) {
        rand = RAND_FastGet(&seed);
        LevelTopn_Input(topn, rand, i);
    }

    LevelTopn_Print(topn);

    LevelTopn_Destroy(topn);

    return 0;
}
