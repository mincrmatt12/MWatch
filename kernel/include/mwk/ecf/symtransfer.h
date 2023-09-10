#pragma once

#include <coroutine>
#ifdef MWKERNEL_EMULATE_SYMTRANSFER
#include <utility>
#endif

// Helper to work around LLVM not properly tail-calling on armv6 systems.

namespace mwk::ecf {
#ifdef MWKERNEL_EMULATE_SYMTRANSFER
	extern thread_local std::coroutine_handle<> active_trampoline;
#endif

	inline void resume_trampoline(std::coroutine_handle<> handle) {
#ifdef MWKERNEL_EMULATE_SYMTRANSFER
		if (active_trampoline) {
			handle.resume();
		}
		else {
			auto base = std::noop_coroutine();
			active_trampoline = handle;
			do {
				handle = std::exchange(active_trampoline, base);
				handle.resume();
			} while (active_trampoline != base);
			active_trampoline = {};
		}
#else
		handle.resume();
#endif
	}

	inline std::coroutine_handle<> symmetric_transfer(std::coroutine_handle<> handle) {
#ifdef MWKERNEL_EMULATE_SYMTRANSFER
		if (active_trampoline) {
			active_trampoline = handle;
			return std::noop_coroutine();
		}
		else return handle;
#else
		return handle;
#endif
	}
}
