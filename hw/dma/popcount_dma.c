/* AUTHOR:       Matteo Vidali
 * AUTHOR EMAIL: mvidali@iu.edu - mmvidali@gmail.com
 * 
 * DESC:
 *   Written for the Indiana University Course E315 as an AUTOGRADER tool
 * This hardware device acts like the hardware on the PYNQ-Zynq7000 board
 * with a popcount bitstream.
 * It contains two memory regions (reset, popcount) which are responsible for 
 * computing popcount. It wiil be established in the arm-virt machine, as a
 * UIO device. There will be a corresponding kernel module present for this
 * device as well.
 *
 * For questions regarding design or maintenance of this software, contact
 * Matteo Vidali at the email address listed above, or Dr. Andrew Lukefahr at 
 * whatever email or phone number you can find.
 */
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "chardev/char.h"
#include "hw/hw.h"
#include "hw/irq.h"
#include "hw/dma/popcount_dma.h"


#define REG_MM2S_DMACR    0x0
#define REG_MM2S_DMASR    0x4
#define REG_MM2S_SA       0x18
#define REG_MM2S_LENGTH   0x28


/* Read Callback for the popcount function */
static uint64_t MM2S_DMACR_read(void *opaque, hwaddr addr, unsigned int size)
{
    /* Aquiring state */
    pdmaState *s = opaque;

    /* Log guest error for debugging */
    qemu_log_mask(LOG_GUEST_ERROR, "%s: read: addr=0x%x size=%d\n",
                  __func__, (int)addr,size);

    /* Return the current bitcount */
    return s->bitcount;
}

/* Write callback for main popcount function */
static void MM2S_DMACR_write(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{
    pdmaState *s = opaque;
    uint32_t value = val64; /*this line is for full correctness - uint32_t*/
    (void)s;

    s->write_reg = value;
    s->bitcount += popcount(value);

    qemu_log_mask(LOG_GUEST_ERROR, "Wrote: %x to %x", value, (int)addr);
}

static uint64_t MM2S_DMASR_read(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{

}
static void MM2S_DMASR_write(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{

}

static uint64_t MM2S_DMA_SA_read(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{

}
static void MM2S_DMA_SA_write(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{

}

static uint64_t MM2S_LENGTH_read(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{

}
static void MM2S_LENGTH_write(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{

}

/* Initializes the write register */
static void write_reg_init(popState *s){
    s->write_reg = 0;
    s->bitcount = 0;
}

/* Memory operation binding for popcount region */
static const MemoryRegionOps DMACR_ops = {
    .read = MM2S_DMACR_read,
    .write = MM2S_DMCR_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1*sizeof(int),
        .max_access_size = 64*sizeof(int)}
};

/* Memory operation binding for reset region */
static const MemoryRegionOps DMASR_ops = {
    .read = MM2S_DMASR_read,
    .write = MM2S_DMASR_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 64}
};

/* Memory operation binding for reset region */
static const MemoryRegionOps DMASA_ops = {
    .read = MM2S_DMA_SA_read,
    .write = MM2S_DMA_SA_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 64}
};

/* Memory operation binding for reset region */
static const MemoryRegionOps DMALEN_ops = {
    .read = MM2S_DMA_LENGTH_read,
    .write = MM2S_DMA_LENGTH_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 64}
};
/* Initialization of the popcount hardware DEVICE_NATIVE_ENDIAN
 * May be desirable to make this of type void in the future to match 
 * QEMU hw device protocol more typically
 */
popState *popcount_create(MemoryRegion *address_space, hwaddr base)
{
    popState *s = g_malloc0(sizeof(popState));
    write_reg_init(s);
    memory_region_init_io(&s->MM2S_DMACR, NULL, &DMACR_ops, s, TYPE_POPCOUNT, 32);
    memory_region_init_io(&s->MM2S_DMASR, NULL, &DMASR_ops, s, TYPE_POPCOUNT, 32);
    memory_region_init_io(&s->MM2S_SA, NULL, &DMASA_ops, s, TYPE_POPCOUNT, 32);
    memory_region_init_io(&s->MM2S_LENGTH, NULL, &DMALEN_ops, s, TYPE_POPCOUNT, 32);
    memory_region_add_subregion(address_space, base, &s->MM2S_DMACR);
    memory_region_add_subregion(address_space, base+4, &s->MM2S_DMASR);
    memory_region_add_subregion(address_space, base+0x18, &s->MM2S_SA);
    memory_region_add_subregion(address_space, base+0x28, &s->MM2S_LENGTH);
    return s;
}

