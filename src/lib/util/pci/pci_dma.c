/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/pci_utl.h"
#include "utl/pci_dma.h"
#include "utl/bit_opt.h"

#define PCIE_RCB 512

static void _pci_dma_process_mwr_be(UINT64 addr, UCHAR *data, UCHAR be,
        PF_PCI_DMA_WRITE dma_write_func, void *user_data)
{
    int i;

    for (i=0; i<4; i++) {
        if (be & (1 << i)) {
            dma_write_func(addr + i, data + i, 1, user_data);
        }
    }
}

int PCI_DMA_ProcessMwr(void *msg, int len, PF_PCI_DMA_WRITE dma_write_func, void *user_data)
{
    PCIE_TLP_MEM_S *tlp = msg;
    UINT64 addr;
    UINT64 last_dw_addr;
    UCHAR first_be;
    UCHAR last_be;
    int length;
    UCHAR *data;

    length = PCIE_GetTlpDataLength(msg);
    first_be = PCIE_TLP_CFG_FIRST_BE(tlp);
    last_be = PCIE_TLP_CFG_LAST_BE(tlp);
    data = PCIE_GetTlpDataPtr(tlp);
    addr = PCIE_GetMemTlpAddr(msg);
    last_dw_addr = PCIE_GetLastDwAddr(addr, length);

    _pci_dma_process_mwr_be(addr, data, first_be, dma_write_func, user_data);

    length -= 4;
    if (length <= 0) {
        return 0;
    }

    addr += 4;
    data += 4;

    if (last_be) {
        length -= 4;
        _pci_dma_process_mwr_be(last_dw_addr, data + length, last_be, dma_write_func, user_data);
    }

    if (length > 0) {
        dma_write_func(addr, data, length, user_data);
    }

    return 0;
}

/* 处理dma读消息, 并进行应答 */
int PCI_DMA_ProcessMrd(PCIE_TLP_MEM_S *tlp, int len,
        PF_PCI_DMA_READ dma_read_func,
        PF_PCI_SEND_DATA send_func,
        void *user_data)
{
    UINT64 addr;
    UINT64 last_dw_addr;
    UCHAR first_be;
    UCHAR last_be;
    int length;
    PCIE_TLP_S reply_msg = {0};
    UINT low_addr;
    UINT last_drop = 0;
    UINT byte_count;
    UINT size;
    UINT copy_size;
    char buf[PCIE_RCB];

    length = PCIE_GetTlpDataLength((void*)tlp);
    first_be = PCIE_TLP_CFG_FIRST_BE(tlp);
    last_be = PCIE_TLP_CFG_LAST_BE(tlp);
    addr = PCIE_GetMemTlpAddr(tlp);
    last_dw_addr = PCIE_GetLastDwAddr(addr, length);
    low_addr = PCIE_FirstBe2LowAddr(addr, first_be);

    /* 根据last_be计算最后需要少拷贝几个字节 */
    if (last_be) {
        last_drop = 3 - BIT_GetFirstIndex(last_be, 3);
    }

    byte_count = length;

    while (byte_count > 0) {
        size = PCIE_RCB - (addr % PCIE_RCB);
        size = MIN(size, byte_count);
        copy_size = size - (low_addr & 0x3);
        if (addr + copy_size > last_dw_addr) {
            copy_size -= last_drop;
        }
        dma_read_func(addr + (low_addr & 0x3), buf, copy_size, user_data);
        PCIE_BuildCpldTLP(0, tlp->request_id, byte_count, low_addr, copy_size, buf, (void*)&reply_msg);
        send_func(&reply_msg, PCIE_TLP_3DW + size, user_data);
        byte_count -= size;
        addr += size;
        low_addr = 0;
    }

    return 0;
}


