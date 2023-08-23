#pragma once

#include "task_manager.h"

namespace mwk::exc {
	// TODO: big global config struct
	using tick_type_t = uint32_t;

	// FreeRTOS-style task-delay list (blocked list is sorted on resume tick)
	class delay_resume_source {
		struct delay_block_args {
			tick_type_t wakeup_at{};
		};

		using source_t = resume_source<delay_block_args>;

		source_t underlying;
		friend struct delay_awaitable;

		static source_t::blocked_list_iterator find_position(source_t& in, tick_type_t ts);

		struct delay_awaitable : std::suspend_always {
			resume_source<delay_block_args>::resume_token t;
			
			std::coroutine_handle<> await_suspend(std::coroutine_handle<> task) {
				t.token.suspend_with_task_at_position(task, find_position(*t.source(), t.extra_data.wakeup_at));
				return t.token.tail_call();
			}
		};

	public:
		void process_wakeup_at_time(tick_type_t current_time);
		delay_awaitable delay_until(tick_type_t target_time) {
			return {
				.t {
					.token{&this->underlying},
					.extra_data{
						.wakeup_at = target_time
					}
				}
			};
		}
	};
};
