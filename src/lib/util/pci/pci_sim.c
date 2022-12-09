/*********************************************************
*   Copyright (C) LiXingang
*   Description: 模拟pcie device行为
*
********************************************************/
#include "bs.h"
#include "utl/pci_utl.h"
#include "utl/pci_dev.h"
#include "utl/pci_sim.h"

static int _pcie_sim_process_cfg_read(PCIE_SIM_S *sim, void *msg)
{
    PCIE_TLP_CFG_S *tlp = msg;
    PCIE_TLP_CFG_CPLD_S cpld = {0};

    UINT size = PCIE_GetTlpDataLength((void*)tlp);
    UINT first_be = PCIE_TLP_CFG_FIRST_BE(tlp);
    UINT addr = PCIE_TLP_CFG_REG_NUM(tlp) * 4;
    UINT low_addr = PCIE_FirstBe2LowAddr(addr, first_be);

    if (size > 4) {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    UINT v = PCIE_DEV_ReadConfig(&sim->dev, addr);

    v = htonl(v);

    PCIE_BuildCpldTLP(sim->bdf, tlp->request_id, size, low_addr, size, &v, (void*)&cpld);
    cpld.tlp.tag = tlp->tag;

    return sim->tlp_send(sim, &cpld, PCIE_TLP_3DW + size);
}

static int _pcie_sim_process_cfg_write(PCIE_SIM_S *sim, void *msg, int len)
{
    PCIE_TLP_CFG_S *tlp = msg;
    PCIE_TLP_COMPLETE_S cpl = {0};

    UINT first_be = PCIE_TLP_CFG_FIRST_BE(tlp);
    UINT addr = PCIE_TLP_CFG_REG_NUM(tlp) * 4;
    UINT *v = PCIE_GetTlpDataPtr(tlp);
    UINT val = *v;

    val = ntohl(val);

    PCIE_DEV_WriteConfig(&sim->dev, addr, val, first_be);

    PCIE_BuildCplTLP(sim->bdf, tlp->request_id, &cpl);
    cpl.tag = tlp->tag;

    return sim->tlp_send(sim, &cpl, sizeof(cpl));
}

static int _pcie_sim_process_mrd(PCIE_SIM_S *sim, void *msg, int len)
{
    PCIE_TLP_MEM_S *tlp = msg;
    PCIE_TLP_CFG_CPLD_S cpld = {0};

    UINT size = PCIE_GetTlpDataLength((void*)tlp);
    UINT first_be = PCIE_TLP_CFG_FIRST_BE(tlp);
    UINT addr = PCIE_GetMemTlpAddr(tlp);
    UINT low_addr = PCIE_FirstBe2LowAddr(addr, first_be);

    if (size > 4) {
        BS_DBGASSERT(0);
        RETURN(BS_ERR);
    }

    UINT v = PCIE_DEV_ReadBar(&sim->dev, 0, addr);

    v = htonl(v);

    PCIE_BuildCpldTLP(sim->bdf, tlp->request_id, size, low_addr, size, &v, (void*)&cpld);
    cpld.tlp.tag = tlp->tag;

    return sim->tlp_send(sim, &cpld, PCIE_TLP_3DW + size);
}

static int _pcie_sim_process_mwr(PCIE_SIM_S *sim, void *msg, int len)
{
    PCIE_TLP_MEM_S *tlp = msg;

    UINT first_be = PCIE_TLP_CFG_FIRST_BE(tlp);
    UINT64 addr = PCIE_GetMemTlpAddr(tlp);
    UINT *v = PCIE_GetTlpDataPtr(tlp);
    UINT val = *v;

    val = ntohl(val);

    return PCIE_DEV_WriteBar(&sim->dev, 0, addr, val, first_be);
}

static int _pcie_sim_process_recved_msg(PCIE_SIM_S *sim, void *msg, int len)
{
    int type = PCIE_GetTlpType(msg);
    int ret = 0;

    switch (type) {
        case PCIE_TLP_TYPE_MRD:
        case PCIE_TLP_TYPE_MRDLK:
            ret = _pcie_sim_process_mrd(sim, msg, len);
            break;

        case PCIE_TLP_TYPE_MWR:
            ret = _pcie_sim_process_mwr(sim, msg, len);
            break;

        case PCIE_TLP_TYPE_CPL:
        case PCIE_TLP_TYPE_CPLD:
            break;

        case PCIE_TLP_TYPE_CFGRD0:
        case PCIE_TLP_TYPE_CFGRD1:
            ret = _pcie_sim_process_cfg_read(sim, msg);
            break;

        case PCIE_TLP_TYPE_CFGWR0:
        case PCIE_TLP_TYPE_CFGWR1:
            ret = _pcie_sim_process_cfg_write(sim, msg, len);
            break;

        default:
            RETURNI(BS_NOT_SUPPORT, "Not support tlp type:%d", type);
            break;
    }

    return ret;
}

int PCIE_SIM_TlpInput(PCIE_SIM_S *sim, void *msg, int len)
{
    int ret = _pcie_sim_process_recved_msg(sim, msg, len);
    if (ret < 0) {
        return ret;
    }

    return 0;
}
