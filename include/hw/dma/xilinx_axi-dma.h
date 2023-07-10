#ifndef HW_XILINX_AXI_DMA_H
#define HW_XILINX_AXI_DMA_H

#include "hw/sysbus.h" /* Defines SysBusDevice, MemoryRegion and AddressSapce*/
#include "hw/stream.h" /* Defines the StreamSink Object */
#include "qom/object.h" 

#define TYPE_XILINX_AXI_DMA "xlnx.axi-dma"
#define TYPE_XILINX_AXI_DMA_DATA_STREAM "xilinx-axi-dma-data-stream"
#define TYPE_XILINX_AXI_DMA_CONTROL_STREAM "xilinx-axi-dma-control-stream"

OBJECT_DECLARE_SIMPLE_TYPE(XilinxAXIDMA, XILINX_AXI_DMA)

typedef struct XilinxAXIDMA XilinxAXIDMA;
typedef struct XilinxAXIDMAStreamSink XilinxAXIDMAStreamSink;

DECLARE_INSTANCE_CHECKER(XilinxAXIDMAStreamSink, XILINX_AXI_DMA_DATA_STREAM,
                         TYPE_XILINX_AXI_DMA_DATA_STREAM)

DECLARE_INSTANCE_CHECKER(XilinxAXIDMAStreamSink, XILINX_AXI_DMA_CONTROL_STREAM,
                         TYPE_XILINX_AXI_DMA_CONTROL_STREAM)

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
