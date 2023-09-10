#pragma once

#include <coroutine>
#include "../utl/err.h"
#include "../utl/result.h"
#include "./sys_error.h"
#include "mwk/ecf/throw.h"
#include "./symtransfer.h"
#include <utility>

namespace mwk::ecf {
	template<typename T>
	concept is_task_result = utl::is_result<T> && T::template may_throw<system_error>();

	template<is_task_result Result> struct basic_task;

	namespace detail {
		template<typename Return, utl::error_category... UserErrors>
		struct task_type_expander {
			using type = basic_task<typename utl::result<Return, system_error>::template add_errors_t<UserErrors...>>;
		};

		template<typename ResultTraits, utl::error_category... InnerErrors, utl::error_category... ExtraErrors>
		struct task_type_expander<utl::basic_result<ResultTraits, InnerErrors...>, ExtraErrors...> {
			using type = basic_task<typename utl::basic_result<ResultTraits, InnerErrors...>::template add_errors_t<system_error, ExtraErrors...>>;
		};
	};

	template<typename Return, utl::error_category... UserErrors>
	using task = typename detail::task_type_expander<Return, UserErrors...>::type;

	namespace detail {
		template<is_task_result Holder>
		class common_task_promise {
			struct start_previous {
				bool await_ready() noexcept { return false; } // calls by returning a new coroutine handle
				void await_resume() noexcept { }              // resumes by changing active coroutine
				template<typename Promise>
				std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> coro) noexcept {
					// Expecting promise to be a common_task_promise, so it has a result()
					auto& promise = coro.promise();
					if (promise.exception_reraise_thunk && promise.result().is_error())
						return symmetric_transfer(promise.exception_reraise_thunk(promise.result().get_encoded_error(), promise.continuation));
					else
						return symmetric_transfer(promise.continuation);
				}
			};

		public:
			static basic_task<Holder> get_return_object_on_allocation_failure();

			Holder& result() & {
				return result_holder;
			}
			Holder&& result() && {
				return std::move(result_holder);
			}

			// Raise an encoded error inside the coroutine and destroy the coroutine state, returning
			// the implicit result of final_suspend().
			//
			// Suitable for sending to continue_but_rethrow
			static std::coroutine_handle<> kill_with_errorcode(utl::encoded_error_t error, std::coroutine_handle<> coro) {
				auto coro_promise = std::coroutine_handle<common_task_promise>::from_address(coro.address());
				auto& instance = coro_promise.promise();
				instance.result_holder = {error, utl::as_error{}};
				// Evil hackery: the first address of the coroutine handle is the resume ptr, which is set to nullptr when the coroutine has hit its final suspend.
				// We can set it to that to convince the compiler that the coroutine has terminated early
				*(reinterpret_cast<void **>(coro.address())) = nullptr;
				// This is not wrapped in symmetric_transfer because the inner suspend should be.
				return instance.final_suspend().await_suspend(coro_promise);
			}

			template<typename Awaitable>
			decltype(auto) await_transform(Awaitable&& awaitable) {
				using await_t = std::decay_t<Awaitable>;

				if constexpr (!is_awaitable_throwable<await_t>) return std::forward<Awaitable>(awaitable);
				else {
					return throwable_traits<await_t>::template bind_awaiter<common_task_promise>(std::forward<Awaitable>(awaitable));
				}
			}

			// Set the continuation of this coroutine to `caller`.
			void continue_in(std::coroutine_handle<> caller) {
				continuation = caller;
			}

			// Continue in the given coroutine, sending exceptions upwards
			void continue_but_rethrow(std::coroutine_handle<> caller, std::coroutine_handle<> (*reraise_thunk)(utl::encoded_error_t, std::coroutine_handle<>)) {
				continuation = caller;
				exception_reraise_thunk = reraise_thunk;
			}

			static std::suspend_always initial_suspend() noexcept { return {}; }
			static start_previous      final_suspend()   noexcept { return {}; }
		protected:
			Holder result_holder {system_error::not_completed, utl::as_error{}};
			std::coroutine_handle<> continuation; // Caller (forms continuation stack)
			std::coroutine_handle<> (*exception_reraise_thunk)(utl::encoded_error_t upcast_from, std::coroutine_handle<> into){};
		};

