#pragma once

#include <stdint.h>

namespace mwos::draw {
	struct font {
		struct kern_t {
			char a, b;
			int8_t offset;
		};

		struct metrics_t {
			uint8_t width, height, stride_or_wordlen;
			int8_t advance, bearingX, bearingY;

			const uint8_t * glyph() const {return ((const uint8_t *)(this) + sizeof(metrics_t));}
		};

		inline const static uint8_t Compressed = 1;
		inline const static uint8_t Italic = 2;
		inline const static uint8_t Bold = 4;
		inline const static uint8_t HasKern = 8;

		char magic[4];
		uint8_t flags;
		uint16_t data_offset;
		uint16_t kern_length;
		uint16_t kern_offset;

		font(const font& other) = delete;

		template<uint8_t Has>
		inline bool is() const {
			return flags & Has;
		}

		const kern_t* kern() const {return (const kern_t *)(((const uint8_t *)(this)) + kern_offset);}
		const uint16_t metrics_offset(char c) const {return ((const uint16_t *)(((const uint8_t *)this) + data_offset))[c];}
		const metrics_t* metrics(char c) const {return (const metrics_t *)(((const uint8_t *)(this)) + metrics_offset(c));}

		bool search_kern(char a, char b, kern_t& out) const {
			uint32_t start = 0, end = kern_length;
			uint16_t needle = (((uint16_t)a << 8) + (uint16_t)b);
			while (start != end) {
				uint32_t head = (start + end) / 2;
				uint16_t current = ((uint16_t)(kern()[head].a) << 8) + (uint16_t)(kern()[head].b);
				if (current == needle) {
					out = kern()[head];
					return true;
				}
				else {
					if (start - end == 1 || end - start == 1) {
						return false;
					}
					if (current > needle) {
						end = head;
					}
					else {
						start = head;
					}
				}
			}
			return true;
		};
	};

	// Draw text to the LCD
	uint16_t /* end pos */ text(int16_t x, int16_t y, const char* text, const font& font, uint8_t color);

	uint16_t text_size(const char *text, const font& font);
};
