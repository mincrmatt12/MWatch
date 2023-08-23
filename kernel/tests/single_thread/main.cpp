#include "mwk/exc/broadcaster.h"
#include <iostream>
#include <mwk/ecf/task.h>
#include <mwk/exc/delay.h>
#include <mwk/exc/task_manager.h>

mwk::exc::task_manager global_task_manager;
mwk::exc::delay_resume_source global_delayer;

struct message_test {
	int x;
};

mwk::exc::tick_type_t cur_time = 0;
mwk::exc::async_broadcaster<message_test> gate;

mwk::ecf::task<int> task_a(mwk::exc::tick_type_t target) {
	std::cout << "waiting until " << target << "\n";
	co_await global_delayer.delay_until(target);
	std::cout << "ready &" << target << "\n";
	co_return 100;
}

mwk::ecf::task<void> send_at(int x, mwk::exc::tick_type_t target) {
	co_await global_delayer.delay_until(target);
	gate.post_event({x});
}

mwk::ecf::task<int> summationiser(int offset) {
	co_return (co_await task_a(offset + 4) + co_await task_a(offset + 6));
}

mwk::ecf::task<void> receive_message() {
	int r = (co_await gate.wait()).x;
	std::cout << "Got message: " << r << "\n";
}

int main() {
	global_task_manager.detach_task(summationiser(0));
	global_task_manager.detach_task(summationiser(1));
	global_task_manager.detach_task(send_at(20, 4));
	global_task_manager.detach_task(receive_message());

	while (global_task_manager.has_waiting_tasks()) {
		std::cout << "now:: " << cur_time << "\n";
		while (global_task_manager.ready_to_resume()) global_task_manager.pop_next().resume();
		++cur_time;
		global_delayer.process_wakeup_at_time(cur_time);
	}
}
