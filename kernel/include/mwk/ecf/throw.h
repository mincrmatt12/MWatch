#pragma once

#include "../utl/err.h"
#include "../utl/result.h"
#include <coroutine>

namespace mwk::ecf {
	// Specialize this to expose errors
	template<typename T>
	struct throwable_traits {
		constexpr inline static bool produces_errors = false;
	};

	// Specialize this to accept errors
	template<typename Tasklike>
	struct throwable_tasklike_bridge {
		constexpr inline static bool can_throw_into = false;

		// Called after suspension from a bind_awaiter() awaitable and explicitly _before_ resuming. After calling this,
		// the awaitable may be disposed of and so should be treated as destroyed. Returns what should
		// be returned from await_suspend
		template<utl::error_category Thrown>
		static std::coroutine_handle<> throw_in(std::coroutine_handle<> tasklike, Thrown error) = delete;
	};

	template<typename T>
	concept accepts_awaitable_throwables = throwable_tasklike_bridge<std::decay_t<T>>::can_throw_into;
	template<typename T>
	concept is_awaitable_throwable = throwable_traits<std::decay_t<T>>::produces_errors;

	// Allow co_await to unpack a result safely
	template<typename T, utl::error_category ...Exposed>
	struct throwable_traits<utl::result<T, Exposed...>> {
		constexpr inline static bool produces_errors = true;
		using decoder_type = utl::error_decoder<Exposed...>;

		// Called by anything that wants to handle a co_await with Result being of type T (in whatever reference is passed to it) and
		template<accepts_awaitable_throwables SuspendedTasklike, typename Result>
			requires (std::is_same_v<std::decay_t<Result>, utl::result<T, Exposed...>>)
		static auto bind_awaiter(Result&& result) {
			using bridge_t = throwable_tasklike_bridge<std::decay_t<SuspendedTasklike>>;

			struct dummy_awaiter {
				std::decay_t<Result> res;

				bool await_ready() const noexcept {
					return (bool)res;
				}
				auto await_suspend(std::coroutine_handle<> caller) noexcept {
					return res.visit_error([caller](auto ec){
						return bridge_t::throw_in(caller, ec);
					});
				}
				decltype(auto) await_resume() {
					return std::move(res).get();
				}
			};

			return dummy_awaiter{std::forward<Result>(result)};
		}
	};

	// Result such that co_await awaitable_result == co_await (awaitable_result.get());
	template<utl::is_result WrappedAwaitable>
	struct awaitable_result : WrappedAwaitable {
		using WrappedAwaitable::WrappedAwaitable;
		using underlying_t = WrappedAwaitable;

		awaitable_result(WrappedAwaitable&& wa) : WrappedAwaitable(std::move(wa)) {}
		awaitable_result(const WrappedAwaitable& wa) : WrappedAwaitable(wa) {}

		decltype(auto) operator co_await() const& {
			return this->get();
		}

		decltype(auto) operator co_await() && {
			return std::move(*this).get();
		}
	};

	// Allow co_await to unpack a awaitable_result safely
	template<typename T, utl::error_category ...Exposed>
	struct throwable_traits<awaitable_result<utl::result<T, Exposed...>>> {
		constexpr inline static bool produces_errors = true;
		using decoder_type = utl::error_decoder<Exposed...>;

		// Called by anything that wants to handle a co_await with Result being of type T (in whatever reference is passed to it) and
		template<accepts_awaitable_throwables SuspendedTasklike, typename Result>
			requires (std::is_same_v<std::decay_t<Result>, awaitable_result<utl::result<T, Exposed...>>>)
		static auto bind_awaiter(Result&& result) {
			using bridge_t = throwable_tasklike_bridge<std::decay_t<SuspendedTasklike>>;

			struct dummy_awaiter {
				typename std::decay_t<Result>::underlying_t res;

				bool await_ready() const noexcept {
					if (!res) return false;
					else return res.get().await_ready();
				}
				auto await_suspend(std::coroutine_handle<> caller) noexcept {
					if (!res) return res.visit_error([caller](auto ec){
						return bridge_t::throw_in(caller, ec);
					});
					else return res.get().await_suspend(caller);
				}
				decltype(auto) await_resume() {
					return std::move(res).get().await_resume();
				}
			};

			return dummy_awaiter{std::forward<Result>(result)};
		}
	};

	// Can be await-thrown to raise an error
	template<utl::error_category Err>
	struct raise {
		Err error;

		raise(Err error) : error(error) {};
	};

	template<utl::error_category Err>
	struct throwable_traits<raise<Err>> {
		constexpr inline static bool produces_errors = true;

		// Called by anything that wants to handle a co_await with Result being of type T (in whatever reference is passed to it) and
		template<accepts_awaitable_throwables SuspendedTasklike>
		static auto bind_awaiter(raise<Err> raiser) {
			using bridge_t = throwable_tasklike_bridge<std::decay_t<SuspendedTasklike>>;

			struct dummy_awaiter : std::suspend_always {
				Err code;

				auto await_suspend(std::coroutine_handle<> caller) noexcept {
					return bridge_t::throw_in(caller, code);
				}
			};

			return dummy_awaiter{.code = raiser.error};
		}
	};
}

namespace mwk {
	using mwk::ecf::raise;
}
