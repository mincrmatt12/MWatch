// Generic binary --> ELF converter with more control over extensions.

	.syntax unified
	.arch armv6m
	.cpu cortex-m0plus
	.thumb

	.section .rodata
	.global SYMBOL_NAME
SYMBOL_NAME:
	.incbin BINARY_PATH
	.size SYMBOL_NAME, .-SYMBOL_NAME
