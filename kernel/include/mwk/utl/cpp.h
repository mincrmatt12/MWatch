#pragma once

#include <concepts>
#include <stddef.h>

namespace mwk::utl {
	// Compile-time template helpers
	namespace comp {
		consteval int log2(int n) {
			if (n == 0) return 0;
			if (n == 1) return 1;
			else return 1 + log2(n/2);
		}

		template<std::integral A0>
		consteval auto max(A0 a0) {
			return a0;
		}

		template<std::integral A0, std::integral A1>
		consteval auto max(A0 a0, A1 a1) {
			return a0 < a1 ? a1 : a0;
		}

		template<std::integral A0, std::integral... Args>
		consteval auto max(A0 a0, Args... args) {
			return max(a0, max(args...));
		}

		template<typename T, typename ...Ts>
		struct pack_index;

		template<typename T, typename ...Ts>
		struct pack_index<T, T, Ts...> : std::integral_constant<int, 0> {};

		template<typename T, typename U, typename ...Ts>
		struct pack_index<T, U, Ts...> : std::integral_constant<int, 1 + pack_index<T, Ts...>::value> {};

		template<typename T, typename ...Ts>
		constexpr inline int pack_index_v = pack_index<T, Ts...>::value;
	}
};
