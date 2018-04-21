cmake_minimum_required(VERSION 3.3)
project(molecular-programgenerator)

set(CMAKE_CXX_STANDARD 14)

add_library(molecular-programgenerator
	src/ProgramGenerator.cpp
	src/ProgramGenerator.h
	src/ProgramFile.cpp
	src/ProgramFile.h
	src/util/Hash.h
	src/util/Parser.h
)
