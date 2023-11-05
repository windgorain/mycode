/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _PCI_DEV_H
#define _PCI_DEV_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    USHORT vendor_id;
    USHORT device_id;
    USHORT command;
    USHORT status;
    UCHAR revision_id;
    UCHAR class_code[3];
    UCHAR chache_line_size;
    UCHAR latency_timer;
    UCHAR header_type;
    UCHAR bist;
    UINT bar0;
    UINT bar1;
    UINT bar2;
    UINT bar3;
    UINT bar4;
    UINT bar5;
    UINT cardbus_cis_pointer;
    USHORT subsystem_vendor_id;
    USHORT subsystem_id;
    UINT expansion_rom_base_address;
    UCHAR capabilities_pointer;
    UCHAR reserved[3];
    UINT reserved2;
    UCHAR interrupt_line;
    UCHAR interrupt_pin;
    UCHAR min_gnt;
    UCHAR max_lat;
}PCIE_DEV_CFG_S;

typedef struct {
    PCIE_DEV_CFG_S cfg;
    PCIE_DEV_CFG_S writable_bits; 
    UINT bar0[16];
    UINT bar1[16];
    UINT bar2[16];
    UINT bar3[16];
    UINT bar4[16];
    UINT bar5[16];
}PCIE_DEV_S;

UINT PCIE_DEV_ReadConfig(PCIE_DEV_S *dev, int addr);
int PCIE_DEV_WriteConfig(PCIE_DEV_S *dev, int addr, UINT val, UCHAR first_be);
UINT PCIE_DEV_ReadBar(PCIE_DEV_S *dev, int bar, UINT addr);
int PCIE_DEV_WriteBar(PCIE_DEV_S *dev, int bar, UINT addr, UINT val, UCHAR first_be);

#ifdef __cplusplus
}
#endif
#endif 
