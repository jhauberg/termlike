cmake_minimum_required(VERSION 3.5)

project(Termlike 
	LANGUAGES C
)

option(TERM_BUILD_EXAMPLES "Build the Termlike sample programs" ON)

# find all library source files
file(GLOB_RECURSE LIB_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.c")
# find all dependency sources that needs to be built
# (note that this also pulls in sources from the glfw submodule)
file(GLOB_RECURSE DEP_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "external/*.c")

# exclude all dependency sources added from the glfw submodule
# (because they might include example sources and other not-needed sources)
list(FILTER DEP_SOURCES EXCLUDE REGEX "/glfw/")

add_library(Termlike STATIC
    "${LIB_SOURCES}"
    "${DEP_SOURCES}"
)

target_compile_options(Termlike PRIVATE "-Wall")
target_compile_features(Termlike PRIVATE c_std_99)

# define 'DEBUG' only on debug builds (e.g. DCMAKE_BUILD_TYPE=Debug)
target_compile_definitions(Termlike PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

set_target_properties(Termlike PROPERTIES
    # override output filename (should result in 'libterm')
    OUTPUT_NAME "term"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

target_include_directories(Termlike
    PUBLIC 
        "include"
    PRIVATE
        "external"
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE "" INTERNAL)

# add entire glfw module
# (note that this uses the CMake configuration provided by the glfw project)
add_subdirectory("external/glfw")

# now link glfw with the Termlike library
target_link_libraries(Termlike PRIVATE glfw)

if (TERM_BUILD_EXAMPLES)
	# build examples
	add_executable(example-cursor "example/cursor.c")
	add_executable(example-bounce "example/bounce.c")
	add_executable(example-hello "example/hello.c")
	add_executable(example-cp437 "example/cp437.c")
	add_executable(example-logo "example/logo.c")
	
	add_executable(test-perf "test/perf.c")

	set(EXAMPLE_EXECUTABLES 
		example-cursor 
		example-bounce 
		example-hello 
		example-cp437
		example-logo 
		test-perf
	)

	set_target_properties(${EXAMPLE_EXECUTABLES} PROPERTIES
	    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)

	foreach(example-target IN LISTS EXAMPLE_EXECUTABLES)
		target_include_directories(${example-target} PRIVATE "include")
	endforeach()

	if (APPLE)
		find_library(IOKIT "IOKit")
		find_library(COCOA "Cocoa")
		find_library(COREVIDEO "CoreVideo")

		set(TERM_FRAMEWORKS ${IOKIT} ${COCOA} ${COREVIDEO})

		foreach(example-target IN LISTS EXAMPLE_EXECUTABLES)
			target_link_libraries(${example-target} ${TERM_FRAMEWORKS})
		endforeach()
	endif()

	foreach(example-target IN LISTS EXAMPLE_EXECUTABLES)
		target_link_libraries(${example-target} Termlike)
	endforeach()
endif()
