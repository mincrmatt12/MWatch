#pragma once

#include "../utl/intrusive_list.h"
#include "../ecf/task.h"
#include <coroutine>

namespace mwk::exc {
	struct task_manager;

	namespace detail {
		struct basic_resume_source;

		struct resume_token {
			enum state {
				empty,
				waiting_for_task,
				suspended,
				waiting_for_resume
			};

			resume_token(const resume_token&) = delete;
			resume_token(resume_token&&) = delete;
			resume_token& operator=(const resume_token&) = delete;
			resume_token& operator=(resume_token&&) = delete;

			resume_token() = default;
			explicit resume_token(basic_resume_source * as_waiting_task) : resumer{as_waiting_task} {}

			~resume_token();
		private:
			task_manager * parent{};
			basic_resume_source * resumer{};
			utl::intrusive_list_item<resume_token> tm_list_item;
			std::coroutine_handle<> target;

		public:
			state current_state() const {
				if (resumer == nullptr) {
					if (target && tm_list_item.in_list()) return waiting_for_resume;
					else return empty;
				}
				else {
					if (target) return suspended;
					return waiting_for_task;
				}
			}

			using coro_list = utl::intrusive_list<detail::resume_token, &detail::resume_token::tm_list_item>;

			friend basic_resume_source;
			friend task_manager;

			inline void prepare_with_resumer(basic_resume_source *rs) {
				// TODO: assert(current_state() == empty || resumer == rs);
				resumer = rs;
			}

			void suspend_with_task(std::coroutine_handle<> task);
			void suspend_with_task_at_position(std::coroutine_handle<> task, coro_list::iterator position);

			void insert_into_ready_list();

			basic_resume_source * source() const {
				if (current_state() == empty || current_state() == waiting_for_resume) return nullptr;
				return resumer;
			}
			task_manager * captured_manager() const {
				if (current_state() < suspended) return nullptr;
				return parent;
			}

			std::coroutine_handle<> tail_call();
			void abandon() &&;
		};
		
		using coro_list = resume_token::coro_list;

		// A single resume_source holds a list of tasks that it is blocking -- precisely how those tasks are unblocked is up to the consumer of the
		// interface: it could use the list as a set of tasks to unblock all at once when an event is ready, it could use the list as an unordered queue
		// of equal-priority responders to a single event, or it could treat them arbitrarily and individually manage which one is unblocked.
		struct basic_resume_source {
			basic_resume_source() {}
			~basic_resume_source() {}

			friend resume_token;

			using blocked_list_iterator = detail::coro_list::iterator;

			auto begin() {return blocking.begin();}
			auto end() {return blocking.end();}

			bool has_waiting_tasks() const {
				return !blocking.empty();
			}
		private:
			detail::coro_list blocking;
		};
	}

	template<typename BlockInfo>
	struct resume_source : detail::basic_resume_source {
		struct resume_token {
			detail::resume_token token;
			[[no_unique_address]] BlockInfo extra_data;

			resume_source *source() const {
				return reinterpret_cast<resume_source *>(token.source());
			}
		};
		
		struct blocked_list_iterator {
			bool operator==(const blocked_list_iterator& o) const {
				return it == o.it;
			}
			
			resume_token& operator*() const {
				return reinterpret_cast<resume_token&>(*it);
			}
			resume_token* operator->() const {
				return reinterpret_cast<resume_token*>(it.operator->());
			}

			blocked_list_iterator& operator++() {
				++it;
				return *this;
			}
			blocked_list_iterator operator++(int) const {
				auto copy = *this;
				++copy;
				return copy;
			}

			operator detail::coro_list::iterator() const {
				return it;
			}
		private:
			blocked_list_iterator(detail::coro_list::iterator it) : it(it) {}

			basic_resume_source::blocked_list_iterator it;

			friend resume_source<BlockInfo>;
		};

		blocked_list_iterator begin() {return basic_resume_source::begin();}
		blocked_list_iterator end() {return  basic_resume_source::end();}
	};

	// A task_manager object is the coroutine equivalent of a scheduler. Unlike full threads, coroutines themselves don't have a priority assignment (mostly
	// because it'd almost never actually be used) and instead are either blocked or ready. The task_manager maintains tokens for events to indicate completion and
	// connects this with the resumption of coroutines in a specific thread.
	
	struct task_manager {
		// When a task is started through the task_manager, this is set to the current task manager.
		//
		// NOTE: this is not treated as valid when running in an ISR
		static thread_local task_manager * volatile active_task_manager;

		// TODO: ~task_manager();

		template<ecf::is_task_result T>
		utl::result<void, ecf::system_error> detach_task(ecf::basic_task<T> &&task) {
			auto root_coro_frame = new (std::nothrow) root_coro_entry{};
			if (root_coro_frame == nullptr) return ecf::system_error::out_of_memory;
			auto h = std::move(task).extract_coroutine_handle(
				std::coroutine_handle<>::from_address(static_cast<void *>(root_coro_frame))
			);
			if (h.is_error()) {
				delete root_coro_frame;
				return h.template as_error_code<ecf::system_error>();
			}
			root_coro_frame->extracted = h.get();
			waiting_root_tasks.push_front(root_coro_frame);
			return {};
		}

		// Polling-based API for resuming tasks in threads (generally suited for single-threaded use w/o preemption)
		bool ready_to_resume() const;
		// Resume the next coroutine. Only valid when ready_to_resume() == true
		std::coroutine_handle<> pop_next();

		// Check if any tasks are ready to execute
		bool has_waiting_tasks() const;
	private:
		struct root_coro_entry {
			static void cleanup_root_task(root_coro_entry *te);

			// Yes, this has to be in ram -- doing so lets us swindle the compiler into thinking this type is
			// in fact a coroutine handle (wild, I know).
			void (* fake_coro_resume)(root_coro_entry *) = cleanup_root_task;
			std::coroutine_handle<> extracted;
			utl::intrusive_list_item<root_coro_entry> list_item;
		};

		friend root_coro_entry;
		friend detail::resume_token;

		// List of suspended awaiters ready to resume
		detail::coro_list ready;
		// Set of all root task entries -- these may be also in the ready/blocked lists
		utl::intrusive_list<root_coro_entry, &root_coro_entry::list_item> root_tasks;
		// Set of all root task entries which have not been started yet (once they are, they get moved to root_tasks)
		utl::intrusive_list<root_coro_entry, &root_coro_entry::list_item> waiting_root_tasks;
	};
}
