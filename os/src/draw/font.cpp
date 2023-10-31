#include "font.h"
#include "../hw/screen.h"
#include "dim.h"
#include <algorithm>

namespace mwos::draw {
	void raw_write_glyph(int16_t x, int16_t y, const font::metrics_t& metrics, uint8_t color) {
		for (int16_t cursor_y = std::max(min_y, y); cursor_y < y + metrics.height; ++cursor_y) {
			if (cursor_y >= max_y) break;
			for (int16_t cursor_x = std::max(min_x, x); cursor_x < x + metrics.width; ++cursor_x) {
				if (cursor_x >= max_x) break;
				// Lookup value in data
				uint8_t byte = (cursor_x - x) / 8;
				uint8_t bit =  (cursor_x - x) % 8;

				if (metrics.glyph()[(cursor_y - y) * metrics.stride_or_wordlen + byte] & (1 << bit))
					screen::set_pixel(cursor_x, cursor_y, color);
			}
		}
	}

	void decompress_write_glyph(int16_t x, int16_t y, const font::metrics_t& metrics, uint8_t color) {
		// Helper function to get bit
		
		int bit = 8;
		const uint8_t *data = metrics.glyph();
		auto GetBit = [&]() -> uint32_t {
			if (bit == 0) {
				bit = 8;
				++data;
			}
			--bit;
			return (*data & (1 << bit)) >> bit;
		};

		int16_t cursor_x = x, cursor_y = y;
		int total = 0;

		bool rowbuf[metrics.width * 2]; // rowbuf1 gets filled, copied to rowbuf0

		auto WriteBit = [&](bool bit){
			++total;
			rowbuf[cursor_x - x] = bit;
			if (bit) {
				if (cursor_y >= max_y || cursor_y < min_y || cursor_x >= max_x || cursor_x < min_x) goto next;
				screen::set_pixel(cursor_x, cursor_y, color);
			}
next:
			++cursor_x;
			if (cursor_x == x + metrics.width) {
				cursor_x = x;
				++cursor_y;

				memcpy(rowbuf + metrics.width, rowbuf, metrics.width);
			}
		};

		while (total < (metrics.width * metrics.height)) {
			// Read two-bit command
			auto cmd = GetBit() << 1 | GetBit();
			if (total == 0 && cmd == 0) {
				// UNCOMPRESSED_RAW
				while (total < (metrics.width * metrics.height)) {
					WriteBit(GetBit());
				}
				return;
			}
			else if (cmd == 0) {
				// Repetition command
				auto cmd_2 = GetBit();
				if (cmd_2) {
					// REPEAT_ROW
					for (int i = 0; i < metrics.width; ++i)
						WriteBit(rowbuf[i + metrics.width]);
				}
				else {
					// REPEAT_WORD
					int word_index = ((cursor_x - x) / metrics.stride_or_wordlen) * metrics.stride_or_wordlen;
					for (int i = word_index; i < (word_index + metrics.stride_or_wordlen); ++i) {
						WriteBit(rowbuf[i + metrics.width]);
					}
				}
			}
			else if (cmd == 0b10) {
				// COPY_ROW
				for (int i = 0; i < metrics.width; ++i) WriteBit(GetBit());
			}
			else if (cmd == 0b11) {
				// ZERO_WORD
				for (int i = 0; i < metrics.stride_or_wordlen; ++i) WriteBit(0);
			}
			else if (cmd == 0b01) {
				// COPY_WORD
				for (int i = 0; i < metrics.stride_or_wordlen; ++i) WriteBit(GetBit());
			}
		}
	}

	uint16_t /* end pos */ text(int16_t x, int16_t y, const char* text, const font& font, uint8_t color) {
		uint16_t pen = x;

		uint8_t c, c_prev = 0;
		while ((c = *(text++)) != 0) {
			if (c_prev != 0 && font.is<font::HasKern>()) {
				font::kern_t dat;
				if (font.search_kern(c_prev, c, dat)) {
					pen += dat.offset;
				}
			}
			c_prev = c;
			if (font.metrics_offset(c) == 0) continue; // unknown char
			const auto& metrics = *font.metrics(c);
			if (c != ' ') {
				if (font.is<font::Compressed>()) {
					decompress_write_glyph(pen + metrics.bearingX, y - metrics.bearingY, metrics, color);
				}
				else {
					raw_write_glyph(pen + metrics.bearingX, y - metrics.bearingY, metrics, color);
				}
			}

			pen += metrics.advance;
		}

		return pen;
	}

	uint16_t text_size(const char * text, const font &font) {
		uint16_t pen = 0;

		uint8_t c, c_prev = 0;
		while ((c = *(text++)) != 0) {
			if (c_prev != 0 && font.is<font::HasKern>()) {
				font::kern_t dat;
				if (font.search_kern(c_prev, c, dat)) {
					pen += dat.offset;
				}
			}
			c_prev = c;
			if (font.metrics_offset(c) == 0) continue; // unknown char
			const auto& metrics = *font.metrics(c);
			pen += metrics.advance;
		}

		return pen;
	}
}
