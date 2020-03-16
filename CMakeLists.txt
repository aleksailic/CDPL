# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("cdpl" CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package (Threads REQUIRED)

# Build tests
add_executable(test "test/test.cpp")
target_link_libraries(test Threads::Threads)
include_directories("include")

# Build examples
file( GLOB_RECURSE EXAMPLE_SRCS "examples/*.cpp" LIST_DIRECTORIES true)
foreach( file ${EXAMPLE_SRCS} )
	file(RELATIVE_PATH dir ${CMAKE_CURRENT_SOURCE_DIR} ${file})
	string(REPLACE "/" "." name ${dir})
	string(REPLACE ".main.cpp" "" name ${name})
	string(REPLACE "examples." "" name ${name})
    add_executable( ${name} ${file})
endforeach( file ${EXAMPLE_SRCS} )

#add_subdirectory ("examples")