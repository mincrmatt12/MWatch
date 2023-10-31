#include <stdio.h>
#include <pico/platform.h>
#include "hw/pwr.h"
#include "hw/screen.h"
#include "kcore/delay.h"

#include <mwk/exc/task_manager.h>

#include "draw/font.h"
#include "draw/rect.h"

#include <font/djv_16.h>
#include <font/lato_32.h>

// Global task manager
mwk::exc::task_manager global_tm;

uint8_t pack_color(int id) {
	int r = id % 4;
	int g = (id / 4) % 4;
	int b = (id / 16) % 4;

	//g = b = 2;

	return (r & 1) | (g & 1) << 1 | (b & 1) << 2 | (r >> 1) << 3 | (g >> 1) << 4 | (b >> 1) << 5;
}

mwk::task<void> main_task() {
	using namespace mwos::screen;
	
	// turn on the screen (dbg)
	co_await power_up();

	// write "HI" and a line
	
	int idx = 0;
	while (true) {
		co_await scanout_and_wait_frame();

		mwos::draw::fill(0);
		++idx;
		char buf[32]; snprintf(buf, 32, "r: %d, g: %d, b: %d", idx % 4, (idx / 4) % 4, (idx / 16) % 4);
		puts(buf);
		mwos::draw::text(50, 80, "color tester", mwos::res::font::djv_16, 0b111'000);
		mwos::draw::text(10, 120, buf, mwos::res::font::lato_32, 0xff);
		mwos::draw::rect(15, 130, 200, 180, pack_color(idx % 0x40));

		co_await mwos::delay.by(250);
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
