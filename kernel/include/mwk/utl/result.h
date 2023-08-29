#pragma once

#include "err.h"

namespace mwk::utl {
	// Construct a result from an error.
	struct as_error {};

	namespace detail {
		// Invariants demanded of result storage types:
		//
		//  - copy/move construction from an error-holding result should treat the constructed-from object as const, _even if move-constructing_.
		//  - should be constexpr constructible

		template<typename T>
		struct basic_result_storage {
			inline bool is_error() const {return !object.has_value();}
			inline encoded_error_t get_error() const {return error;}

			T& get() & {
				return *object;
			}
			const T& get() const & {
				return *object;
			}
			T&& get() && {
				return *std::move(object);
			}
			const T&& get() const && {
				return *std::move(object);
			}

			template<
				std::convertible_to<T> Value
			>
			constexpr basic_result_storage(Value&& v) : error(0), object(std::forward<Value>(v)) {}
			constexpr basic_result_storage(encoded_error_t error, as_error) : error(error) {}
		private:
			encoded_error_t error;
			std::optional<T> object;
		};

		struct void_result_storage {
			inline bool is_error() const {return error != 0;}
			inline encoded_error_t get_error() const {return error - 1;}

			constexpr void get() const {}

			constexpr void_result_storage() : error{} {}
			constexpr void_result_storage(encoded_error_t error, as_error) : error(error + 1) {}
		private:
			encoded_error_t error;
		};

		// Aligned pointer result storage will allow a void* so that you can do
		// result_t<aligned_pointer_result_storage<void>, ...> and get a void* wrapped around
		// an error holder.
		template<typename T> requires (alignof(T) >= 2 || std::is_void_v<T>)
		struct aligned_pointer_result_storage {
			inline bool is_error() const {return (address & 1) == 1;}
			inline encoded_error_t get_error() const {return address >> 1;}
			inline T* get() const {return reinterpret_cast<T *>(address);}

			constexpr aligned_pointer_result_storage(T* v) : address(reinterpret_cast<uintptr_t>(v)) {};
			constexpr aligned_pointer_result_storage(encoded_error_t error, as_error) : address((error << 1) | 1) {};
		private:
			uintptr_t address;
		};

		template<typename T> requires (alignof(std::decay_t<T>) >= 2 && std::is_reference_v<T>)
		struct aligned_reference_result_storage : private aligned_pointer_result_storage<std::remove_reference_t<T>> {
			using aligned_pointer_result_storage<T>::is_error;
			using aligned_pointer_result_storage<T>::get_error;
			inline T get() const {return *std::forward<T>(aligned_pointer_result_storage<T>::get());}

			constexpr aligned_reference_result_storage(T v) : aligned_pointer_result_storage<T>(std::addressof(v)) {}
			constexpr aligned_reference_result_storage(encoded_error_t error, as_error) : aligned_pointer_result_storage<T>(error, as_error{}) {}
		};

		template<typename T> requires (alignof(T) < 2)
		struct fallback_reference_result_storage : private basic_result_storage<T*> {
			using basic_result_storage<T*>::is_error;
			using basic_result_storage<T*>::get_error;
			inline T& get() const {return *basic_result_storage<T>::get();}
		};

		template<typename T>
		struct default_result_storage_traits {
			using storage_type = basic_result_storage<T>; // storage helper
			using success_type = T;
			constexpr inline static bool as_rvalue = false; // whether or not the type is trivially constructible so we return as rvalue
		};

		template<typename T> requires (alignof(T) >= 2)
		struct default_result_storage_traits<T*> {
			using storage_type = aligned_pointer_result_storage<T>; 
			using success_type = T*;
			constexpr inline static bool as_rvalue = true;
		};

		template<typename T> requires (alignof(T) >= 2)
		struct default_result_storage_traits<T&> {
			using storage_type = aligned_reference_result_storage<T&>; 
			using success_type = T&;
			constexpr inline static bool as_rvalue = true;
		};

