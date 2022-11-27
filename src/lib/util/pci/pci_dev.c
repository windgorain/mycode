/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/bit_opt.h"
#include "utl/pci_utl.h"
#include "utl/pci_dev.h"

UINT PCIE_DEV_ReadConfig(PCIE_DEV_S *dev, int addr)
{
    BS_DBGASSERT((addr & 0x3) == 0);

    if (addr >= sizeof(dev->cfg)) {
        return 0;
    }

    UINT *d = (void*)&dev->cfg;

    return d[addr/4];
}

int PCIE_DEV_WriteConfig(PCIE_DEV_S *dev, int addr, UINT val, UCHAR first_be)
{
    int i;
    UCHAR *d = (void*)&dev->cfg;
    UCHAR *s = (void*)&val;
    UCHAR *msk = (void*)&dev->writable_bits;

    BS_DBGASSERT((addr & 0x3) == 0);

    if (addr >= sizeof(dev->cfg)) {
        RETURN(BS_ERR);
    }

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

    return 0;
}

static UINT _pcie_dev_read_bar0(PCIE_DEV_S *dev, UINT addr)
{
    if (addr >= sizeof(dev->bar0)) {
        return 0;
    }

    return dev->bar0[addr/4];
}

static int _pcie_dev_write_bar0(PCIE_DEV_S *dev, UINT addr, UINT val, UCHAR first_be)
{
    int i;
    UCHAR *d = (void*)&dev->bar0[0];
    UCHAR *s = (void*)&val;

    BS_DBGASSERT((addr & 0x3) == 0);

    if (addr >= sizeof(dev->bar0)) {
        RETURN(BS_ERR);
    }

    d += addr;

    for (i=0; i<4; i++) {
        if (first_be & (1 << i)) {
            d[i] = s[i];
        }
    }

    return 0;
}

UINT PCIE_DEV_ReadBar(PCIE_DEV_S *dev, int bar, UINT addr)
{
    UINT v = 0;

    BS_DBGASSERT((addr & 0x3) == 0);

    if (bar > 5) {
        return v;
    }

    switch (bar) {
        case 0:
            v = _pcie_dev_read_bar0(dev, addr);
            break;
        default:
            break;
    }

    return v;
}

int PCIE_DEV_WriteBar(PCIE_DEV_S *dev, int bar, UINT addr, UINT val, UCHAR first_be)
{
    int ret;

    BS_DBGASSERT((addr & 0x3) == 0);

    if (bar > 5) {
        RETURN(BS_ERR);
    }

    switch (bar) {
        case 0:
            ret = _pcie_dev_write_bar0(dev, addr, val, first_be);
            break;
        default:
            RETURN(BS_NOT_SUPPORT);
    }

    return ret;
}
