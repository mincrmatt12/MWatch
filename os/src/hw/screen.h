#pragma once

#include <stdint.h>

namespace mwos::screen {
	extern uint16_t fb_data[120*240];

	void power_up();   // perform screen init  ( setup voltage, setup gpios, send blank frame, enable vcom, etc. )
	void power_off();  // perform screen deinit
	
	void scanout_frame();
	bool is_finished_scanning();

	// framebuffer helpers
	
	static inline void set_pixel(int x, int y, uint8_t color) {
		uint16_t old = fb_data[(x / 2) + y*120];
		if (x % 2)
			fb_data[(x / 2) + y*120] = (old & ~0b00'111'000'00'111'000) | ((color & 0b111) << 3) | ((color & 0b111'000) << 11);
		else
			fb_data[(x / 2) + y*120] = (old & ~0b00'000'111'00'000'111) | ((color & 0b111) << 0) | ((color & 0b111'000) << 8);
	}
}
