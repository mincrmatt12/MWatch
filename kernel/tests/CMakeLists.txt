#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

add_executable(coros ./coros/main.cpp)
target_link_libraries(coros mwkernel)

add_executable(errs ./errs/main.cpp)
target_link_libraries(errs mwkernel)

add_executable(ds ./ds/main.cpp)
target_link_libraries(ds mwkernel)

add_executable(single_thread ./single_thread/main.cpp)
target_link_libraries(single_thread mwkernel)
