/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/cuckoo_hash.h"
#include "utl/jhash_utl.h"
#include "utl/file_utl.h"
#include "utl/box_utl.h"

static UINT databox_Index(void *hash, void *data)
{
    LDATA_S *pstData = data;

    return JHASH_GeneralBuffer(pstData->pucData, pstData->uiLen, 0);
}

static int databox_Cmp(void *hash, void *data1, void *data2)
{
    LDATA_S *pstData1 = data1;
    LDATA_S *pstData2 = data2;

    return MEM_Cmp(pstData1->pucData, pstData1->uiLen,
            pstData2->pucData, pstData2->uiLen);
}

int DataBox_Init(BOX_S *box, CUCKOO_HASH_NODE_S *table,
        UINT bucket_num, UINT bucket_depth)
{
    int ret;
    ret = CuckooHash_Init(&box->hash, table, bucket_num, bucket_depth);
    if (ret != 0) {
        return ret;
    }

    CuckooHash_SetIndexFunc(&box->hash, databox_Index);
    CuckooHash_SetCmpFunc(&box->hash, databox_Cmp);

    return 0;
}

