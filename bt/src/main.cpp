#include "pwr/chip.h"
#include "pwr/if.h"

#include <mwk/exc/task_manager.h>

using namespace bt;

mwk::exc::task_manager global_tm;

mwk::task<void> prepare_hw() {
	if ((co_await pwr::enable_rp2040(true).result()).is_error()) {
		// TODO: PANIC
	}
}

int main() {
	// INIT HARDWARE (not coroutines; pre-scheduler coming online)
	pwr::init();

	// call prepare_hw
	global_tm.detach_task(prepare_hw()); // todo: check result

	// SCHEDULER LOOP
	while (1) {
		while (global_tm.ready_to_resume()) global_tm.resume_next();
		// ticks, etc.
	}
}
