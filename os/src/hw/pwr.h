#pragma once

// Power module: contains routines for doing power line control

namespace mwos::pwr {

	void init();  // talks to bluetooth module


	// SCREEN LINES:
	
	void set_5v_enable(bool on);
	void set_3v2_enable(bool on);
}
