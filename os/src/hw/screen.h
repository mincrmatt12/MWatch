#pragma once

#include <mwk/ecf/task.h>
#include <mwk/exc/broadcaster.h>
#include <stdint.h>

namespace mwos::screen {
	// TODO
	struct screen_evt {};
	extern mwk::exc::async_broadcaster<screen_evt> frame_finished_evt;

	extern uint16_t fb_data[120*240];

	mwk::task<void> power_up();   // perform screen init  ( setup voltage, setup gpios, send blank frame, enable vcom, etc. )
	// void power_off();  // perform screen deinit
	
	void scanout_frame(); // async start scanning a frame
	bool is_finished_scanning();
	inline auto frame_finished() {
		return frame_finished_evt.wait();
	}
	inline auto scanout_and_wait_frame() {
		scanout_frame();
		return frame_finished();
	}

	void dma_irq0_channel1_handler();

	// framebuffer helpers
	
	static inline void set_pixel(int x, int y, uint8_t color) {
		// TODO: #ifdef rotation

		x = 240 - x;
		y = 240 - y;

		uint16_t old = fb_data[(x / 2) + y*120];
		if (x % 2)
			fb_data[(x / 2) + y*120] = (old & ~0b00'111'000'00'111'000) | ((color & 0b111) << 3) | ((color & 0b111'000) << 8);
		else
			fb_data[(x / 2) + y*120] = (old & ~0b00'000'111'00'000'111) | ((color & 0b111) << 0) | ((color & 0b111'000) << 5);
	}
}