		template<typename T> requires (alignof(T) >= 2)
		struct default_result_storage_traits<T&&> {
			using storage_type = aligned_reference_result_storage<T&&>; 
			using success_type = T&&;
			constexpr inline static bool as_rvalue = true;
		};

		template<typename T> requires (alignof(T) < 2)
		struct default_result_storage_traits<T&> {
			using storage_type = fallback_reference_result_storage<T&>; 
			using success_type = T&;
			constexpr inline static bool as_rvalue = true;
		};

		template<typename T> requires (alignof(T) < 2)
		struct default_result_storage_traits<T&&> {
			using storage_type = fallback_reference_result_storage<T&&>; 
			using success_type = T&&;
			constexpr inline static bool as_rvalue = true;
		};

		template<>
		struct default_result_storage_traits<void> {
			using storage_type = void_result_storage;
			using success_type = void;
			constexpr inline static bool as_rvalue = false;
		};

		template<typename T>
		concept result_storage_traits = requires {
			typename T::storage_type;
			typename T::success_type;
			{ T::as_rvalue } -> std::convertible_to<bool>;
		};

		template<typename Working, typename Deleting, typename... Remaining>
		struct remove_error_helper {
			using next = void;
			using type = Working;
		};

		template<typename Working, typename Deleting, typename Head>
		struct remove_error_helper<Working, Deleting, Head> {
			using next = std::conditional_t<std::is_same_v<Deleting, Head>, Working, typename Working::template add_error_t<Head>>;
			using type = next;
		};

		template<typename Working, typename Deleting, typename Head, typename... Remaining>
		struct remove_error_helper<Working, Deleting, Head, Remaining...> {
			using next = typename remove_error_helper<Working, Deleting, Head>::next;
			using type = typename remove_error_helper<next, Deleting, Remaining...>::type;
		};

		template<typename T>
		concept error_category_or_void = std::is_void_v<T> || error_category<T>;
	}

	namespace result_traits {
		template<typename T>
		struct aligned_ptr {
			using storage_type = detail::aligned_pointer_result_storage<T>;
			using success_type = T*;
			constexpr inline static bool as_rvalue = true;
		};
	}

	template<detail::result_storage_traits R, error_category... Errors>
	struct basic_result : private R::storage_type {
		using success_type = typename R::success_type;
		using decoder_type = error_decoder<Errors...>;
		using storage_type = typename R::storage_type;
		
		template<error_category Add>
		using add_error_t = std::conditional_t<decoder_type::template present<Add>(), basic_result, basic_result<R, Errors..., Add>>;
		template<detail::error_category_or_void Remove> requires (std::is_void_v<Remove> || decoder_type::template present<Remove>())
		using remove_error_t = std::conditional_t<std::is_void_v<Remove>, basic_result, typename detail::remove_error_helper<basic_result<R>, Remove, Errors...>::type>;

		template<error_category... Adds>
		struct add_errors {
			using type = basic_result;
		};
		template<error_category Add0, error_category... Adds>
		struct add_errors<Add0, Adds...> {
			using type = typename add_error_t<Add0>::template add_errors<Adds...>::type;
		};
		template<error_category... Adds>
		using add_errors_t = typename add_errors<Adds...>::type;

		template<detail::error_category_or_void... Removes>
		struct remove_errors {
			using type = basic_result;
		};
		template<detail::error_category_or_void Remove0, detail::error_category_or_void... Removes>
		struct remove_errors<Remove0, Removes...> {
			using type = typename remove_error_t<Remove0>::template remove_errors<Removes...>::type;
		};
		template<detail::error_category_or_void... Removes>
		using remove_errors_t = typename remove_errors<Removes...>::type;

		template<detail::result_storage_traits T>
		using rebind_basic_t = basic_result<T, Errors...>;

		template<typename T>
		using rebind_t = rebind_basic_t<std::conditional_t<detail::result_storage_traits<T>, T, detail::default_result_storage_traits<T>>>;

