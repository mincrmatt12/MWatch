#pragma once

#include <stdint.h>

namespace mwos::screen {
	extern uint8_t fb_data[242*240 + 2]; // actual framebuffer contents ( 2 bytes at end are used for dma )

	struct ls012b7dd06 {
		void power_up();   // perform screen init  ( setup voltage, setup gpios, send blank frame, enable vcom, etc. )
		void power_off();  // perform screen deinit
		
		void scanout_frame();

	private:
		uint32_t reconfig_dma, fifo_dma;
	};

	extern ls012b7dd06 module;
}
