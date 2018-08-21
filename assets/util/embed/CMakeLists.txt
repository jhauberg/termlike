cmake_minimum_required(VERSION 3.5)

project(embed LANGUAGES C)

if (NOT CMAKE_BUILD_TYPE) 
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_executable(embed "src/main.c")

target_compile_options(embed PRIVATE "-Wall")
target_compile_features(embed PRIVATE c_std_99)

set_target_properties(embed PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
