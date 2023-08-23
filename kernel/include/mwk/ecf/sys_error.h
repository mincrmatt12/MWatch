#pragma once

#include <stdint.h>

namespace mwk::ecf {
	// Common system errors for the kernel. Some modules
	// use further refined versions of this, however the majority
	// just take a system_error and call it a day.
	enum struct system_error : uint16_t {
		out_of_memory, // Unable to allocate heap
		already_awaited, // Attempt to re-await a coroutine whose handle has been emptied
		not_completed, // Access to coroutine result which is still running.
		null_reference,

		max
	};
}