		template<is_task_result Holder, typename Inner=typename Holder::success_type>
		struct task_promise final : common_task_promise<Holder> {
			basic_task<Holder> get_return_object();

			// Always allow returning via co_return raise(system_error) (e.g.)
			template<utl::error_category Err>
				requires (Holder::template may_throw<Err>())
			void return_value(raise<Err> err) {
				this->result_holder = {err.error, utl::as_error{}};
			}

			template<typename T>
			void return_value(T&& value) {
				this->result_holder = std::forward<T>(value);
			}
		};

		template<is_task_result Holder>
		struct task_promise<Holder, void> final : common_task_promise<Holder> {
			basic_task<Holder> get_return_object();

			void return_void() {
				this->result_holder = {};
			}
		};

		struct from_allocation_failure {};
		inline void dummy_destroy() {};

		template<is_task_result PromiseLike, system_error AsError>
		struct always_complete_error_coroutine_frame {
			// Set resume pointer to null -- results in the coroutine as acting as already finished.
			void (*resume)() = nullptr;
			// Ensure h.destroy() does not break things
			void (*destroy)() = dummy_destroy;
			// Now the coroutine "promise" object. This will be recast and interpreted as a task_promise<PromiseLike> object, which 
			// (due to standard-layout rules) will be laid out as a common_task_promise. The result holder is intentionally placed first
			PromiseLike fake_promise__result_holder {AsError, utl::as_error{}};
			// The continuation/reraise thunks are intentionally not included here as there should be no way to access them from an opaque
			// coroutine handle (nothing should be continue_in-ing a done() coroutine)
		};

