#include <mwk/exc/delay.h>

namespace mwk::exc {
	delay_resume_source::source_t::blocked_list_iterator delay_resume_source::find_position(source_t& in, tick_type_t ts) {
		auto it = in.begin();
		for (; it != in.end(); ++it) {
			if (it->extra_data.wakeup_at > ts) break;
		}
		return it;
	}

	void delay_resume_source::process_wakeup_at_time(tick_type_t current_time) {
		while (underlying.has_waiting_tasks() && underlying.begin()->extra_data.wakeup_at <= current_time) {
			underlying.begin()->token.insert_into_ready_list();
		}
	}
}
