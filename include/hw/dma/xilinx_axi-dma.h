#ifndef HW_XILINX_AXI_DMA_H
#define HW_XILINX_AXI_DMA_H

#include "hw/sysbus.h" /* Defines SysBusDevice, MemoryRegion and AddressSapce*/
#include "hw/stream.h" /* Defines the StreamSink Object */
#include "qom/object.h" 
#include "hw/ptimer.h" /* Needed for ptimer in Stream */

#define TYPE_XILINX_AXI_DMA "xlnx.axi-dma"
#define TYPE_XILINX_AXI_DMA_DATA_STREAM "xilinx-axi-dma-data-stream"
#define TYPE_XILINX_AXI_DMA_CONTROL_STREAM "xilinx-axi-dma-control-stream"

#define R_DMACR             (0x00 / 4)
#define R_DMASR             (0x04 / 4)
#define R_CURDESC           (0x08 / 4)
#define R_TAILDESC          (0x10 / 4)
#define R_MAX               (0x30 / 4)

#define CONTROL_PAYLOAD_WORDS 5
#define CONTROL_PAYLOAD_SIZE (CONTROL_PAYLOAD_WORDS * (sizeof(uint32_t)))

OBJECT_DECLARE_SIMPLE_TYPE(XilinxAXIDMA, XILINX_AXI_DMA)

typedef struct XilinxAXIDMA XilinxAXIDMA;
typedef struct XilinxAXIDMAStreamSink XilinxAXIDMAStreamSink;

DECLARE_INSTANCE_CHECKER(XilinxAXIDMAStreamSink, XILINX_AXI_DMA_DATA_STREAM,
                         TYPE_XILINX_AXI_DMA_DATA_STREAM)

DECLARE_INSTANCE_CHECKER(XilinxAXIDMAStreamSink, XILINX_AXI_DMA_CONTROL_STREAM,
                         TYPE_XILINX_AXI_DMA_CONTROL_STREAM)

enum {
    DMACR_RUNSTOP = 1,
    DMACR_TAILPTR_MODE = 2,
    DMACR_RESET = 4
};

enum {
    DMASR_HALTED = 1,
    DMASR_IDLE  = 2,
    DMASR_IOC_IRQ  = 1 << 12,
    DMASR_DLY_IRQ  = 1 << 13,

    DMASR_IRQ_MASK = 7 << 12
};

struct SDesc {
    uint64_t nxtdesc;
    uint64_t buffer_address;
    uint64_t reserved;
    uint32_t control;
    uint32_t status;
    uint8_t app[CONTROL_PAYLOAD_SIZE];
};

enum {
    SDESC_CTRL_EOF = (1 << 26),
    SDESC_CTRL_SOF = (1 << 27),

    SDESC_CTRL_LEN_MASK = (1 << 23) - 1
};

enum {
    SDESC_STATUS_EOF = (1 << 26),
    SDESC_STATUS_SOF_BIT = 27,
    SDESC_STATUS_SOF = (1 << SDESC_STATUS_SOF_BIT),
    SDESC_STATUS_COMPLETE = (1 << 31)
};

struct Stream {
    struct XilinxAXIDMA *dma;
    ptimer_state *ptimer;
    qemu_irq irq;

    int nr;

    bool sof;
    struct SDesc desc;
    unsigned int complete_cnt;
    uint32_t regs[R_MAX];
    uint8_t app[20];
    unsigned char txbuf[16 * 1024];
};

struct XilinxAXIDMAStreamSink {
    Object parent;

    struct XilinxAXIDMA *dma;
};

struct XilinxAXIDMA {
    SysBusDevice busdev;
    MemoryRegion iomem;
    MemoryRegion *dma_mr;
    AddressSpace as;

    uint32_t freqhz;
    StreamSink *tx_data_dev;
    StreamSink *tx_control_dev;
    XilinxAXIDMAStreamSink rx_data_dev;
    XilinxAXIDMAStreamSink rx_control_dev;

    struct Stream streams[2];

    StreamCanPushNotifyFn notify;
    void *notify_opaque;
};

#endif /* HW_XILINX_AXI_DMA_H */
