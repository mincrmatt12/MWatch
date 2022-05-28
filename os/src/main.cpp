#include <stdio.h>
#include "hw/screen.h"
#include "pico/stdlib.h"
#include "pico/runtime.h"

int main() {
	// setup clocks
	
	set_sys_clock_khz(96000, true); // goto 96mhz woooooo
	sleep_ms(5);
	setup_default_uart();
	puts("hi!");
	puts("hi!");
	puts("hi!");
	puts("hi!");
	puts("hi!");
	
	// turn on the screen (dbg)
	mwos::screen::module.power_up();
}
