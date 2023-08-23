// MWatch BT crt0: loads data, clears bss, sets up the global TLS block, then
// calls SystemInit and jumps to main.

#include <stdint.h>
#include <string.h>
#include <picotls.h>

// Addresses from linker script.
extern char __data_source[];
extern char __data_start[];
extern char __data_end[];
extern char __data_size[];
extern char __bss_start[];
extern char __bss_end[];
extern char __bss_size[];
extern char __tls_base[];
extern char __tdata_end[];
extern char __tls_end[];

// LIBC bits
extern int main(void);
extern void __libc_init_array(void);
extern void SystemInit();

// Entry point from NVIC
void Reset_Handler(void) {
	// Setup environment
	bzero(__bss_start, (uintptr_t)__bss_size);
	memcpy(__data_start, __data_source, (uintptr_t)__data_size);
	_set_tls(__tls_base);
	SystemInit();
	__libc_init_array();
	main();
	// trap on main return
	for (;;) {}
}
