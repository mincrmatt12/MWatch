#include <stdio.h>
#include <pico/platform.h>
#include "hw/pwr.h"
#include "hw/screen.h"
#include "kcore/delay.h"

#include <mwk/exc/task_manager.h>

// Global task manager
mwk::exc::task_manager global_tm;

void draw_frame(int idx) {
	for (int color = 0; color < 64 ;++color) {
		for (int y = 130; y < 150; ++y) mwos::screen::set_pixel(80+color, y, (idx+color) % 0x40);
	}
}

void set_big_text(int x, int y, uint8_t c) {
	x -= 100;
	y -= 119;

	x*= 2;
	y*= 2;

	x += 100;
	y += 119;

	for (int ox = 0; ox < 2; ++ox) {
		for (int oy = 0; oy < 2; ++oy) {
			mwos::screen::set_pixel(x+ox, y+oy, c);
		}
	}
}

mwk::task<void> main_task() {
	puts("hi!");
	puts("hi!");
	puts("hi!");
	puts("hi!");
	puts("hi!");
	
	using namespace mwos::screen;
	
	// turn on the screen (dbg)
	co_await power_up();

	// write "HI" and a line
	
	for (int y = 0; y < 50; ++y) {
		for (int x = 120 - y; x < 120 + y; ++x) {
			set_pixel(x, y, (x + y) % 0x40);
		}
	}
	for (int y = 240-50; y < 240; ++y) {
		int off = 240 - y;
		for (int x = 120 - off; x < 120 + off; ++x) {
			set_pixel(x, y, (x + y) % 0x40);
		}
	}
	
	set_big_text(100, 120, 0b100100);
	set_big_text(100, 119, 0b100100);
	set_big_text(100, 121, 0b100100);
	set_big_text(101, 120, 0b100100);
	set_big_text(102, 119, 0b100100);
	set_big_text(102, 120, 0b100100);
	set_big_text(102, 121, 0b100100);

	set_big_text(104, 121, 0b010010);
	set_big_text(104, 119, 0b010010);
	set_big_text(105, 119, 0b010010);
	set_big_text(105, 120, 0b010010);
	set_big_text(105, 121, 0b010010);
	set_big_text(106, 121, 0b010010);
	set_big_text(106, 119, 0b010010);

	set_big_text(108, 119, 0b111001);
	set_big_text(108, 120, 0b111001);
	set_big_text(108, 121, 0b111001);
	set_pixel(116, 122, 0);
	set_pixel(117, 122, 0);
	set_pixel(116, 123, 0);
	set_pixel(117, 123, 0);

	puts("ready");

	// the multicolored line
	
	int idx = 0;
	while (true) {
		co_await scanout_and_wait_frame();
		draw_frame(idx++);
	}
}

int main() {
	mwos::pwr::init();

	// call prepare_hw
	global_tm.detach_task(main_task()); // todo: check result

	// SCHEDULER LOOP
	while (1) {
		if (global_tm.ready_to_resume()) global_tm.resume_next();
		// ticks, etc.
		mwos::delay.process();
	}
}
