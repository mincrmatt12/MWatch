#pragma once

#include <concepts>
#include <stdint.h>
#include <functional>
#include "./cpp.h"

namespace mwk::utl {
	template<typename T>
	concept error_category = std::is_enum_v<T> && std::is_same_v<std::underlying_type_t<T>, uint16_t> && requires {
		{ T::max };
	};

	template<error_category E>
	struct error_category_traits {
		constexpr inline static uint16_t max = static_cast<uint16_t>(E::max);
	};

	template<error_category... Errs>
	struct error_bundle_traits {
		constexpr inline static int type_bits = comp::log2(sizeof...(Errs) - 1);
		constexpr inline static int max_code = comp::max(error_category_traits<Errs>::max...);
		constexpr inline static int code_bits = comp::log2(max_code);
	};

	struct unhandled {};

	using encoded_error_t = uint32_t;

	template<error_category... Errs>
	struct basic_error_decoder {
		static_assert(sizeof...(Errs) >= 1);

		using traits = error_bundle_traits<Errs...>;

		template<error_category E>
		constexpr inline static bool present() {
			return (std::is_same_v<E, Errs> || ...);
		}

		template<error_category E> requires (present<E>())
		constexpr inline static int index() {
			return comp::pack_index_v<E, Errs...>;
		}

		template<error_category E> requires (present<E>())
		constexpr inline static encoded_error_t build(E code) {
			uint16_t code_int = static_cast<uint16_t>(code);
			return (code_int << traits::type_bits) | index<E>();
		}

		constexpr inline static int index(encoded_error_t encoded) {
			return encoded & ((1 << traits::type_bits) - 1);
		}

		constexpr inline static int code(encoded_error_t encoded) {
			return (encoded >> traits::type_bits) & ((1 << traits::code_bits) - 1);
		}

		template<error_category E> requires (present<E>())
		constexpr inline static E code(encoded_error_t encoded) {
			return static_cast<E>(code(encoded));
		}

		template<error_category... Other> requires (basic_error_decoder<Other...>::template present<Errs>() && ...)
		constexpr static encoded_error_t cast(encoded_error_t us) {
			return visit([](auto code){return basic_error_decoder<Other...>::build(code);}, us);
		}

	private:
		template<typename Handler>
		struct visit_helper {
			template<error_category Head, error_category... Remain>
			inline static auto visit_step(Handler&& handler, encoded_error_t encoded) {
				if constexpr (sizeof...(Remain) == 0) {
					return std::invoke(std::forward<Handler>(handler), code<Head>(encoded));
				}
				else {
					if (index(encoded) == index<Head>()) return std::invoke(std::forward<Handler>(handler), code<Head>(encoded));
					else return visit_step<Remain...>(std::forward<Handler>(handler), encoded);
				}
			}
		};

	public:
		template<typename Handler> requires (std::is_invocable_v<Handler, Errs> && ...)
		static auto visit(Handler&& handler, encoded_error_t encoded) {
			return visit_helper<Handler>::template visit_step<Errs...>(std::forward<Handler>(handler), encoded);
		}

		template<typename Handler> requires (std::is_invocable_v<Handler, unhandled>)
		static auto visit_or_unhandled(Handler&& handler, encoded_error_t encoded) {
			return visit([&](auto code){
				if constexpr(std::is_invocable_v<Handler, decltype(code)>) return std::invoke(std::forward<Handler>(handler), code);
				else return std::invoke(std::forward<Handler>(handler), unhandled{});
			});
		}
	};

	struct null_error_decoder {
		template<error_category E>
		constexpr inline static bool present() {return false;}

		constexpr inline static int index(encoded_error_t encoded) {
			return -1;
		}

		constexpr inline static int code(encoded_error_t encoded) {
			return -1;
		}
	};

	template<error_category... Errs>
	using error_decoder = std::conditional_t<sizeof...(Errs) == 0, null_error_decoder, basic_error_decoder<Errs...>>;
}
