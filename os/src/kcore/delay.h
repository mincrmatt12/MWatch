#pragma once

#include <mwk/exc/delay.h>

namespace mwos::kcore {
	struct delay_t {
		uint64_t current_time();
			
		inline auto until(uint64_t tick) {
			return drs.delay_until(tick);
		}

		inline auto by_us(uint64_t offset) {
			return drs.delay_until(current_time() + offset);
		}

		inline auto by(uint32_t milliseconds) {
			return by_us(milliseconds * 1000);
		}

		inline auto by_s(uint32_t seconds) {
			return by(seconds * 1000);
		}

		void process() {
			drs.process_wakeup_at_time(current_time());
		}
	private:
		mwk::exc::delay_resume_source drs;
	};
};

namespace mwos {
	extern kcore::delay_t delay;
}
