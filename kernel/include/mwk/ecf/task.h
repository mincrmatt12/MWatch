#pragma once

#include <coroutine>
#include "../utl/err.h"
#include "../utl/result.h"
#include "./sys_error.h"
#include "mwk/ecf/throw.h"

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
		// The specialization-independent parts of the task promise.
		class common_task_promise_base {
			std::coroutine_handle<> continuation; // Caller (forms continuation stack)
			std::coroutine_handle<> (*exception_reraise_thunk)(utl::encoded_error_t upcast_from, std::coroutine_handle<> into){};

			struct start_previous {
				bool await_ready() noexcept { return false; } // calls by returning a new coroutine handle
				void await_resume() noexcept { }              // resumes by changing active coroutine
				template<typename Promise>
				std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> coro) noexcept {
					// Expecting promise to be a common_task_promise, so it has a result()
					auto& promise = coro.promise();
					if (promise.exception_reraise_thunk && promise.result().is_error())
						return promise.exception_reraise_thunk(promise.result().get_encoded_error(), promise.continuation);
					else
						return promise.continuation;
				}
			};
		public:
			// Set the continuation of this coroutine to `caller`.
			void continue_in(std::coroutine_handle<> caller) {
				continuation = caller;
			}

			// Continue in the given coroutine, sending exceptions upwards
			void continue_but_rethrow(std::coroutine_handle<> caller, std::coroutine_handle<> (*reraise_thunk)(utl::encoded_error_t, std::coroutine_handle<>)) {
				continuation = caller;
				exception_reraise_thunk = reraise_thunk;
			}

			std::suspend_always initial_suspend() noexcept { return {}; }
			start_previous      final_suspend()   noexcept { return {}; }
		};

		template<is_task_result Holder>
		struct common_task_promise : common_task_promise_base {
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
		protected:
			Holder result_holder {system_error::not_completed, utl::as_error{}};
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

		basic_task() = default;
		~basic_task() {
			if (auto h = coro_handle()) h.destroy();
		}
		basic_task(const basic_task&) = delete;
		basic_task(basic_task&& t) : coroutine_address(std::exchange(t.coroutine_address, {system_error::already_awaited})) {}
		basic_task& operator=(const basic_task&) = delete;
		basic_task& operator=(basic_task&& o) {
			if (std::addressof(o) == this) return *this;
			this->~basic_task();
			new (this) basic_task(std::move(o));
			return *this;
		}

		explicit operator bool() const {
			return !coroutine_address.is_error();
		}

		bool has_task_handle_error() const {
			return !coroutine_address.is_error();
		}

		system_error task_handle_error() const {
			return coroutine_address.as_error_code<system_error>();
		}

		bool is_ready() const {
			if (auto h = coro_handle()) return h.done();
			return false;
		}

		void start_sync() {
			if (!is_ready()) {
				coro_handle().promise().continue_in(std::noop_coroutine());
				coro_handle().resume();
			}
		}

		// Resume the coroutine synchronously -- you almost _never_ want to do this yourself.
		void resume_sync() {
			if (!is_ready()) coro_handle().resume();
		}

		utl::result<std::coroutine_handle<>, system_error> extract_coroutine_handle(std::coroutine_handle<> continuation) && {
			if (coroutine_address.is_error()) return {coroutine_address.as_error_code<system_error>()};
			else {
				coro_handle().promise().continue_in(continuation);
				return std::exchange(coroutine_address, {system_error::already_awaited}).transform<std::coroutine_handle<>>(std::coroutine_handle<>::from_address);
			}
		}
	private:
		using wrapped_coro_t = utl::basic_result<utl::result_traits::aligned_ptr<void>, system_error>;

		// Holds the to-be-awaited coroutine without setting up the return value.
		struct common_wrapped_task_awaiter {
			common_wrapped_task_awaiter(wrapped_coro_t t) : coro(t) {}

			bool await_ready() const noexcept {
				if (!coro) return true;
				auto coro_promise = std::coroutine_handle<promise_type>::from_address(coro.get());
				return coro_promise.done();
			}
			std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
				auto coro_promise = std::coroutine_handle<promise_type>::from_address(coro.get());
				coro_promise.promise().continue_in(caller);
				return coro_promise;
			}
		protected:
			wrapped_coro_t coro;
		};

		// Holds the to-be-awaited coroutine assuming no handle_error is present
		template<bool Move>
		struct common_raw_task_awaiter {
			common_raw_task_awaiter(std::coroutine_handle<promise_type> t) : coro(t) {}
			
			bool await_ready() const noexcept {
				return coro.done();
			}
			std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
				coro.promise().continue_in(caller);
				return coro;
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

		const std::coroutine_handle<promise_type> coro_handle() const {
			if (coroutine_address) return std::coroutine_handle<promise_type>::from_address(coroutine_address.get());
			else return {};
		}

		std::coroutine_handle<promise_type> coro_handle() {
			return const_cast<std::coroutine_handle<promise_type>&&>(std::as_const(*this).coro_handle());
		}

		template<typename WrappedCoro> requires (std::is_same_v<std::decay_t<WrappedCoro>, wrapped_coro_t>)
		static Result recover_true_result(WrappedCoro&& wc) {
			if (wc) return std::coroutine_handle<promise_type>::from_address(std::forward<WrappedCoro>(wc).get()).promise().result();
			else return {std::forward<WrappedCoro>(wc).template as_error_code<system_error>(), utl::as_error{}};
		}

		wrapped_coro_t coroutine_address {
			system_error::already_awaited
		};

	public:
		// Wait for the raw result, including potential allocation failures. Note that using this directly will _always_ do at least
		// one copy of the result object, as it might have to initialize a new result object pulled from the allocation failure result.
		//
		// If you're directly co_awaiting with the rethrow mechanism, you can ignore this. If you want to get the result<> object but
		// ignore system_errors coming from allocation failures, you can use result() -- that formulation will 'correctly' directly propagate
		// references to the promise's result object.
		auto raw_result() const & noexcept {
			// lvalue version, keeps copy around
			struct copy_awaiter : common_wrapped_task_awaiter {
				using common_wrapped_task_awaiter::common_wrapped_task_awaiter;

				auto await_resume() {
					return recover_true_result(this->coro);
				}
			};

			return copy_awaiter{coroutine_address};
		}

		auto raw_result() const && noexcept {
			// rvalue version, returns via move
			struct move_awaiter : common_wrapped_task_awaiter {
				using common_wrapped_task_awaiter::common_wrapped_task_awaiter;

				auto await_resume() {
					return recover_true_result(std::move(this->coro));
				}
			};

			return move_awaiter{coroutine_address};
		}

		auto result() const & noexcept {
			return awaitable_result(coroutine_address.transform<common_raw_task_awaiter<false>>(std::coroutine_handle<promise_type>::from_address));
		}
		auto result() && noexcept {
			return awaitable_result(coroutine_address.transform<common_raw_task_awaiter<true>>(std::coroutine_handle<promise_type>::from_address));
		}
	private:
		basic_task(promise_type& promise) : coroutine_address(std::coroutine_handle<promise_type>::from_promise(promise).address()) {}
		basic_task(detail::from_allocation_failure) : coroutine_address(system_error::out_of_memory) {}
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
				typename task_t::wrapped_coro_t coro;

				bool await_ready() const noexcept {
					if (!coro) return false;
					auto coro_promise = std::coroutine_handle<typename task_t::promise_type>::from_address(coro.get());
					return coro_promise.done();
				}
				std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
					// If there's already an error inside coro (allocation fail), just directly return it
					if (!coro) {
						return bridge_t::throw_in(caller, coro.template as_error_code<system_error>());
					}
					else {
						auto coro_promise = std::coroutine_handle<typename task_t::promise_type>::from_address(coro.get());
						coro_promise.promise().continue_but_rethrow(caller, &throwable_traits::throw_in_thunk<SuspendedTasklike>);
						return coro_promise;
					}
				}
				decltype(auto) await_resume() {
					// if we call resume, we didn't rethrow
					auto coro_promise = std::coroutine_handle<typename task_t::promise_type>::from_address(coro.get());
					if constexpr (std::is_rvalue_reference_v<Task>) {
						return std::move(coro_promise.promise()).result().get();
					}
					else {
						return coro_promise.promise().result().get();
					}
				}
			};

			return hybrid_awaiter{.coro = result.coroutine_address};
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
