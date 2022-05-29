#pragma once

#include <stdint.h>

namespace mwos::screen {
	extern uint8_t fb_data[240*240]; // actual framebuffer contents ( 2 bytes at end are used for dma )

	struct ls012b7dd06 {
		void power_up();   // perform screen init  ( setup voltage, setup gpios, send blank frame, enable vcom, etc. )
		void power_off();  // perform screen deinit
		
		void scanout_frame();
		bool is_finished_scanning();
	};

	extern ls012b7dd06 module;
}
