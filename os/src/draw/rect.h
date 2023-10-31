#pragma once

#include <stdint.h>

namespace mwos::draw {
	// Fill the LCD with a color
	void fill(uint8_t color);

	// Draw a filled rectangle
	void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
}
