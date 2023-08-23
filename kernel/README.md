# MWos Kernel

Common OS code between bt / main cpu.

Contains preemptive multitasking routines as well as the ecf (elaborated control flow") coroutine abstraction.

## Build setup

The CMake script for kernel registers it as a library called `mwkernel` -- the idea is that it can be easily included from another directory. 
