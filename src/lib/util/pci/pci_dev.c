/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/pci_utl.h"

UINT PCI_CFG_Read(PCIE_DEV_CFG_S *cfg, int addr, int size)
{
    UCHAR *d = (void*)cfg;
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

void PCI_CFG_Write(PCIE_DEV_CFG_S *cfg, int addr, int size, UINT val)
{
    UCHAR *d = (void*)cfg;

    d += addr;

    switch (size) {
        case 1:
            *d = val;
            break;
        case 2:
            *(USHORT*)d = val;
            break;
        case 4:
            *(UINT*)d = val;
            break;
    }
}


