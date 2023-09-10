#include <mwk/ecf/symtransfer.h>

#ifdef MWKERNEL_EMULATE_SYMTRANSFER
thread_local std::coroutine_handle<> mwk::ecf::active_trampoline{};
#endif
