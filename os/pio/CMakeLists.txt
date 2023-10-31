find_program(PIOASM_EXECUTABLE pioasm REQUIRED)

set(MWOS_PIOS_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/include/mwos_pios)

add_library(mwos_pios INTERFACE)
target_include_directories(mwos_pios INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated/include)

file(MAKE_DIRECTORY ${MWOS_PIOS_DIR})

function(add_pio_target target_name pio_source_file)
	add_custom_command(OUTPUT ${MWOS_PIOS_DIR}/${target_name}.h
		COMMAND ${PIOASM_EXECUTABLE} ${pio_source_file} ${MWOS_PIOS_DIR}/${target_name}.h
		MAIN_DEPENDENCY ${pio_source_file}
	)
	add_custom_target(${target_name}_pioasm DEPENDS ${MWOS_PIOS_DIR}/${target_name}.h)
	add_dependencies(mwos_pios ${target_name}_pioasm)
endfunction()

add_pio_target(screen ${CMAKE_CURRENT_LIST_DIR}/screen.pio)
