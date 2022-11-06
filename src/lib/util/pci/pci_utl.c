/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/pci_utl.h"

/* 构造pcie config tlp */
void PCIE_BuildCfgTLP(UCHAR fmt, UCHAR type, USHORT bdf, UINT addr, int size, OUT PCIE_TLP_CFG_S *tlp)
{
    UINT v = 0;
    int i;

    PCIE_SET_TLP_FMT(tlp, fmt);
    PCIE_SET_TLP_TYPE(tlp, type);
    PCIE_SET_TLP_CFG_REQUEST_ID(tlp, bdf);
    PCIE_SET_TLP_CFG_BDF(tlp, bdf);

    PCIE_SET_TLP_LENGTH(tlp, 1);

    PCIE_SET_TLP_CFG_REG_NUM(tlp, addr/4);

    for (i=(addr % 4); i<4; i++) {
        if (i < size) {
            v |= (1 << i);
        }
    }
    PCIE_SET_TLP_CFG_FIRST_DW(tlp, v);
}

/* 构造pcie config read tlp */
void PCIE_BuildCfgReadTLP(int is_ep, USHORT bdf, UINT addr, int size, OUT PCIE_TLP_CFG_S *tlp)
{
    if (is_ep) {
        PCIE_BuildCfgTLP(0, 0b00100, bdf, addr, size, tlp);
    } else {
        PCIE_BuildCfgTLP(0, 0b00101, bdf, addr, size, tlp);
    }
}

/* 构造pcie config write tlp */
void PCIE_BuildCfgWriteTLP(int is_ep, USHORT bdf, UINT addr, int size, UINT val, OUT PCIE_TLP_CFG_S *tlp)
{
    if (is_ep) {
        PCIE_BuildCfgTLP(0b010, 0b00100, bdf, addr, size, tlp);
    } else {
        PCIE_BuildCfgTLP(0b010, 0b00101, bdf, addr, size, tlp);
    }

    PCIE_WriteTlpData(&val, size, 0, (void*)tlp);
}

/* 构造pcie mem tlp */
void PCIE_BuildMemTLP(UCHAR fmt, UCHAR type, USHORT bdf, UINT64 addr, BOOL_T is64, int size, OUT PCIE_TLP_MEM_S *tlp)
{
    int i;
    UINT v;
    UINT end_addr = addr + size;

    PCIE_SET_TLP_FMT(tlp, fmt);
    PCIE_SET_TLP_TYPE(tlp, type);
    PCIE_SET_TLP_CFG_REQUEST_ID(tlp, bdf);
    PCIE_SET_TLP_LENGTH(tlp, (size+3)/4);

    if (is64) {
        tlp->addr1 = (addr >> 32);
        tlp->addr2 = addr & 0xfffffffc;
    } else {
        tlp->addr1 = addr & 0xfffffffc;
    }

    v = 0;
    for (i=(addr % 4); i<4; i++) {
        if (i < size) {
            v |= (1 << i);
        }
    }
    PCIE_SET_TLP_CFG_FIRST_DW(tlp, v);

    v = 0;
    for (i=0; i<(end_addr % 4); i++) {
        v |= (1 << i);
    }
    PCIE_SET_TLP_CFG_LAST_DW(tlp, v);

}

/* 构造pcie ep mem read tlp */
void PCIE_BuildEpMemReadTLP(USHORT bdf, UINT addr, BOOL_T is64, int size, OUT PCIE_TLP_MEM_S *tlp)
{
    if (is64) {
        PCIE_BuildMemTLP(1, 0, bdf, addr, is64, size, tlp);
    } else {
        PCIE_BuildMemTLP(0, 0, bdf, addr, is64, size, tlp);
    }
}

/* 构造pcie ep mem write tlp */
void PCIE_BuildEpMemWriteTLP(USHORT bdf, UINT addr, BOOL_T is64, int size, void *data, OUT PCIE_TLP_MEM_S *tlp)
{
    if (is64) {
        PCIE_BuildMemTLP(3, 0, bdf, addr, is64, size, tlp);
    } else {
        PCIE_BuildMemTLP(2, 0, bdf, addr, is64, size, tlp);
    }
    PCIE_WriteTlpData(data, size, is64, (void*)tlp);
}

void PCIE_WriteTlpData(void *data, int data_len, BOOL_T is4dw, OUT PCIE_TLP_S *tlp)
{
    char *d = (void*)tlp;

    d += 12;
    if (is4dw) {
        d += 4;
    } 

    memcpy(d, data, data_len);
}

/* 返回read了多长 */
int PCIE_ReadTlpData(OUT void *data, int data_size, BOOL_T is4dw, PCIE_TLP_S *tlp)
{
    char *d = (void*)tlp;
    int length = PCIE_TLP_LENGTH(tlp);
    int len = MIN(length, data_size);

    d += 12;
    if (is4dw) {
        d += 4;
    } 

    memcpy(data, d, len);

    return len;
}


