// MWatch BT crt0: loads data, clears bss, sets up the global TLS block, then
// calls SystemInit and jumps to main.

#include <stdint.h>
#include <string.h>
#include <picotls.h>

#include <hardware/resets.h>
#include <hardware/clocks.h>
#include <hardware/xosc.h>
#include <hardware/pll.h>
#include <hardware/watchdog.h>

void setup_clocks() {
    clocks_hw->resus.ctrl = 0;
	xosc_init();

	// Start by ensuring the system clock is running off of the USB clock so that PLL
	// calibration runs in a stable state.

    hw_clear_bits(&clocks_hw->clk[clk_sys].ctrl, CLOCKS_CLK_SYS_CTRL_SRC_BITS);
    while (clocks_hw->clk[clk_sys].selected != 0x1)
        tight_loop_contents();
    hw_clear_bits(&clocks_hw->clk[clk_ref].ctrl, CLOCKS_CLK_REF_CTRL_SRC_BITS);
    while (clocks_hw->clk[clk_ref].selected != 0x1)
        tight_loop_contents();

	// Startup the main PLL to 96 MHZ
	pll_init(pll_sys, 1, 1536 * MHZ, 4, 4);
    pll_init(pll_usb, 1, 1200 * MHZ, 5, 5);
	
	// CLK REF = XOSC (12MHz) / 1 = 12MHz
	clock_configure(clk_ref,
					CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
					0, // No aux mux
					12 * MHZ,
					12 * MHZ);

	// CLK SYS = PLL SYS (96MHz) / 1 = 96MHz
	clock_configure(clk_sys,
					CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
					CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
					96 * MHZ, 96 * MHZ);

	// CLK USB = PLL USB (48MHz) / 1 = 48MHz
    clock_configure(clk_usb,
                    0, // No GLMUX
                    CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    48 * MHZ);

    // CLK ADC = PLL USB (48MHZ) / 1 = 48MHz
    clock_configure(clk_adc,
                    0, // No GLMUX
                    CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    48 * MHZ);

    // CLK RTC = PLL USB (48MHz) / 1024 = 46875Hz
    clock_configure(clk_rtc,
                    0, // No GLMUX
                    CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    46875);

    // CLK PERI = PLL USB (48MHz)
	clock_configure(clk_peri,
					0, // Only AUX mux on ADC
					CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
					48 * MHZ,
					48 * MHZ);

	// Set the watchdog tick (so that the timer is ready)
	watchdog_start_tick(XOSC_MHZ);
}

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
extern void uart_stdio_init(void);

void panic(const char *fmt, ...) {
	while (1) {;}
}

void hard_assertion_failure(void) {
	panic("assert fail");
}
void panic_unsupported(void) {
	panic("unsupported");
}
void _exit(int x) {
	panic("exited");
}

// Entry point from NVIC
void Reset_Handler(void) {
	// Setup environment
	bzero(__bss_start, (uintptr_t)__bss_size);
	memcpy(__data_start, __data_source, (uintptr_t)__data_size);
	_set_tls(__tls_base);
	__libc_init_array();
    reset_block(~(
            RESETS_RESET_IO_QSPI_BITS |
            RESETS_RESET_PADS_QSPI_BITS |
            RESETS_RESET_PLL_USB_BITS |
            RESETS_RESET_USBCTRL_BITS |
            RESETS_RESET_SYSCFG_BITS |
            RESETS_RESET_PLL_SYS_BITS
    ));
    unreset_block_wait(RESETS_RESET_BITS & ~(
            RESETS_RESET_ADC_BITS |
            RESETS_RESET_RTC_BITS |
            RESETS_RESET_SPI0_BITS |
            RESETS_RESET_SPI1_BITS |
            RESETS_RESET_UART0_BITS |
            RESETS_RESET_UART1_BITS |
            RESETS_RESET_USBCTRL_BITS
    ));
	setup_clocks();
	unreset_block_wait(RESETS_RESET_BITS);
	uart_stdio_init();
	main();
	// trap on main return
	for (;;) {}
}
