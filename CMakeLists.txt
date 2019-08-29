cmake_minimum_required(VERSION 3.3)
project(molecular-programgenerator)

set(CMAKE_CXX_STANDARD 14)

add_library(molecular-programgenerator
	src/ProgramGenerator.cpp
	src/ProgramGenerator.h
	src/ProgramFile.cpp
	src/ProgramFile.h
)
target_link_libraries(molecular-programgenerator PUBLIC molecular::util)
add_library(molecular::programgenerator ALIAS molecular-programgenerator)
target_include_directories(molecular-programgenerator PUBLIC .)
