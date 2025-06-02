cmake_minimum_required(VERSION 3.15)
project(deltabase LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE CORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.c
)

add_library(core STATIC ${CORE_SRCS})

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

function(add_cpp_module module_name)
    file(GLOB_RECURSE MOD_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/*.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/*.cc
    )

    add_library(${module_name} STATIC ${MOD_SRCS})

    target_include_directories(${module_name} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/include
    )

    target_link_libraries(${module_name} PRIVATE core)
endfunction()

add_cpp_module(executor)
add_cpp_module(sql)
add_cpp_module(server)
add_cpp_module(misc)
add_cpp_module(tests)

file(GLOB_RECURSE TESTS_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
)

add_executable(test.exe ${TESTS_SRC})
target_link_libraries(test.exe 
    tests
    misc
    executor
    sql
    server
    core
)

add_executable(main.exe main.cpp)
target_link_libraries(main.exe 
    server
    sql
    executor
    misc
    core
)
