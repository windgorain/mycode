/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "utl/leb_utl.h"
#include "utl/wasm_utl.h"
#include "wasm_def.h"
#include "wasm_func.h"

#define PAGE_COUNT   256 // 64K * 256 = 16MB
#define TOTAL_PAGES  65535
#define TABLE_COUNT  256

WASM_MEMORY_S memory = {PAGE_COUNT, TOTAL_PAGES, PAGE_COUNT, 0};
uint8_t  *__memory_base;
static UCHAR g_memory[PAGE_COUNT * WASM_PAGE_SIZE];

// Initialize memory globals
void WASME_Init() {
    __memory_base = g_memory;
}

