cmake_minimum_required(VERSION 3.15)
project(deltabase LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)

set(CMAKE_OBJECT_MAX_PATH 4096)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UUID REQUIRED uuid)

if (UUID_FOUND)
    message(STATUS "Found libuuid: ${UUID_LIBRARIES}")
else()
    message(FATAL_ERROR "libuuid not found. Please install it (e.g., 'sudo pacman -S uuid-dev').")
endif()

# --- core ---
file(GLOB_RECURSE CORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c
)

add_library(core STATIC ${CORE_SRCS})

target_include_directories(core PUBLIC ${UUID_INCLUDE_DIRS})
target_link_libraries(core PUBLIC ${UUID_LIBRARIES})

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/include
)

# --- modules ---
function(add_cpp_module module_name)
    file(GLOB_RECURSE MOD_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${module_name}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${module_name}/*.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${module_name}/*.cc
    )

    message(STATUS "${module_name} sources: ${MOD_SRCS}")
    add_library(${module_name} STATIC ${MOD_SRCS})

    target_include_directories(${module_name} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${module_name}/include
    )

    target_link_libraries(${module_name} PRIVATE core)
endfunction()

add_cpp_module(executor)
add_cpp_module(sql)
add_cpp_module(server)
add_cpp_module(misc)
add_cpp_module(cli)

# --- tests --- 
file(GLOB_RECURSE TESTS_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cxx   
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
)

add_executable(test.exe ${TESTS_SRC})
target_link_libraries(test.exe 
    core
    misc
    sql
    executor
    server
    cli
)

# --- main ---
add_executable(main.exe main.cpp)
target_link_libraries(main.exe 
    core
    misc
    sql
    executor
    server
    cli
)
