#include "screen.h"
#include <hardware/dma.h>

extern "C" void DMA_IRQ_0_Handler() {
	if (dma_channel_get_irq0_status(1)) mwos::screen::dma_irq0_channel1_handler();
}
