#include <stdio.h>

#include <hardware/uart.h>
#include <hardware/gpio.h>

void uart_stdio_init(void) {
	gpio_set_function(0, GPIO_FUNC_UART);
	gpio_set_function(1, GPIO_FUNC_UART);
	uart_init(uart0, 115200);
}

static int uart_stdio_putc(char c, FILE *file) {
	(void)file;
	while (!uart_is_writable(uart0)) tight_loop_contents();
	uart0_hw->dr = c;
	return c;
}

static int uart_stdio_getc(FILE *file) {
	(void)file;
	while (!uart_is_readable(uart0)) tight_loop_contents();
	return (uint8_t)uart0_hw->dr;
}

static FILE __stdio = FDEV_SETUP_STREAM(uart_stdio_putc, uart_stdio_getc, NULL, _FDEV_SETUP_RW);
FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);
