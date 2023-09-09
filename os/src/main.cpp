#include <stdio.h>
#include <pico/platform.h>
#include "hw/screen.h"
#include "kcore/delay.h"

#include <mwk/exc/task_manager.h>

// Global task manager
mwk::exc::task_manager global_tm;

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
	
	set_pixel(100, 120, 0x3f);
	set_pixel(100, 119, 0x3f);
	set_pixel(100, 121, 0x3f);
	set_pixel(101, 120, 0x3f);
	set_pixel(102, 119, 0x3f);
	set_pixel(102, 120, 0x3f);
	set_pixel(102, 121, 0x3f);

	set_pixel(104, 121, 0x3f);
	set_pixel(104, 119, 0x3f);
	set_pixel(105, 119, 0x3f);
	set_pixel(105, 120, 0x3f);
	set_pixel(105, 121, 0x3f);
	set_pixel(106, 121, 0x3f);
	set_pixel(106, 119, 0x3f);

	for (int color = 0; color < 64 ;++color) {
		set_pixel(80+color, 130, color);
	}

	puts("ready");

	// the multicolored line
	
	while (true) {
		co_await scanout_and_wait_frame();
		puts("frame");
		co_await mwos::delay.by(16);
	}
}

int main() {
	// call prepare_hw
	global_tm.detach_task(main_task()); // todo: check result

	// SCHEDULER LOOP
	while (1) {
		if (global_tm.ready_to_resume()) global_tm.pop_next().resume();
		// ticks, etc.
		mwos::delay.process();
	}
}
