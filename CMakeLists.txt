cmake_minimum_required(VERSION 3.3)
project(molecular-programgenerator)

set(CMAKE_CXX_STANDARD 14)

add_library(molecular-programgenerator
	molecular/programgenerator/ProgramGenerator.cpp
	molecular/programgenerator/ProgramGenerator.h
	molecular/programgenerator/ProgramFile.cpp
	molecular/programgenerator/ProgramFile.h
)
target_link_libraries(molecular-programgenerator PUBLIC molecular::util)
add_library(molecular::programgenerator ALIAS molecular-programgenerator)
target_include_directories(molecular-programgenerator PUBLIC .)

add_executable(test-program-generator
	examples/sample1.cpp
	examples/sample1.glsl)
target_link_libraries(test-program-generator PUBLIC molecular::util molecular-programgenerator)
target_include_directories(test-program-generator PUBLIC .)
add_dependencies(test-program-generator copy-shader-example)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sample1.glsl
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/examples/sample1.glsl ${CMAKE_CURRENT_BINARY_DIR}
)
add_custom_target(copy-shader-example
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sample1.glsl
)
