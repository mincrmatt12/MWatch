#include <mwk/ecf/task.h>
#include <iostream>
#include <vector>

struct complainer {
	~complainer() {
		std::cout << "goodbye\n";
	}
};

enum struct user_error : uint16_t {
	testa,
	testb,
	testc,

	max
};

mwk::ecf::task<void> side_effects() {
	co_await mwk::ecf::raise(mwk::ecf::system_error::not_completed);

	co_return;
}

mwk::ecf::task<std::vector<int>, user_error> testb(int a) {
	co_await side_effects();

	if (a == 0) co_return mwk::ecf::raise(user_error::testc);
	if (a == 1) co_return mwk::ecf::raise(mwk::ecf::system_error::out_of_memory);
	else co_return std::vector<int>(a, 52);
}

mwk::ecf::task<int, user_error> test(int b) {
	{
		auto res = (co_await testb(b).result()).transform<int>([](auto&& x){return x[0];}).handle([](user_error ec){
			return -1;
		});
		co_return co_await res;
	}
}

int main() {
	int x; std::cin >> x;
	auto arg = test(x);
	arg.start_sync();
	if (arg.is_ready()) {
		// hack to extract
		auto result = (arg.raw_result().await_resume());
		if (result) std::cout << "ok: " << result.get();
		else std::cout << "fail: " << result.category_code() << ", " << result.raw_error_code() << "\n";
	}
}
