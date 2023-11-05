/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _PCI_SIM_H
#define _PCI_SIM_H

#include "utl/pci_dev.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*PF_PCIE_TLP_SEND)(void *sim, void *data, int len);

typedef struct {
    USHORT bdf;
    void *user_data;
    PCIE_DEV_S dev;
    PF_PCIE_TLP_SEND tlp_send;
    PF_PRINT_FUNC print_func;
}PCIE_SIM_S;

int PCIE_SIM_TlpInput(PCIE_SIM_S *sim, void *msg, int len);

#ifdef __cplusplus
}
#endif
#endif 
