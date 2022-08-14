#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/exec_utl.h"
#include "utl/num_utl.h"
#include "utl/txt_utl.h"
#include "utl/atomic_once.h"
#include "utl/mem_utl.h"
#include "utl/list_dtq.h"
#include "mem_bs.h"

#ifndef IN_DEBUG

BS_STATUS MemDebug_ShowSizeOfMem(int argc, char **argv)
{
    return 0;
}

BS_STATUS MemDebug_ShowLineConflict(int argc, char **argv)
{
    return 0;
}

int MemDebug_Check(int argc, char **argv)
{
    return 0;
}

#endif
