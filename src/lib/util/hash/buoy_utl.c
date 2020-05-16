#include "bs.h"
#include "utl/buoy_utl.h"
#include "utl/bitmap_utl.h"

static void buoy_set_point(int buoy_size, void *buoy, unsigned int point)
{
    if (buoy_size == 1) {
        *((unsigned char *)buoy) = point;
    } 

    if (buoy_size == 2) {
        *((unsigned short *)buoy) = point;
    }

    *((unsigned int*)buoy) = point;
}

int BUOY_Init(BUOY_CTRL_S *ctrl, int buoys_num, int buoy_size, void *buoys)
{
    if ((buoy_size != 1) && (buoy_size != 2) && (buoy_size != 4)) {
        return -1;
    }

    ctrl->buoys = buoys;
    ctrl->buoys_num = buoys_num;
    ctrl->buoy_size = buoy_size;

    return 0;
}

unsigned int BUOY_Anchor1(BUOY_CTRL_S *ctrl, int buoy_index, PF_BUOY_GET_BEST_ANCHOR_POINT func, void *user_data)
{
    unsigned char * buoy;

    assert(ctrl->buoy_size == 1);

    buoy = (unsigned char*)ctrl->buoys + buoy_index;
    if (*buoy == 0) {
        *buoy = func(user_data);
    }

    BITMAP_SET(&ctrl->bitmap, buoy_index);

    return *buoy;
}

unsigned int BUOY_Anchor2(BUOY_CTRL_S *ctrl, int buoy_index, PF_BUOY_GET_BEST_ANCHOR_POINT func, void *user_data)
{
    unsigned short * buoy;

    assert(ctrl->buoy_size == 2);

    buoy = (unsigned short*)ctrl->buoys + buoy_index;
    if (*buoy == 0) {
        *buoy = func(user_data);
    }

    BITMAP_SET(&ctrl->bitmap, buoy_index);

    return *buoy;
}

unsigned int BUOY_Anchor4(BUOY_CTRL_S *ctrl, int buoy_index, PF_BUOY_GET_BEST_ANCHOR_POINT func, void *user_data)
{
    unsigned int * buoy;

    assert(ctrl->buoy_size == 4);

    buoy = (unsigned int*)ctrl->buoys + buoy_index;
    if (*buoy == 0) {
        *buoy = func(user_data);
    }

    BITMAP_SET(&ctrl->bitmap, buoy_index);

    return *buoy;
}

void BUOY_Retrieve(BUOY_CTRL_S *ctrl)
{
    unsigned int index;
    void *buoy;

    BITMAP_SCAN_UNSETTED_BEGIN(&ctrl->bitmap, index)
    {
        buoy = (char*)ctrl->buoys + index * ctrl->buoy_size;
        buoy_set_point(ctrl->buoy_size, buoy, 0);
    }BITMAP_SCAN_END();

    BITMAP_Zero(&ctrl->bitmap);
}

#ifdef TEST
#define BUOYS_TEST_NUM (1024*1024)
typedef struct {
    BUOY_CTRL_S buoy_ctrl;
    unsigned char buoys[BUOYS_TEST_NUM];
    unsigned int  bitmap[(BUOYS_TEST_NUM +31)/32];
}BUOY_TEST_S;

int test()
{
    BUOY_TEST_S test_ctrl;

    BUOY_Init(&test_ctrl.buoy_ctrl, BUOYS_TEST_NUM, 1, test_ctrl.buoys);
    BITMAP_Init(&test_ctrl.buoy_ctrl.bitmap, BUOYS_TEST_NUM, test_ctrl.bitmap);

    return 0;
}
#endif
