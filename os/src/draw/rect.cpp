#include "rect.h"
#include "dim.h"
#include "hw/screen.h"
#include <algorithm>

namespace mwos::draw {
	void fill(uint8_t color) {
		rect(0, 0, 240, 240, color);
	}

	void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
		y0 = std::min(std::max(min_y, y0), max_y);
		y1 = std::min(std::max(min_y, y1), max_y);
		x0 = std::min(std::max(min_x, x0), max_x);
		x1 = std::min(std::max(min_x, x1), max_x);

		if (x0 % 2) {
			for (int16_t y = y0; y < y1; ++y) screen::set_pixel(x0, y, color);
			++x0;
		}
		if (x1 % 2) {
			for (int16_t y = y0; y < y1; ++y) screen::set_pixel(x1 - 1, y, color);
			--x1;
		}
		
		for (int16_t y = y0; y < y1; ++y) {
			for (int16_t x = x0; x < x1; x += 2)
				screen::set_dual_pixel(x, y, color, color);
		}
	}
}
