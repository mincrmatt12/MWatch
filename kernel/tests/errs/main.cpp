#include "mwk/utl/result.h"
#include <mwk/utl/err.h>
#include <iostream>

enum struct system_error : uint16_t {
	out_of_memory,
	empty_optional,

	max
};

enum struct user_error : uint16_t {
	testa,
	testb,
	testc,

	max
};

enum struct user_error2 : uint16_t {
	testa,
	testb,
	testc,

	max
};

int main() {
	mwk::utl::result<int, system_error, user_error> test = 5,
										err = {system_error::out_of_memory, mwk::utl::as_error{}},
										err2 = user_error::testb;

	std::cout << "a: " << test.get() << "\n";
	std::cout << "b: " << err.is_error() << "," << err.holds_error<system_error>() << "\n";

	auto partially_handled = err.handle([](system_error ec){return -1;});

	std::cout << partially_handled.is_error() << "," << partially_handled.get() << "\n";

	mwk::utl::result<float, system_error, user_error2, user_error> test3 = test, test4 = err2;

	std::cout << test3.get() << "," << test4.category_code() << "==" << test4.category_code_for<user_error>() << "," << (int)test4.as_error_code<user_error>() << "\n";

	mwk::utl::result<void, system_error> empty{}, empty_err{system_error::out_of_memory};
}
