#include <string.h>
#include <stdio.h>
#include <pico/platform.h>
#include "hw/screen.h"

int main() {
	// setup clocks

	puts("hi!");
	puts("hi!");
	puts("hi!");
	puts("hi!");
	puts("hi!");
	
	using namespace mwos::screen;
	
	// turn on the screen (dbg)
	power_up();

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

	// the multicolored line
	
	while (true) {
		scanout_frame();
		while (!is_finished_scanning()) tight_loop_contents(); // wait for frame to finish if one was in progress
	}
	//scanout_frame();
	//while (!is_finished_scanning()) tight_loop_contents(); // wait for frame to finish if one was in progress

	// brag about it
	puts("hi there ls012b7dd06...?");

	// wait for poweroff command
	while (getchar() != 'D') tight_loop_contents();

	power_off();

	while (1) {}
}
