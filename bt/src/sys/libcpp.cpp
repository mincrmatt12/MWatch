// Helper declarations to make libcpp happy

#include <new>
#include <string.h>

const std::nothrow_t std::nothrow;

void * operator new(size_t x, std::nothrow_t const&) noexcept {
	return malloc(x);
}

void operator delete(void *p) {
	free(p);
}

[[noreturn]] void std::__libcpp_verbose_abort(char const *, ...) {
	while (1) {;}
}
