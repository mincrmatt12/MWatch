#include <iostream>
#include <mwk/utl/intrusive_list.h> 

struct test {
	int value;
	mwk::utl::intrusive_list_item<test> list_item;
};

using list_t = mwk::utl::intrusive_list<test, &test::list_item>;

template<typename T>
void print_list(const T& l) {
	std::cout << "iterable start ---\n";
	for (const auto& x : l) {
		std::cout << x.value << "\n";
	}
	std::cout << "iterable end ---\n";
}

int main() {
	test a{1}, b{2}, c{3}, d{4};

	std::cout << "intrusive list\n";
	list_t the_list;
	print_list(the_list);
	the_list.push_back(&a);
	print_list(the_list);
	the_list.push_back(&b);
	print_list(the_list);
	the_list.push_front(&c);
	the_list.push_front(&d);
	print_list(the_list);
	the_list.delete_at(&c.list_item);
	
	print_list(the_list);

	// clear
	while (the_list) the_list.delete_at(the_list.first());
	print_list(the_list);
}
