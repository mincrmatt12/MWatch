#pragma once

#include "chip.h"

namespace bt::pwr {
	// Core interface to the PMIC. Supports a variety of commands
	
	void init(); // Setup TWI peripheral

	mwk::task<void, pwr_error> enable_rp2040(bool on); // Enable the 1.1V and 3.2V lines. The 1.1V line is started first/killed last.
	mwk::task<void, pwr_error> enable_5v(bool on); // Enable/disable 5V boost line
}