		inline bool is_fake_frame(std::coroutine_handle<> handle) {
			struct exposition_coroutine_frame {
				void (*resume)();
				void (*destroy)();
			};

			return static_cast<exposition_coroutine_frame *>(handle.address())->destroy == dummy_destroy;
		}
	}

	template<is_task_result Holder>
	struct throwable_tasklike_bridge<detail::common_task_promise<Holder>> {
		constexpr inline static bool can_throw_into = true;

		template<utl::error_category Thrown>
		static std::coroutine_handle<> throw_in(std::coroutine_handle<> tasklike, Thrown error) {
			static_assert(Holder::decoder_type::template present<Thrown>(), "Exception in implicit rethrow not exposed by calling task");

			return detail::common_task_promise<Holder>::kill_with_errorcode(
				Holder::decoder_type::build(error),
				tasklike
			);
		}
	};

	template<is_task_result Result>
	struct throwable_traits<basic_task<Result>>;

	template<is_task_result Result>
	struct [[nodiscard("ensure you actually start the task (try co_await)")]] basic_task final {
		using promise_type = detail::task_promise<Result>;

		friend promise_type;
		friend detail::common_task_promise<Result>;
		friend throwable_traits<basic_task>;

		operator bool() const {
			return !detail::is_fake_frame(handle);
		}

		bool is_ready() const {
			return handle.done();
		}

		void start_sync() {
			if (!is_ready()) {
				handle.promise().continue_in(std::noop_coroutine());
				handle.resume();
			}
		}

		// Resume the coroutine synchronously -- you almost _never_ want to do this yourself.
		void resume_sync() {
			if (!is_ready()) handle.resume();
		}
	private:
		template<system_error Error>
		static inline std::coroutine_handle<promise_type> produce_erroring_frame() {
			constinit const static detail::always_complete_error_coroutine_frame<Result, Error> instance{};
			return std::coroutine_handle<promise_type>::from_address((void *)&instance);
		}
	public:
		std::coroutine_handle<> extract_coroutine_handle(std::coroutine_handle<> continuation) && {
			if (!detail::is_fake_frame(handle)) handle.promise().continue_in(continuation);
			return std::exchange(handle, produce_erroring_frame<system_error::already_awaited>());
		}

		basic_task() = default;
		~basic_task() {
			handle.destroy();
		}
		basic_task(const basic_task&) = delete;
		basic_task(basic_task&& t) : handle(std::exchange(t.handle, produce_erroring_frame<system_error::already_awaited>())) {}
		basic_task& operator=(const basic_task&) = delete;
		basic_task& operator=(basic_task&& o) {
			if (std::addressof(o) == this) return *this;
			this->~basic_task();
			new (this) basic_task(std::move(o));
			return *this;
		}
	private:
		// Holds the to-be-awaited coroutine assuming no handle_error is present
		template<bool Move>
		struct task_result_awaiter {
			task_result_awaiter(std::coroutine_handle<promise_type> t) : coro(t) {}
			
			bool await_ready() const noexcept {
				return coro.done();
			}
			std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
				coro.promise().continue_in(caller);
				return symmetric_transfer(coro);
			}
			decltype(auto) await_resume() noexcept {
				if constexpr (Move) {
					return std::move(coro.promise()).result();
				}
				else {
					return coro.promise().result();
				}
			}

		protected:
			std::coroutine_handle<promise_type> coro;
		};

		std::coroutine_handle<promise_type> handle;
	public:
		auto result() const & noexcept {
			return task_result_awaiter<false>({this->handle});
		}
		auto result() && noexcept {
			return task_result_awaiter<true>({this->handle});
		}

	private:
		basic_task(promise_type& promise) : handle(std::coroutine_handle<promise_type>::from_promise(promise)) {}
		basic_task(detail::from_allocation_failure) : handle(produce_erroring_frame<system_error::out_of_memory>()) {}
	};

	// Allow awaiting a basic_task directly for its result's underlying object, rethrowing errors.
	template<is_task_result Result>
	struct throwable_traits<basic_task<Result>> {
		constexpr inline static bool produces_errors = true;

	private:
		using task_t = basic_task<Result>;

		template<accepts_awaitable_throwables Target>
		static std::coroutine_handle<> throw_in_thunk(utl::encoded_error_t et, std::coroutine_handle<> them) {
			return Result::decoder_type::visit([them](auto code){
				return throwable_tasklike_bridge<std::decay_t<Target>>::throw_in(them, code);
			}, et);
		}

	public:
		template<accepts_awaitable_throwables SuspendedTasklike, typename Task>
			requires (std::is_same_v<std::decay_t<Task>, basic_task<Result>>)
		static auto bind_awaiter(Task&& result) {
			using bridge_t = throwable_tasklike_bridge<std::decay_t<SuspendedTasklike>>;

			struct hybrid_awaiter : std::suspend_always {
				std::coroutine_handle<typename basic_task<Result>::promise_type> coro;

				bool await_ready() const noexcept {
					return coro.done() && !detail::is_fake_frame(coro);
				}
				std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
					if (detail::is_fake_frame(coro)) {
						return bridge_t::throw_in(caller, coro.promise().result().template as_error_code<system_error>());
					}
					coro.promise().continue_but_rethrow(caller, &throwable_traits::throw_in_thunk<SuspendedTasklike>);
					return coro;
				}
				decltype(auto) await_resume() {
					// if we call resume, we didn't rethrow
					if constexpr (std::is_rvalue_reference_v<Task>) {
						return std::move(coro.promise()).result().get();
					}
					else {
						return coro.promise().result().get();
					}
				}
			};

			return hybrid_awaiter{.coro = result.handle};
		}
	};

	namespace detail {
		template<is_task_result Holder, typename Inner>
		basic_task<Holder> task_promise<Holder, Inner>::get_return_object() {
			return {*this};
		}

		template<is_task_result Holder>
		basic_task<Holder> task_promise<Holder, void>::get_return_object() {
			return {*this};
		}

		template<is_task_result Holder>
		basic_task<Holder> common_task_promise<Holder>::get_return_object_on_allocation_failure() {
			return {from_allocation_failure{}};
		}
	}
}

namespace mwk {
	using mwk::ecf::task;
}
