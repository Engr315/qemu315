#ifndef HW_BUTTER_ROBOT_H
#define HW_BUTTER_ROBOT_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_BUTTER_ROBOT   "virt_butter"
#define VIRT_BR(obj)        OBJECT_CHECK(brState, (obj), TYPE_BUTTER_ROBOT) 

typedef struct brState brState;

DECLARE_INSTANCE_CHECKER(brState, BUTTER_ROBOT, TYPE_BUTTER_ROBOT)

struct brState
{
    SysBusDevice    parent_obj;
    MemoryRegion    mmio;
    unsigned char   butterReg[6]; //{'B','U','T','T','E','R'};
    qemu_irq        irq;
};

brState *br_create(MemoryRegion *address_space, hwaddr base);

#endif //HW_BUTTER_ROBOT_H
