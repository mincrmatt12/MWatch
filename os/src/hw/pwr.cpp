#include "./pwr.h"
#include "hardware/gpio.h"
#include "../kcore/delay.h"
#include <cstdio>

void mwos::pwr::init() {
	gpio_init(26);
	gpio_put(26, false);
	gpio_set_dir(26, true);
	gpio_put(26, false);
}

mwk::task<void> mwos::pwr::set_5v_enable(bool on) {
	// DBG: print out
	printf("activate 5v %d\n", on);
	co_await delay.by(50);
	gpio_put(26, on);
	co_await delay.by(50);
	puts("done");
}
