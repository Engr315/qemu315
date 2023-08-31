/* AUTHOR:       Matteo Vidali
 * AUTHOR EMAIL: mvidali@iu.edu - mmvidali@gmail.com
 *
 * DESC:
 *
*/

#ifndef HW_PDMA_H
#define HW_PDMA_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_PDMA "PDMA"

typedef struct pdmaState pdmaState;

DECLARE_INSTANCE_CHECKER(pdmaState, PDMA, TYPE_PDMA)

struct pdmaState
{
    SysBusDevice parent_obj;
    MemoryRegion MM2S_DMACR;
    MemoryRegion MM2S_DMASR;
    MemoryRegion MM2S_SA;
    MemoryRegion MM2S_LENGTH;

    uint32_t CR_reg;
    uint32_t SA_reg; // This is uncecessary, its unaccessable in userspace
    uint32_t LEN_reg;
    uint32_t VALUE;
    uint8_t start;
};

pdmaState *pdma_create(MemoryRegion *address_space, hwaddr base);

#endif //HW_PDMA_H