		// Inherit constructors from actual result types -- also allows construction with encoded error if desired
		using storage_type::storage_type;

		template<error_category E> requires (
			!std::is_constructible_v<storage_type, E> &&
			decoder_type::template present<E>()
		)
		constexpr basic_result(E code) : storage_type(decoder_type::build(code), as_error{}) {}

		template<error_category E> requires (
			decoder_type::template present<E>()
		)
		constexpr basic_result(E code, as_error) : storage_type(decoder_type::build(code), as_error{}) {}

		// Error access
		using storage_type::is_error;
		explicit operator bool() const { return !is_error(); }

		// Unchecked get
		using storage_type::get;

		// Raw error get
		inline encoded_error_t get_encoded_error() const {
			return storage_type::get_error();
		}

		// Casting operators 
		template<detail::result_storage_traits Target, error_category... TargetErrors> requires (
			std::is_constructible_v<typename Target::storage_type, decltype(std::declval<const storage_type&>().get())> &&
			requires(encoded_error_t ee) { decoder_type::template cast<TargetErrors...>(ee); }
		)
		operator basic_result<Target, TargetErrors...>() const& {
			if (is_error()) {
				return basic_result<Target, TargetErrors...>(decoder_type::template cast<TargetErrors...>(this->get_error()), as_error{});
			}
			else {
				return basic_result<Target, TargetErrors...>(this->get());
			}
		}

		template<detail::result_storage_traits Target, error_category... TargetErrors> requires (
			std::is_constructible_v<typename Target::storage_type, decltype(std::declval<storage_type&&>().get())> &&
			requires(encoded_error_t ee) { decoder_type::template cast<TargetErrors...>(ee); }
		)
		operator basic_result<Target, TargetErrors...>() && {
			if (is_error()) {
				return basic_result<Target, TargetErrors...>(decoder_type::template cast<TargetErrors...>(this->get_error()), as_error{});
			}
			else {
				return basic_result<Target, TargetErrors...>(std::forward<std::invoke_result_t<typename storage_type::get, storage_type&&>>(std::move(*this).get()));
			}
		}

		// Error code access
		template<error_category E>
		constexpr inline static auto may_throw() { return decoder_type::template present<E>(); }

		template<error_category E>
		constexpr inline static auto category_code_for() { return decoder_type::template index<E>(); }

		inline auto category_code() const { return decoder_type::index(this->get_error()); }
		inline auto raw_error_code() const { return decoder_type::code(this->get_error()); }

		template<error_category E>
		inline auto holds_error() const { return is_error() && category_code() == category_code_for<E>(); }
		template<error_category E>
		inline E as_error_code() const { return decoder_type::template code<E>(this->get_error()); }
		template<error_category E>
		inline auto holds_error(E specific) const { return is_error() && category_code() == category_code_for<E>() && as_error_code<E>() == specific; }

		template<typename Handler>
			requires (std::is_invocable_v<Handler, Errors> && ...)
		inline auto visit_error(Handler&& handler) 
		{
			return decoder_type::visit(std::forward<Handler>(handler), this->get_error());
		}

		template<typename Handler, typename ConstructFrom>
			requires (std::is_invocable_v<Handler, Errors> && ...)
		inline auto visit_error(Handler&& handler, ConstructFrom&& if_success)
		{
			if (is_error()) return decoder_type::visit(std::forward<Handler>(handler), this->get_error());
			else return std::forward<ConstructFrom>(if_success);
		}

		template<typename Handler, typename ConstructFromSuccess>
		inline auto visit_error_or_unhandled(Handler&& handler, ConstructFromSuccess&& if_success)
		{
			if (is_error()) return decoder_type::visit_or_unhandled(std::forward<Handler>(handler), this->get_error());
			else return std::forward<ConstructFromSuccess>(if_success);
		}

