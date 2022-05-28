#include "./pwr.h"
#include "pico/platform.h"
#include <cstdio>

void mwos::pwr::set_5v_enable(bool on) {
	// DBG: print out
	printf("activate 5v %d\n", on);
	while (getchar() != '5') tight_loop_contents();
	puts("done");
}

void mwos::pwr::set_3v2_enable(bool on) {
	// DBG: print out
	printf("activate 3v2 %d\n", on);
	while (getchar() != '3') tight_loop_contents();
	puts("done");
}
