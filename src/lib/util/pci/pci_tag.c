/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/pci_utl.h"
#include "utl/pci_tag.h"

void PCIE_TAG_Init(OUT PCIE_TAG_S *tags)
{
    int i;
    memset(tags, 0, sizeof(PCIE_TAG_S));

    for (i=0; i<PCIE_TAG_MAX; i++) {
        tags->node[i].sem = SEM_CCreate("pcie_tag", 0);
    }

    MUTEX_Init(&tags->lock);
}

static int pcie_tag_alloc_tag(PCIE_TAG_S *tags)
{
    int i;
    for (i=0; i<PCIE_TAG_MAX; i++) {
        if (! tags->node[i].used) {
            tags->node[i].used = 1;
            return i;
        }
    }

    return -1;
}

int PCIE_TAG_AllocTag(PCIE_TAG_S *tags)
{
    int ret;
    MUTEX_P(&tags->lock);
    ret = pcie_tag_alloc_tag(tags);
    MUTEX_V(&tags->lock);

    return ret;
}

static void pcie_tag_free_tag(PCIE_TAG_S *tags, int tag)
{
    tags->node[tag].used = 0;
}

void PCIE_TAG_FreeTag(PCIE_TAG_S *tags, int tag)
{
    MUTEX_P(&tags->lock);
    pcie_tag_free_tag(tags, tag);
    MUTEX_V(&tags->lock);
}

static void pcie_tag_wait(PCIE_TAG_S *tags, int tag)
{
    SEM_P(tags->node[tag].sem, BS_WAIT, BS_WAIT_FOREVER);
}

static void pcie_tag_wakeup(PCIE_TAG_S *tags, int tag)
{
    SEM_V(tags->node[tag].sem);
}

void PCIE_TAG_WriteTlp(PCIE_TAG_S *tags, void *tlp, int tlp_len)
{
    PCIE_TLP_COMPLETE_S *complete_tlp = tlp;
    int tag = complete_tlp->tag;
    memcpy(&tags->node[tag].tlp, tlp, tlp_len);
    pcie_tag_wakeup(tags, tag);
}

PCIE_TLP_S * PCIE_TAG_GetTlp(PCIE_TAG_S *tags, int tag)
{
    pcie_tag_wait(tags, tag);
    return &tags->node[tag].tlp;
}