		template<typename Handler, typename ConstructFromSuccess, typename ConstructFromUnhandled>
		inline auto visit_error_or_unhandled(Handler&& handler, ConstructFromSuccess&& if_success, ConstructFromUnhandled&& if_unhandled)
		{
			if (is_error()) return decoder_type::visit_or_unhandled([&](auto code){
				if constexpr (!std::is_same_v<std::decay_t<decltype(code)>, unhandled>) return std::forward<Handler>(handler)(code);
				else return std::forward<ConstructFromUnhandled>(if_unhandled);
			}, this->get_error());
			else return std::forward<ConstructFromSuccess>(if_success);
		}

		template<typename T>
		inline rebind_t<T> cast() const& {
			if (is_error()) return {this->get_error(), as_error{}};
			else return this->get();
		}

		template<typename T>
		inline rebind_t<T> cast() && {
			if (is_error()) return {this->get_error(), as_error{}};
			else return std::move(*this).get();
		}

		template<typename T, typename Handler>
		inline rebind_t<T> transform(Handler&& handler) const& {
			if (is_error()) return {this->get_error(), as_error{}};
			else return std::forward<Handler>(handler)(this->get());
		}

		template<typename T, typename Handler>
		inline rebind_t<T> transform(Handler&& handler) && {
			if (is_error()) return {this->get_error(), as_error{}};
			else return std::forward<Handler>(handler)(std::move(*this).get());
		}

		template<typename T, typename HandlerOrValue>
		inline rebind_t<T> subst(HandlerOrValue&& handler_or_value) const& {
			if (is_error()) return {this->get_error(), as_error{}};

			if constexpr (std::is_invocable_v<HandlerOrValue>)
				return std::forward<HandlerOrValue>(handler_or_value)();
			else
				return handler_or_value;
		}

		template<typename Handler>
			requires (std::is_invocable_v<Handler, Errors> || ...)
		inline auto handle(Handler&& handler) const& -> 
			remove_errors_t<std::conditional_t<std::is_invocable_v<Handler, Errors>, Errors, void>...> {
			if (!is_error()) return this->get();
			else return decoder_type::visit([&](auto code) -> remove_errors_t<std::conditional_t<std::is_invocable_v<Handler, Errors>, Errors, void>...> {
				if constexpr (std::is_invocable_v<Handler, decltype(code)>) {
					return std::forward<Handler>(handler)(code);
				}
				else {
					return {code, as_error{}};
				}
			}, this->get_error());
		}

		template<typename Handler>
			requires (std::is_invocable_v<Handler, Errors> || ...)
		inline auto handle(Handler&& handler) && -> 
			remove_errors_t<std::conditional_t<std::is_invocable_v<Handler, Errors>, Errors, void>...> {
			if (!is_error()) return std::move(*this).get();
			else return decoder_type::visit([&](auto code) -> remove_errors_t<std::conditional_t<std::is_invocable_v<Handler, Errors>, Errors, void>...> {
				if constexpr (std::is_invocable_v<Handler, decltype(code)>) {
					return std::forward<Handler>(handler)(code);
				}
				else {
					return {code, as_error{}};
				}
			}, this->get_error());
		}

		template<typename Handler>
			requires (std::is_invocable_v<Handler, Errors> && ...)
		inline auto get_or(Handler&& handler) const& {
			return handle(handler).get();
		}

		template<typename Handler>
			requires (std::is_invocable_v<Handler, Errors> && ...)
		inline auto get_or(Handler&& handler) && {
			return handle(handler).get();
		}
	};

	template<typename T, error_category... Errors>
	using result = basic_result<detail::default_result_storage_traits<T>, Errors...>;

	namespace detail {
		// Undefined helper for requires
		template<detail::result_storage_traits R, error_category... Errors>
		void check_is_result_impl(const basic_result<R, Errors...>&);
	}

	template<typename T>
	concept is_result = requires(const T& t) {
		detail::check_is_result_impl(t);
	};
}

namespace mwk {
	using utl::result;
}
