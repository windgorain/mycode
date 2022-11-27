/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/pci_dev.h"

int PCI_Vnet_Init(OUT PCIE_DEV_S *dev)
{
    PCIE_DEV_CFG_S *cfg;
    PCIE_DEV_CFG_S *writable_bits;

    memset(dev, 0, sizeof(*dev));

    cfg = &dev->cfg;
    cfg->vendor_id = 0x1af4;
    cfg->device_id = 0x1000;

    writable_bits = &dev->writable_bits;
    writable_bits->command = 0xffff;
    writable_bits->bar0 = 0xfff00000;

    return 0;
}




