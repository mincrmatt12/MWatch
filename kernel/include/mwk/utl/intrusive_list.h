#pragma once

#include "cpp.h"
#include <utility>

namespace mwk::utl {
	template<typename T>
	struct intrusive_list_item;

	template<typename T, intrusive_list_item<T> T::* Member>
	struct intrusive_list;

	template<typename T>
	class intrusive_list_item {
		template<typename _T, intrusive_list_item<_T> _T::* Member>
		friend struct intrusive_list;

		intrusive_list_item * p_prev{};
		T * p_next{};

	public:
		// Note: intrusive list items are not (by default) unmoveable/uncopyable.
		intrusive_list_item() = default;

		bool in_list() const {
			return p_prev;
		}
	private:
		intrusive_list_item(intrusive_list_item *p_prev, T* p_next) : 
			p_prev(p_prev),
			p_next(p_next) {
		}
	};

	template<typename T, intrusive_list_item<T> T::* Member>
	struct intrusive_list {
		using key_t = T;
		using item_t = intrusive_list_item<T>;

	private:
		template<bool Const, bool AsKeys>
		struct basic_iterator {
			friend intrusive_list<T, Member>;

			using ptr_t = std::conditional_t<Const, const item_t *, item_t *>;

			basic_iterator(ptr_t x) : x(x) {}

			bool operator== (const basic_iterator& other) const {
				return other.x == x;
			}

			decltype(auto) operator*() const {
				if constexpr (AsKeys) return *(x->p_next);
				else return x;
			}
			auto operator->() const {
				if constexpr (AsKeys) return x->p_next;
				else return x;
			}

			basic_iterator &operator++() {
				x = item_in(x->p_next);
				return *this;
			}
			basic_iterator operator++(int) {
				auto copy = *this;
				++(*this);
				return copy;
			}
		private:
			ptr_t x;
		};
	public:
		key_t * insert_after(item_t *pos_in, key_t *item) {
			if (last() == pos_in) root.p_prev = item_in(item);
			if (pos_in->p_next) item_in(pos_in->p_next)->p_prev = item_in(item);
			item_in(item)->p_next = std::exchange(pos_in->p_next, item);
			item_in(item)->p_prev = pos_in;
			return item;
		}
		void delete_at(item_t *pos) {
			(pos->p_prev)->p_next = pos->p_next;
			if (pos->p_next) item_in(pos->p_next)->p_prev = pos->p_prev;
			if (last() == pos) root.p_prev = pos->p_prev;
			pos->p_next = nullptr;
			pos->p_prev = nullptr;
		}
		key_t * delete_at(key_t *pos) {
			delete_at(item_in(pos));
			return pos;
		}

		key_t * push_front(key_t *item) {
			return insert_after(&root, item);
		}
		key_t * push_back(key_t *item) {
			return insert_after(root.p_prev, item);
		}
		key_t * pop_front() {
			return delete_at(first());
		}

		auto previous(const item_t * item) const {
			if (item == &root) return nullptr;
			else return item->p_prev;
		}
		auto previous(item_t * item) const {
			return const_cast<item_t *>(previous(std::as_const(item)));
		}
		auto next(const item_t * item) const {
			return item_in(item->p_next);
		}
		auto next(item_t * item) const {
			return const_cast<item_t *>(next(std::as_const(item)));
		}

		item_t *last() {
			return root.p_prev;
		}
		const item_t *last() const {
			return root.p_prev;
		}

		key_t *first() {
			return root.p_next;
		}
		const key_t *first() const {
			return root.p_next;
		}

		operator bool() const {
			return root.p_next != nullptr;
		}
		bool empty() const {
			return root.p_next == nullptr;
		}

		static auto item_in(const T *item) {
			return &(item->*Member);
		}
		static auto item_in(T *item) {
			return &(item->*Member);
		}

		using iterator = basic_iterator<false, true>;
		using const_iterator = basic_iterator<true, true>;

		iterator begin() { return &root; }
		iterator end() { return last(); }
		const_iterator begin() const { return &root; }
		const_iterator end()   const { return last(); }
		const_iterator cbegin() const { return &root; }
		const_iterator cend()   const { return last(); }

		key_t * insert_before(iterator pos_in, key_t *item) {
			return insert_after(pos_in.x, item);
		}
		key_t * insert_after(iterator pos_in, key_t *item) {
			return insert_after((++pos_in).x, item);
		}
	private:
		item_t root {
			&root,
			nullptr
		};
	};
}
