/*================================================================
*   Created by LiXingang
*   Description: AC算法
*
================================================================*/
#include "bs.h"


typedef struct {

    int acsmMaxStates;
    int acsmNumStates;

    ACSM_PATTERN    * acsmPatterns;
    ACSM_STATETABLE * acsmStateTable;

    int   bcSize;
    short bcShift[256];

    int numPatterns;
    void (*userfree)(void *p);
    void (*optiontreefree)(void **p);
    void (*neg_list_free)(void **p);

}ACSM_STRUCT;
