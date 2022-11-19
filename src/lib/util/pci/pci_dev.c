/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/bit_opt.h"
#include "utl/pci_utl.h"
#include "utl/pci_dev.h"

UINT PCIE_DevCfg_Read(PCIE_DEV_S *dev, int addr, int size)
{
    UCHAR *d = (void*)&dev->cfg;
    UINT val = 0;

    d += addr;

    switch (size) {
        case 1:
            val = *d;
            break;
        case 2:
            val = *(USHORT*)d;
            break;
        case 4:
            val = *(UINT*)d;
            break;
    }

    return val;
}

void PCIE_DevCfg_Write(PCIE_DEV_S *dev, int addr, UINT val, UCHAR first_be)
{
    int i;
    UCHAR *d = (void*)&dev->cfg;
    UCHAR *s = (void*)&val;
    UCHAR *msk = (void*)&dev->writable_bits;

    d += addr;
    msk += addr;

    for (i=0; i<4; i++) {
        if (first_be & (1 << i)) {
            if (! msk[i]) {
                continue;
            }
            BIT_SETTO(d[i], msk[i], s[i]);
        }
    }

    return;
}


