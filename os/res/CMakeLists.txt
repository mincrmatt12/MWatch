find_package (Python3 REQUIRED COMPONENTS Interpreter)

set(MWOS_RES_BINS_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/bins)
set(MWOS_RES_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/include)

add_library(mwos_res INTERFACE)
target_include_directories(mwos_res INTERFACE ${MWOS_RES_INCLUDE_DIR})

set(MWOS_RES_SCRIPTS_DIR ${CMAKE_CURRENT_LIST_DIR}/scripts)

set(generic_embed_path ${MWOS_RES_SCRIPTS_DIR}/generic_embed.S)
set(mangle_py_path ${MWOS_RES_SCRIPTS_DIR}/mangle.py)

file(MAKE_DIRECTORY ${MWOS_RES_INCLUDE_DIR})

function(add_generated_res_bin_target target_name bin_relative_path template_path)
	cmake_parse_arguments(PARSE_ARGV 2 arg "" "SUBDIRECTORY" "COMMAND;DEPS;SYMBOL_NS")

	# Compute mangled name
	execute_process(COMMAND ${Python3_EXECUTABLE} ${mangle_py_path} ${arg_SYMBOL_NS}
				OUTPUT_VARIABLE mangled_symbol_name
				OUTPUT_STRIP_TRAILING_WHITESPACE)

	# Create a header file for the target based on its symbol name
	list(GET arg_SYMBOL_NS -1 GENERATED_SYMBOL_NAME)
	set(generated_include_file ${MWOS_RES_INCLUDE_DIR}/${arg_SUBDIRECTORY}/${GENERATED_SYMBOL_NAME}.h)
	file(MAKE_DIRECTORY ${MWOS_RES_INCLUDE_DIR}/${arg_SUBDIRECTORY})
	configure_file(${template_path} ${generated_include_file} @ONLY)

	# Add a target that will link the generated bin.
	set(bin_abs_path ${MWOS_RES_BINS_DIR}/${arg_SUBDIRECTORY}/${bin_relative_path})
	file(MAKE_DIRECTORY ${MWOS_RES_BINS_DIR}/${arg_SUBDIRECTORY})
	add_library(${target_name} OBJECT ${generic_embed_path})
	target_compile_definitions(${target_name} PRIVATE -DSYMBOL_NAME=${mangled_symbol_name} -DBINARY_PATH="${bin_abs_path}")

	# Add a command that generates the target bin
	add_custom_command(OUTPUT ${bin_abs_path}
					   COMMAND ${arg_COMMAND}
					   WORKING_DIRECTORY ${MWOS_RES_BINS_DIR}
					   DEPENDS ${arg_DEPS})
   add_custom_target(${target_name}_gen DEPENDS ${bin_abs_path})
   add_dependencies(${target_name} ${target_name}_gen)
endfunction()

add_subdirectory(font)
target_link_libraries(mwos_res INTERFACE mwos_res_fonts)
