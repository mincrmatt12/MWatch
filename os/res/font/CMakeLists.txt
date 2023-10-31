find_package (Python3 REQUIRED COMPONENTS Interpreter)

set(template_path ${CMAKE_CURRENT_LIST_DIR}/font_template.h.in)

add_library(mwos_res_fonts STATIC)

function(add_font_target name path size)
	add_generated_res_bin_target(mwos_res_font_${name}_${size} ${name}_${size}.bin ${template_path}
		COMMAND ${Python3_EXECUTABLE} ${MWOS_RES_SCRIPTS_DIR}/fnter.py ${path} ${size} font/${name}_${size}.bin ${ARGN}
		SUBDIRECTORY font
		DEPS ${MWOS_RES_SCRIPTS_DIR}/fnter.py ${path}
		SYMBOL_NS mwos res font ${name}_${size})
endfunction()

add_font_target(djv ${CMAKE_CURRENT_LIST_DIR}/dejavu_sans.ttf 16 c)
add_font_target(djv ${CMAKE_CURRENT_LIST_DIR}/dejavu_sans.ttf 24 c)
add_font_target(lato ${CMAKE_CURRENT_LIST_DIR}/lato.ttf 32 c)

target_link_libraries(mwos_res_fonts
	mwos_res_font_djv_16
	mwos_res_font_djv_24
	mwos_res_font_lato_32
)
