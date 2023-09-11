#pragma once

// Power module: contains routines for doing power line control

#include "mwk/ecf/task.h"

namespace mwos::pwr {

	void init();  // talks to bluetooth module

	// SCREEN LINES:
	
	mwk::task<void> set_5v_enable(bool on);
}
