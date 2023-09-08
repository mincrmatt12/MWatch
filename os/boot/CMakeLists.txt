include(min-pico-sdk-dirs)
find_package(Python 3 COMPONENTS Interpreter)

add_executable(boot_rom_stage2_raw ${CMAKE_CURRENT_LIST_DIR}/rom_stage2/stage2.S)

target_include_directories(boot_rom_stage2_raw PUBLIC
	${picosdk_boot2_dir}/asminclude
)
target_link_libraries(boot_rom_stage2_raw picosdk_base)
target_link_options(boot_rom_stage2_raw PRIVATE
	"LINKER:--script=${picosdk_boot2_dir}/boot_stage2.ld"
)

# Generate binary & sources
add_custom_command(TARGET boot_rom_stage2_raw
	POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:boot_rom_stage2_raw> ${CMAKE_CURRENT_BINARY_DIR}/boot_rom_stage2_raw.bin
	COMMAND ${Python_EXECUTABLE} ${picosdk_boot2_dir}/pad_checksum -s 0xffffffff ${CMAKE_CURRENT_BINARY_DIR}/boot_rom_stage2_raw.bin ${CMAKE_CURRENT_BINARY_DIR}/boot_rom_stage2_pad.s
	BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/boot_rom_stage2_raw.bin ${CMAKE_CURRENT_BINARY_DIR}/boot_rom_stage2_pad.s
)

add_library(boot_rom_stage2 OBJECT ${CMAKE_CURRENT_BINARY_DIR}/boot_rom_stage2_pad.s)
