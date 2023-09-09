#pragma once

#include <optional>
#include "./task_manager.h"

namespace mwk::exc {
	// Captures an "event object" and sends it to tasks which are waiting for it. Acts as a basic sync gate;
	// if an event is sent to the broadcaster before anything is waiting on it, the event is held back until tasks
	// are ready.
	template<typename Event>
	struct async_broadcaster {
		bool event_awaiting_processing() const {
			return received_early.has_value();
		}
		bool tasks_awaiting_event() const {
			return source.has_waiting_tasks();
		}

	private:
		struct extra_data {
			std::optional<Event> received;
		};

		std::optional<Event> received_early;

		using source_t = resume_source<extra_data>;
		source_t source;

	public:
		void post_event(const Event& e) {
			if (!tasks_awaiting_event()) {
				received_early = e;
			}
			else {
				received_early.reset();
				for (auto &awaiter : source) {
					awaiter.extra_data.received = e;
				}
				while (source.has_waiting_tasks()) source.begin()->token.insert_into_ready_list();
			}
		}

		struct value_awaiter {
			typename source_t::resume_token t;

			bool await_ready() const {
				return t.extra_data.received.has_value();
			}
			Event&& await_resume() {
				return std::move(t.extra_data.received).value();
			}
			std::coroutine_handle<> await_suspend(std::coroutine_handle<> task) {
				t.token.suspend_with_task(task);
				return t.token.tail_call();
			}
		};

		auto wait() {
			if (event_awaiting_processing()) {
				return value_awaiter {
					.t {
						.extra_data {
							.received {std::exchange(received_early, std::nullopt)}
						}
					}
				};
			}
			else {
				return value_awaiter {
					.t {
						.token{&this->source},
					}
				};
			}
		}

		auto operator co_await() {
			return wait();
		}
	};
}
