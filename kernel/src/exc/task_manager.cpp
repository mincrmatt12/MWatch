#include <mwk/exc/task_manager.h>

namespace mwk::exc {
	thread_local task_manager * volatile task_manager::active_task_manager = nullptr;

	bool task_manager::ready_to_resume() const {
		return tasks_made_ready || !ready.empty() || !waiting_root_tasks.empty();
	}

	std::coroutine_handle<> task_manager::pop_next() { // TODO: critical
		active_task_manager = this;
		if (waiting_root_tasks) {
			auto * root_record = root_tasks.push_front(waiting_root_tasks.pop_front());
			return root_record->extracted;
		}
		else if (ready || tasks_made_ready) {
			tasks_made_ready = tasks_made_ready - 1;
			auto * tcb_to_start = ready.delete_at(ready.first());
			auto handle = tcb_to_start->target;
			tcb_to_start->target = {};
			tcb_to_start->parent = nullptr;
			return handle;
		}
		else return std::noop_coroutine();
	}

	std::coroutine_handle<> detail::resume_token::tail_call() {
		if (current_state() == suspended) return ecf::symmetric_transfer(parent->pop_next());
		return std::noop_coroutine();
	}

	void detail::resume_token::suspend_with_task(std::coroutine_handle<> task) {
		// assert(current_state() == waiting_for_task)
		suspend_with_task_at_position(task, resumer->begin());
	}
	void detail::resume_token::suspend_with_task_at_position(std::coroutine_handle<> task, coro_list::iterator it) {
		target = task;
		parent = task_manager::active_task_manager;
		resumer->blocking.insert_before(it, this);
	}
	void detail::resume_token::insert_into_ready_list() {
		resumer->blocking.delete_at(this);
		resumer = nullptr;
		parent->ready.push_back(this);
		parent->tasks_made_ready = parent->tasks_made_ready + 1;
	}

	void detail::resume_token::abandon() && {
		switch (current_state()) {
			case suspended:
				resumer->blocking.delete_at(this);
				break;
			case waiting_for_resume:
				parent->ready.delete_at(this);
				break;
			default: break;
		}
		parent = nullptr;
		resumer = nullptr;
		target = {};
	}

	detail::resume_token::~resume_token() {
		std::move(*this).abandon();
	}

	bool task_manager::has_waiting_tasks() const {
		return ready || waiting_root_tasks || root_tasks;
	}

	void task_manager::root_coro_entry::cleanup_root_task(root_coro_entry *te) {
		te->extracted.destroy();
		delete active_task_manager->root_tasks.delete_at(te);
	}
}
