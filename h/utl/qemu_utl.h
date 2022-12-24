/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _QEMU_UTL_H
#define _QEMU_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

static inline USHORT QEMU_GetBdf(PCIDevice *pci_dev)
{
    UCHAR bus = pci_dev_bus_num(pci_dev);
    UCHAR df = pci_dev->devfn;
    USHORT bdf = ((UINT)bus) << 8 | df;
    return bdf;
}

static inline char * QEMU_Bdf2String(USHORT bdf, OUT char *buf, int buf_size)
{
    snprintf(buf, buf_size, "%02x:%02x.%u", bdf >> 8, (bdf & 0xff) >> 3, bdf & 7);
    return buf;
}

/* 判断bar是否64位 */
static inline int QEMU_Is64BitBar(PCIDevice *pci_dev, int bar)
{
    unsigned int addr = bar * 4 + 0x10;
    unsigned int bar_val;

    /* 获取bar寄存器内容 */
    bar_val = pci_default_read_config(pci_dev, addr, 4);

    /* 判断是否64位 */
    if ((bar_val & 0x6) == 0) {
        /* 32位 */
        return 0;
    }

    return 1;
}

/* 获取指定bar中的地址 */
static inline UINT64 QEMU_GetBarAddr(PCIDevice *pci_dev, int bar)
{
    unsigned int addr = bar * 4 + 0x10;
    unsigned int bar_val;
    unsigned int bar_val2;
    unsigned long long addr64;

    /* 获取bar寄存器内容 */
    bar_val = pci_default_read_config(pci_dev, addr, 4);

    /* 判断是否64位 */
    if ((bar_val & 0x6) == 0) {
        /* 32位 */
        return bar_val & 0xfffff000;
    }

    /* 64位, 读取下一个bar */
    addr += 4;
    bar_val2 = pci_default_read_config(pci_dev, addr, 4);

    addr64 = bar_val2;
    addr64 = addr64 << 32;
    addr64 |= (bar_val & 0xfffff000);

    return addr64;
}

/* 定义bar的读写函数 */
#define QEMU_BAR_FUNC_DEF(id, read_func, write_func, ud) \
static uint64_t _qemu_bar##id##_read(void *opaque, hwaddr addr, unsigned int len) { \
    return read_func(opaque, id, addr, len, ud); \
} \
static void _qemu_bar##id##_write(void *opaque, hwaddr addr, uint64_t val, unsigned int len) { \
    write_func(opaque, id, addr, val, len, ud); \
}

#define QEMU_ALL_BAR_FUNC_DEF(read_func, write_func, ud) \
    QEMU_BAR_FUNC_DEF(0, read_func, write_func, ud) \
    QEMU_BAR_FUNC_DEF(1, read_func, write_func, ud) \
    QEMU_BAR_FUNC_DEF(2, read_func, write_func, ud) \
    QEMU_BAR_FUNC_DEF(3, read_func, write_func, ud) \
    QEMU_BAR_FUNC_DEF(4, read_func, write_func, ud) \
    QEMU_BAR_FUNC_DEF(5, read_func, write_func, ud)


/* 定义bar opt结构 */
#define QEMU_BAR_OPT_DEF(id, _min_size, _max_size)  { \
    .read = _qemu_bar##id##_read, \
    .write = _qemu_bar##id##_write, \
    .endianness = DEVICE_NATIVE_ENDIAN, \
    .impl = { \
        .min_access_size = (_min_size), \
        .max_access_size = (_max_size), \
    } \
}

#define QEMU_ALL_BAR_OPT_DEF(_val_name, _min_size, _max_size) \
static const MemoryRegionOps _val_name[] = { \
    QEMU_BAR_OPT_DEF(0, _min_size, _max_size), \
    QEMU_BAR_OPT_DEF(1, _min_size, _max_size), \
    QEMU_BAR_OPT_DEF(2, _min_size, _max_size), \
    QEMU_BAR_OPT_DEF(3, _min_size, _max_size), \
    QEMU_BAR_OPT_DEF(4, _min_size, _max_size), \
    QEMU_BAR_OPT_DEF(5, _min_size, _max_size), \
};

static inline void QEMU_InitBars(void *pci_dev, MemoryRegion *bars, const MemoryRegionOps *opts,
        const char *name_prefix, unsigned int *bar_layout)
{
    int i;
    char name[64];

    for (i=0; i<6; i++) {
        if (! bar_layout[i]) {
            continue;
        }
        snprintf(name, sizeof(name), "%s%d", name_prefix, i);
        memory_region_init_io(&bars[i], OBJECT(pci_dev), &opts[i], pci_dev, name, bar_layout[i]);
        pci_register_bar(pci_dev, i, PCI_BASE_ADDRESS_SPACE_MEMORY, &bars[i]);
    }
}

#ifdef __cplusplus
}
#endif
#endif //QEMU_UTL_H_
