#include "./delay.h"
#include <hardware/timer.h>

namespace mwos::kcore {
	// Copied from sdk time_us_64 -- criticalless timer get (instead of the latching one)
	uint64_t delay_t::current_time() {
		// Need to make sure that the upper 32 bits of the timer
		// don't change, so read that first
		uint32_t hi = timer_hw->timerawh;
		uint32_t lo;
		do {
			// Read the lower 32 bits
			lo = timer_hw->timerawl;
			// Now read the upper 32 bits again and
			// check that it hasn't incremented. If it has loop around
			// and read the lower 32 bits again to get an accurate value
			uint32_t next_hi = timer_hw->timerawh;
			if (hi == next_hi) break;
			hi = next_hi;
		} while (true);
		return ((uint64_t) hi << 32u) | lo;
	}
}

mwos::kcore::delay_t mwos::delay{};
