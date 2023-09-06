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
#include "hw/popcount/popcount.h"


#define DMA_OFFSET        0x1000
#define REG_MM2S_DMACR    0x0 + DMA_OFFSET
#define REG_MM2S_DMASR    0x4 + DMA_OFFSET
#define REG_MM2S_SA       0x18 + DMA_OFFSET
#define REG_MM2S_LENGTH   0x28 + DMA_OFFSET

/* Read Callback for the popcount function */
static uint64_t pop_read(void *opaque, hwaddr addr, unsigned int size)
{
    /* Aquiring state */
    popState *s = opaque;

    /* Log guest error for debugging */
    qemu_log_mask(LOG_GUEST_ERROR, "%s: read: addr=0x%x size=%d\n",
                  __func__, (int)addr,size);

    /* Return the current bitcount */
    return s->bitcount;
}

/* Actual popcount computation */
static uint32_t popcount(uint32_t val){
    uint32_t count;
    for (count=0; val; count++){
        val &= val - 1;
    }
    return count; 
}

/* Write callback for main popcount function */
static void pop_write(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{
    popState *s = opaque;
    uint32_t value = val64; /*this line is for full correctness - uint32_t*/
    (void)s;

    s->write_reg = value;
    s->bitcount += popcount(value);

    qemu_log_mask(LOG_GUEST_ERROR, "Wrote: %x to %x", value, (int)addr);
}

static uint64_t dma_read(void *opaque, hwaddr addr, unsigned int size){
  popState *s = opaque;
  uint32_t reduced_addr = addr - s->base;

  switch (reduced_addr) {
    case REG_MM2S_DMACR:
      return s->CR_reg;
    case REG_MM2S_DMASR:
      return s->SR_reg;
    case REG_MM2S_SA:
      return s->SA_reg;
    case REG_MM2S_LENGTH:
      return s->LEN_reg;
    default:
      return 0x51515151;
  }
}

static void dma_write(void *opaque, hwaddr addr, uint64_t val64, unsigned int size){
  popState *s = opaque;
  uint32_t value = val64;
  uint32_t reduced_addr = addr - s->base;
  switch (reduced_addr) {
    case REG_MM2S_DMACR:
      s->CR_reg=value;
      break;
    case REG_MM2S_DMASR:
      s->SR_reg=value;
      break;
    case REG_MM2S_SA:
      s->SA_reg = value;
      break;
    default:
      break;
  }
}

static void MM2S_LENGTH_write(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{
  popState *s = opaque;
  // 31-26 are reserved bits! see:
  // https://docs.xilinx.com/r/en-US/pg021_axi_dma/MM2S_LENGTH-MM2S-DMA-Transfer-Length-Register-Offset-28h
  uint32_t value = val64 & 0x3FFFFFF;
  //uint32_t buffer[8]; // No idea how this works, but for some reason it does...
    //
  // set length
  s->LEN_reg = value;
  uint32_t buffer[value/4];
  // DMA Transfer``
  // cpu_physical_memory_read(hwaddr addr, void* buffer, hwaddr len)
  // len is number of bytes uint32 = 4 bytes
  cpu_physical_memory_read(s->SA_reg, &buffer, value);
  uint32_t *a = buffer;
  for (int i = 0; i < (int)value/4; i++){
      cpu_physical_memory_write(0x40000004, a, 4);
      a++;
  }
  //cpu_physical_memory_read(0x40001000, &buffer, 4);
  //cpu_physical_memory_write(s->SA_reg, &buffer, 4);
  //s->VALUE = buffer;
  s->LEN_reg = buffer[0]; 
}

/* Initializes the write register */
static void write_reg_init(popState *s){
    s->write_reg = 0;
    s->bitcount = 0;
}

/* Dummy function for reading the reset register - simply logs guest error*/
static uint64_t r_read(void *opaque, hwaddr addr, unsigned int size){
    qemu_log_mask(LOG_GUEST_ERROR, "READING RESET IS NOT USEFUL");
    return 0;
}

/* Reset Callback - resets the count and write register */
static void r_write(void *opaque, hwaddr addr, uint64_t val64, unsigned int size){
    popState *s = opaque;
    uint32_t value = val64;
    (void)s;

    if (value != 0){
        s->bitcount = 0;
        s->write_reg = 0;
    } 

}

/* Memory operation binding for popcount region */
static const MemoryRegionOps pop_ops = {
    .read = pop_read,
    .write = pop_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1*sizeof(int),
        .max_access_size = 2*sizeof(int)}
};

/* Memory operation binding for reset region */
static const MemoryRegionOps r_ops = {
    .read = r_read,
    .write = r_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4}
};

static const MemoryRegionOps dma_ops = {
  .read = dma_read,
  .write = dma_write,
  .endianness = DEVICE_NATIVE_ENDIAN,
  .valid = {
    .min_access_size = 1,
    .max_access_size = 64}
};

static const MemoryRegionOps dma_len_ops = {
  .read = dma_read,
  .write = MM2S_LENGTH_write,
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
    memory_region_init_io(&s->reset, NULL, &r_ops, s, TYPE_POPCOUNT, 4);
    memory_region_init_io(&s->mmio, NULL, &pop_ops, s, TYPE_POPCOUNT, 32);
    memory_region_init_io(&s->MM2S_DMACR, NULL, &dma_ops, s, TYPE_POPCOUNT, 32);
    memory_region_init_io(&s->MM2S_DMASR, NULL, &dma_ops, s, TYPE_POPCOUNT, 32);
    memory_region_init_io(&s->MM2S_SA, NULL, &dma_ops, s, TYPE_POPCOUNT, 32);
    memory_region_init_io(&s->MM2S_LENGTH, NULL, &dma_len_ops, s, TYPE_POPCOUNT, 32);
    memory_region_add_subregion(address_space, base, &s->reset);
    memory_region_add_subregion(address_space, base+4, &s->mmio);
    memory_region_add_subregion(address_space, base+DMA_OFFSET, &s->MM2S_DMACR);
    memory_region_add_subregion(address_space, base+DMA_OFFSET+4, &s->MM2S_DMASR);
    memory_region_add_subregion(address_space, base+DMA_OFFSET+0x18, &s->MM2S_SA);
    memory_region_add_subregion(address_space, base+DMA_OFFSET+0x28, &s->MM2S_LENGTH);
    s->base = base;
    return s;
}

